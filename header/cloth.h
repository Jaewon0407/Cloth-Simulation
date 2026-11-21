#pragma once
#include <glm/glm.hpp>
#include "collider.h"
#include <cstdint>
#include <vector>

struct Particle
{
    glm::vec3 position{0.0f};
    glm::vec3 prevPosition{0.0f};
    glm::vec3 velocity{0.0f};
    float invMass = 0.0f;
};

enum class ConstraintType : uint8_t
{
    Structural,
    Shear,
    Bend,
};

struct DistanceConstraint
{
    int a, b;
    float restLength;
    float compliance;
    ConstraintType type;
};

struct ClothSettings
{
    int resolution = 40;
    float size = 2.0f;
    float height = 1.0f;
    float particleMass = 0.0025f;
    int substeps = 15;
    float damping = 0.5f;
    glm::vec3 gravity{0.0f, -9.81f, 0.0f};

    float structuralCompliance = 0.0f;
    float shearCompliance = 1e-3f;
    float bendCompliance = 1e-2f;

    float collisionMargin = 0.02f;
    float friction = 0.3f;
};

class Cloth
{
public:
    explicit Cloth(const ClothSettings& settings = {});

    void update(float dt);
    void reset();

    void releasePins();

    Particle& at(int i, int j) { return particles_[i*N + j]; }
    int resolution() const { return N; }
    float spacing() const { return settings_.size / (N - 1); }

    const std::vector<Particle>& particles() const { return particles_; }
    const std::vector<DistanceConstraint>& constraints() const { return constraints_; }
    std::vector<glm::vec3> getPositions() const;

    CollisionWorld& colliders() { return colliders_; }
    const CollisionWorld& colliders() const { return colliders_; }

private:
    void buildParticles();
    void buildConstraints();

    void substep(float h);
    void integrate(float h);
    void solveConstraints(float h);
    void solveCollisions();
    void applyFriction(Particle& p, const glm::vec3& normal) const;
    void updateVelocities(float h);

    ClothSettings settings_;
    int N;
    std::vector<Particle> particles_;
    std::vector<DistanceConstraint> constraints_;
    CollisionWorld colliders_;
};
