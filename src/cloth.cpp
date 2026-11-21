#include "../header/cloth.h"

#include <algorithm>
#include <bit>
#include <cmath>

Cloth::Cloth(const ClothSettings& settings)
    : settings_(settings), N(settings.resolution),
      pool_(std::make_unique<ThreadPool>(settings.threadCount))
{
    reset();
}

void Cloth::reset()
{
    buildParticles();
    buildConstraints();
    colorConstraints();
    anyTorn_ = false;
    topologyVersion_++;
}

void Cloth::buildParticles()
{
    particles_.assign(N*N, Particle{});
    const float half = settings_.size * 0.5f;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            Particle& p = at(i, j);
            p.position = glm::vec3(-half + j*spacing(), settings_.height, -half + i*spacing());
            p.prevPosition = p.position;
            p.velocity = glm::vec3(0.0f);
            p.invMass = 1.0f / settings_.particleMass;
        }
    }

    for (int j = 0; j < N; j++)
        at(0, j).invMass = 0.0f;
}

void Cloth::buildConstraints()
{
    constraints_.clear();
    const float s = spacing();
    const float diagonal = s * std::sqrt(2.0f);
    auto idx = [this](int i, int j) { return i*N + j; };
    auto add = [this](int a, int b, float rest, float compliance, ConstraintType type) {
        constraints_.push_back({a, b, rest, compliance, type});
    };

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (j+1 < N) add(idx(i,j), idx(i,j+1), s, settings_.structuralCompliance, ConstraintType::Structural);
            if (i+1 < N) add(idx(i,j), idx(i+1,j), s, settings_.structuralCompliance, ConstraintType::Structural);

            if (i+1 < N && j+1 < N)  add(idx(i,j), idx(i+1,j+1), diagonal, settings_.shearCompliance, ConstraintType::Shear);
            if (i+1 < N && j-1 >= 0) add(idx(i,j), idx(i+1,j-1), diagonal, settings_.shearCompliance, ConstraintType::Shear);

            if (j+2 < N) add(idx(i,j), idx(i,j+2), 2.0f*s, settings_.bendCompliance, ConstraintType::Bend);
            if (i+2 < N) add(idx(i,j), idx(i+2,j), 2.0f*s, settings_.bendCompliance, ConstraintType::Bend);
        }
    }
}

void Cloth::colorConstraints()
{
    std::vector<uint64_t> usedColors(particles_.size(), 0);
    for (auto& c : constraints_)
    {
        const uint64_t taken = usedColors[c.a] | usedColors[c.b];
        const int color = std::countr_one(taken);
        c.color = static_cast<uint8_t>(color);
        usedColors[c.a] |= uint64_t(1) << color;
        usedColors[c.b] |= uint64_t(1) << color;
    }

    std::stable_sort(constraints_.begin(), constraints_.end(),
                     [](const DistanceConstraint& x, const DistanceConstraint& y) { return x.color < y.color; });
    computeColorRanges();
}

void Cloth::computeColorRanges()
{
    colorStart_.assign(1, 0);
    for (int k = 0; k < static_cast<int>(constraints_.size()); ++k)
    {
        while (static_cast<int>(colorStart_.size()) <= constraints_[k].color)
            colorStart_.push_back(k);
    }
    colorStart_.push_back(static_cast<int>(constraints_.size()));
}

void Cloth::releasePins()
{
    for (auto& p : particles_)
        p.invMass = 1.0f / settings_.particleMass;
}

void Cloth::movePinned(int i, int j, const glm::vec3& delta)
{
    Particle& p = at(i, j);
    if (p.invMass != 0.0f)
        return;
    p.position += delta;
    p.prevPosition = p.position;
}

std::vector<glm::vec3> Cloth::getPositions() const
{
    std::vector<glm::vec3> positions(particles_.size());
    for (size_t i = 0; i < particles_.size(); ++i)
        positions[i] = particles_[i].position;
    return positions;
}

void Cloth::update(float dt)
{
    const float h = dt / settings_.substeps;
    int substepsUntilRebuild = 0;
    for (int s = 0; s < settings_.substeps; ++s)
    {
        if (settings_.selfCollision && substepsUntilRebuild == 0)
            substepsUntilRebuild = findSelfCollisionNeighbors(h);
        substep(h);
        substepsUntilRebuild--;
    }

    if (anyTorn_.load(std::memory_order_relaxed))
        removeTornConstraints();
}

namespace
{
constexpr int kParticleGrain = 256;
constexpr int kConstraintGrain = 512;
}

void Cloth::substep(float h)
{
    integrate(h);
    solveConstraints(h);
    if (settings_.selfCollision)
        computeSelfCollisionCorrections();
    finishSubstep(h);
}

void Cloth::integrate(float h)
{
    pool_->parallelFor(static_cast<int>(particles_.size()), kParticleGrain, [this, h](int begin, int end) {
        const glm::vec3 dv = settings_.gravity * h;
        for (int i = begin; i < end; ++i)
        {
            Particle& p = particles_[i];
            p.prevPosition = p.position;
            if (p.invMass == 0.0f)
                continue;
            p.velocity += dv;
            p.position += p.velocity * h;
        }
    });
}

void Cloth::solveConstraints(float h)
{
    const float invH2 = 1.0f / (h*h);
    for (int color = 0; color + 1 < static_cast<int>(colorStart_.size()); ++color)
    {
        const int first = colorStart_[color];
        const int count = colorStart_[color + 1] - first;
        pool_->parallelFor(count, kConstraintGrain, [this, first, invH2](int begin, int end) {
            bool tore = false;
            for (int k = first + begin; k < first + end; ++k)
                tore |= projectConstraint(constraints_[k], invH2);
            if (tore)
                anyTorn_.store(true, std::memory_order_relaxed);
        });
    }
}

bool Cloth::projectConstraint(DistanceConstraint& c, float invH2)
{
    if (c.torn)
        return false;

    Particle& pa = particles_[c.a];
    Particle& pb = particles_[c.b];
    const glm::vec3 delta = pa.position - pb.position;
    const float length = glm::length(delta);

    if (settings_.tearing && length > settings_.tearRatio * c.restLength)
    {
        c.torn = true;
        return true;
    }

    const float w = pa.invMass + pb.invMass;
    if (w == 0.0f || length < 1e-9f)
        return false;

    const float C = length - c.restLength;
    const float alpha = c.compliance * invH2;
    const float dLambda = -C / (w + alpha);
    const glm::vec3 correction = (dLambda / length) * delta;

    pa.position += pa.invMass * correction;
    pb.position -= pb.invMass * correction;
    return false;
}

int Cloth::findSelfCollisionNeighbors(float h)
{
    const int count = static_cast<int>(particles_.size());
    float maxSpeed2 = 0.0f;
    snapshot_.resize(count);
    for (int i = 0; i < count; ++i)
    {
        snapshot_[i] = particles_[i].position;
        maxSpeed2 = glm::max(maxSpeed2, glm::dot(particles_[i].velocity, particles_[i].velocity));
    }

    const float thickness = settings_.selfCollisionThicknessRatio * spacing();
    const float maxStep = 2.0f * std::sqrt(maxSpeed2) * h;
    int interval = settings_.substeps;
    if (maxStep > 0.0f)
        interval = glm::clamp(static_cast<int>(thickness / maxStep), 1, settings_.substeps);
    const float queryRadius = thickness + maxStep * interval;
    const float queryRadius2 = queryRadius * queryRadius;
    hash_.build(snapshot_, queryRadius);

    auto forEachNeighbor = [this, queryRadius2](int i, auto&& fn) {
        if (particles_[i].invMass == 0.0f)
            return;
        const glm::vec3 xi = snapshot_[i];
        hash_.forEachNearby(xi, [&](int j) {
            if (j == i)
                return;
            const glm::vec3 d = xi - snapshot_[j];
            if (glm::dot(d, d) < queryRadius2)
                fn(j);
        });
    };

    neighborStart_.assign(count + 1, 0);
    pool_->parallelFor(count, kParticleGrain, [&](int begin, int end) {
        for (int i = begin; i < end; ++i)
        {
            int found = 0;
            forEachNeighbor(i, [&](int) { found++; });
            neighborStart_[i + 1] = found;
        }
    });

    for (int i = 0; i < count; ++i)
        neighborStart_[i + 1] += neighborStart_[i];
    neighbors_.resize(neighborStart_[count]);

    pool_->parallelFor(count, kParticleGrain, [&](int begin, int end) {
        for (int i = begin; i < end; ++i)
        {
            int k = neighborStart_[i];
            forEachNeighbor(i, [&](int j) { neighbors_[k++] = j; });
        }
    });

    return interval;
}

void Cloth::computeSelfCollisionCorrections()
{
    const float thickness = settings_.selfCollisionThicknessRatio * spacing();
    const int count = static_cast<int>(particles_.size());
    selfCorrections_.resize(count);

    pool_->parallelFor(count, kParticleGrain, [this, thickness](int begin, int end) {
        const float thickness2 = thickness * thickness;
        for (int i = begin; i < end; ++i)
        {
            selfCorrections_[i] = glm::vec3(0.0f);
            const float wi = particles_[i].invMass;
            if (wi == 0.0f)
                continue;

            const int ri = i / N, ci = i % N;
            const glm::vec3 xi = particles_[i].position;
            glm::vec3 correction(0.0f);
            int contacts = 0;

            for (int k = neighborStart_[i]; k < neighborStart_[i + 1]; ++k)
            {
                const int j = neighbors_[k];
                const glm::vec3 d = xi - particles_[j].position;
                const float dist2 = glm::dot(d, d);
                if (dist2 >= thickness2 || dist2 < 1e-12f)
                    continue;

                const int dr = ri - j / N, dc = ci - j % N;
                const float restDist = spacing() * std::sqrt(static_cast<float>(dr*dr + dc*dc));
                const float minDist = glm::min(thickness, restDist);
                const float dist = std::sqrt(dist2);
                if (dist >= minDist)
                    continue;

                const float wj = particles_[j].invMass;
                correction += (minDist - dist) * (wi / (wi + wj)) * (d / dist);
                contacts++;
            }

            if (contacts > 0)
                selfCorrections_[i] = correction / static_cast<float>(contacts);
        }
    });
}

void Cloth::finishSubstep(float h)
{
    const bool self = settings_.selfCollision;
    pool_->parallelFor(static_cast<int>(particles_.size()), kParticleGrain, [this, h, self](int begin, int end) {
        const float damping = glm::max(0.0f, 1.0f - settings_.damping * h);
        for (int i = begin; i < end; ++i)
        {
            Particle& p = particles_[i];
            if (p.invMass == 0.0f)
                continue;
            if (self)
                p.position += selfCorrections_[i];
            collideWithWorld(p);
            p.velocity = (p.position - p.prevPosition) / h * damping;
        }
    });
}

void Cloth::collideWithWorld(Particle& p) const
{
    const float margin = settings_.collisionMargin;

    for (const auto& sphere : colliders_.spheres)
    {
        const glm::vec3 d = p.position - sphere.center;
        const float r = sphere.radius + margin;
        const float dist2 = glm::dot(d, d);
        if (dist2 >= r*r || dist2 < 1e-12f)
            continue;
        const glm::vec3 n = d / std::sqrt(dist2);
        p.position = sphere.center + n * r;
        applyFriction(p, n);
    }

    for (const auto& box : colliders_.boxes)
    {
        const glm::vec3 local = p.position - box.center;
        const glm::vec3 extent = box.halfExtents + glm::vec3(margin);
        const glm::vec3 depth = extent - glm::abs(local);
        if (depth.x <= 0.0f || depth.y <= 0.0f || depth.z <= 0.0f)
            continue;

        int axis = 0;
        if (depth.y < depth[axis]) axis = 1;
        if (depth.z < depth[axis]) axis = 2;

        glm::vec3 n(0.0f);
        n[axis] = local[axis] >= 0.0f ? 1.0f : -1.0f;
        p.position[axis] = box.center[axis] + n[axis] * extent[axis];
        applyFriction(p, n);
    }

    if (colliders_.ground.enabled)
    {
        const float floor = colliders_.ground.height + margin;
        if (p.position.y < floor)
        {
            p.position.y = floor;
            applyFriction(p, glm::vec3(0.0f, 1.0f, 0.0f));
        }
    }
}

void Cloth::applyFriction(Particle& p, const glm::vec3& normal) const
{
    const glm::vec3 moved = p.position - p.prevPosition;
    const glm::vec3 tangential = moved - glm::dot(moved, normal) * normal;
    p.position -= tangential * settings_.friction;
}

void Cloth::removeTornConstraints()
{
    std::erase_if(constraints_, [](const DistanceConstraint& c) { return c.torn; });
    computeColorRanges();
    anyTorn_.store(false, std::memory_order_relaxed);
    topologyVersion_++;
}
