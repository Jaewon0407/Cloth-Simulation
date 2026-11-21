#include "../header/cloth.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

static double runScene(int resolution, unsigned threads, int frames)
{
    ClothSettings settings;
    settings.resolution = resolution;
    settings.threadCount = threads;
    settings.tearing = false;

    Cloth cloth(settings);
    CollisionWorld& world = cloth.colliders();
    world.spheres = { SphereCollider{glm::vec3(-0.3f, 0.0f, 0.1f), 0.45f} };
    world.boxes = { BoxCollider{glm::vec3(0.6f, -1.1f, 0.3f), glm::vec3(0.35f, 0.4f, 0.35f)} };
    cloth.releasePins();

    const float dt = 1.0f / 60.0f;
    for (int f = 0; f < 10; ++f)
        cloth.update(dt);

    const auto start = std::chrono::steady_clock::now();
    for (int f = 0; f < frames; ++f)
        cloth.update(dt);
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count() / frames;
}

int main(int argc, char** argv)
{
    const int frames = argc > 1 ? std::atoi(argv[1]) : 120;
    unsigned threads = argc > 2 ? static_cast<unsigned>(std::atoi(argv[2])) : 0;
    if (threads == 0)
        threads = std::max(1u, std::thread::hardware_concurrency());

    std::printf("Frames per run: %d, threads: %u\n\n", frames, threads);
    std::printf("%-12s %-10s %-14s %-14s %-8s\n", "Grid", "Particles", "1 thread (ms)", "N threads (ms)", "Speedup");

    for (int resolution : {40, 80, 160})
    {
        const double serial = runScene(resolution, 1, frames);
        const double parallel = runScene(resolution, threads, frames);
        char grid[32];
        std::snprintf(grid, sizeof(grid), "%dx%d", resolution, resolution);
        std::printf("%-12s %-10d %-14.2f %-14.2f %.2fx\n",
                    grid, resolution * resolution, serial, parallel, serial / parallel);
    }
    return 0;
}
