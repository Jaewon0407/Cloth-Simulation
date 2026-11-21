# Cloth Simulation

A real-time cloth simulation in C++ with OpenGL 3.3.

- **XPBD solver** — Extended Position Based Dynamics with substepping, using structural, shear and bend distance constraints
- **Collisions** — sphere, box and ground colliders with friction
- **Self-collision** — spatial hash grid with neighbor lists cached across substeps
- **Tearing** — constraints break when stretched past a set ratio of their rest length
- **Multithreading** — custom thread pool; constraints are graph-colored so each color is solved in parallel without data races
- **Rendering** — OpenGL + GLM math

## Controls

| Key | Action |
| --- | --- |
| Space | Release the pinned edge so the cloth falls |
| T (hold) | Pull the pinned edge apart to tear the cloth |
| W / A / S / D | Move the sphere |
| Q / E | Move the sphere down / up |
| R | Reset |
| Esc | Quit |

## Dependencies

- CMake 3.16+
- A C++20 compiler
- GLFW 3
- OpenGL (system-provided)

GLM and GLAD are vendored in `include/` — GLM as a git submodule, so clone recursively.

### macOS

```bash
brew install cmake glfw
```

### Linux (Debian/Ubuntu)

```bash
sudo apt install cmake build-essential libglfw3-dev
```

## Build & run

```bash
git clone --recurse-submodules git@github.com:Jaewon0407/Cloth-Simulation.git
cd Cloth-Simulation
cmake -S . -B build
cmake --build build
cd build && ./ClothSimulation
```

If you already cloned without `--recurse-submodules`:

```bash
git submodule update --init --recursive
```

The build copies `shaders/` next to the executable, so run the binary from the
directory that contains it (`build/`).

## Benchmark

`ClothBenchmark` runs the physics without a window and compares one thread against all cores:

```bash
cd build && ./ClothBenchmark [frames] [threads]
```
