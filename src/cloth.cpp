#include "../header/cloth.h"

#include <cmath>

Cloth::Cloth(const ClothSettings& settings)
    : settings_(settings), N(settings.resolution)
{
    reset();
}

void Cloth::reset()
{
    buildParticles();
    buildConstraints();
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

    at(0, 0).invMass = 0.0f;
    at(0, N-1).invMass = 0.0f;
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

void Cloth::releasePins()
{
    for (auto& p : particles_)
        p.invMass = 1.0f / settings_.particleMass;
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
    for (int s = 0; s < settings_.substeps; ++s)
        substep(h);
}

void Cloth::substep(float h)
{
    integrate(h);
    solveConstraints(h);
    updateVelocities(h);
}

void Cloth::integrate(float h)
{
    for (auto& p : particles_)
    {
        p.prevPosition = p.position;
        if (p.invMass == 0.0f)
            continue;
        p.velocity += settings_.gravity * h;
        p.position += p.velocity * h;
    }
}

void Cloth::solveConstraints(float h)
{
    const float invH2 = 1.0f / (h*h);
    for (const auto& c : constraints_)
    {
        Particle& pa = particles_[c.a];
        Particle& pb = particles_[c.b];
        const float w = pa.invMass + pb.invMass;
        if (w == 0.0f)
            continue;

        const glm::vec3 delta = pa.position - pb.position;
        const float length = glm::length(delta);
        if (length < 1e-9f)
            continue;

        const float C = length - c.restLength;
        const float alpha = c.compliance * invH2;
        const float dLambda = -C / (w + alpha);
        const glm::vec3 correction = (dLambda / length) * delta;

        pa.position += pa.invMass * correction;
        pb.position -= pb.invMass * correction;
    }
}

void Cloth::updateVelocities(float h)
{
    const float damping = glm::max(0.0f, 1.0f - settings_.damping * h);
    for (auto& p : particles_)
    {
        if (p.invMass == 0.0f)
            continue;
        p.velocity = (p.position - p.prevPosition) / h * damping;
    }
}
