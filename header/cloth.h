#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>

struct Particle 
{
    float mass=0.0025f;
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
                float x = -1+j * spacing;
                float y = 1-i * spacing;
                at(i,j).position = glm::vec3(x,y,0);
            }
        }

        // Initialize spring
        glm::float32 stiffness = 10.0f;
        for (int i=0; i<n; i++) {
            for (int j=0; j<n; j++) {
                float spacing = 2.0f / (n-1);
                // The if statements are used to avoid linking the last particle with 
                // the first particle
                if (j + 1 < n) springs.push_back({i*n+j, i*n+(j+1), spacing, stiffness});
                if (i + 1 < n) springs.push_back({i*n+j, (i+1)*n+j, spacing, stiffness});
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
        const int substeps = 4;
        for (int s = 0; s < substeps; ++s) 
        {
            dt = dt / substeps;

            // apply gravity force
            for (auto& particle : particles) 
            {
                particle.force = glm::vec3(0);
                particle.force += glm::vec3(0, -9.81, 0) * particle.mass;
            }

            // apply spring force
            for (auto& spring : springs)
            {
                glm::vec3 displacement = particles[spring.a].position - particles[spring.b].position;
                glm::float32 currentLength = glm::length(displacement);
                glm::float32 stretch = currentLength - spring.restLength;
                glm::vec3 direction = displacement / currentLength;
                glm::vec3 force = -spring.stiffness * stretch * direction;
                particles[spring.a].force += force;
                particles[spring.b].force -= force;
            }

            // apply total force
            for (auto& particle : particles)
            {
                if (particle.pinned)
                {
                    continue;
                }

                particle.velocity += (particle.force / particle.mass) * dt;
                // add damping
                particle.velocity *= glm::max(0.0f, 1.0f - 0.5f * dt);
                particle.position += particle.velocity * dt;
            }
        }
    }

private:
    std::vector<Spring> springs;
    std::vector<Particle> particles; // want to store as 1D vector, but access it like 2D vector
    int N; // number of particles
};
