#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>

struct Particle 
{
    float mass=1.0f;
    glm::vec3 velocity;
    glm::vec3 position;
    glm::vec3 force;

    bool pinned=false;
};

struct Spring 
{
    int a, b;
    float restLength;
    float stiffness;
};

class Cloth 
{
public:
    Cloth(int n) : N(n) 
    {
        particles.resize(n*n);

        // Initialize the particle indices and 3D world position of each particle
        for (int i=0; i<n; i++) {
            for (int j=0; j<n; j++) {
                float spacing = 2.0f / (n-1);
                // row 0 is top, row n-1 is bottom
                float x = -1+j * spacing;
                float y = 1-i * spacing;
                at(i,j).position = glm::vec3(x,y,0);
            }
        }

        // pin the corners
        at(0,0).pinned=true;
        at(0,n-1).pinned=true;
    }

    Particle& at(int i, int j) 
    {
        return particles[i*N+j];
    }

    std::vector<glm::vec3> getPositions() 
    {
        std::vector<glm::vec3> positions; 
        positions.resize(particles.size()); // initialize the size of positions vector
        for (size_t i = 0; i < particles.size(); ++i)
            positions[i] = particles[i].position;
        return positions;
    }

    void update(float dt) 
    {
        // update for each particle
        for (auto& particle : particles) 
        {
            // skip if the particle is pinned
            if (particle.pinned)
            {
                continue;
            }

            particle.force = glm::vec3(0);
            particle.force = glm::vec3(0, -9.81, 0) * particle.mass;
            particle.velocity += (particle.force / particle.mass) * dt;
            particle.position += particle.velocity * dt;
        }
    }

private:
    std::vector<Spring> springs;
    std::vector<Particle> particles; // want to store as 1D vector, but access it like 2D vector
    int N; // number of particles
};
