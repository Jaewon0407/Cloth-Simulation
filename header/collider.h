#pragma once
#include <glm/glm.hpp>
#include <vector>

struct SphereCollider
{
    glm::vec3 center{0.0f};
    float radius = 0.5f;
};

struct BoxCollider
{
    glm::vec3 center{0.0f};
    glm::vec3 halfExtents{0.5f};
};

struct GroundCollider
{
    bool enabled = true;
    float height = -1.5f;
};

struct CollisionWorld
{
    std::vector<SphereCollider> spheres;
    std::vector<BoxCollider> boxes;
    GroundCollider ground;
};
