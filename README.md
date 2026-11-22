# Cloth Simulation

<p align="center">
  <img src="images/cloth_demo.gif" alt="Cloth falling onto a sphere and box, being pushed by the sphere, and tearing" width="720">
</p>

<p align="center">
  <img src="images/drape.png" alt="Cloth draped over the sphere and box colliders" width="32%">
  <img src="images/sphere_push.png" alt="Sphere pushing into the hanging cloth" width="32%">
  <img src="images/tearing.png" alt="Cloth tearing as the pinned edge is pulled apart" width="32%">
</p>
<p align="center"><em>Left to right: draped over the colliders, pushed by the sphere, torn apart.</em></p>

## Features

### Physics
- XPBD solver with 15 substeps per frame
- Structural, shear and bend constraints: 9,200+ at the default 40×40 size
- Gravity and damping

### Collisions
- Sphere, box and ground colliders, with friction
- Self-collision using a spatial hash grid, so the cloth doesn't pass through itself

### Tearing
- Constraints break when stretched past 1.6× their rest length

### Performance
- Custom thread pool for multithreading
- Graph coloring that splits constraints into 13 groups so they can be solved in parallel safely
- Headless benchmark tool that compares 1 thread against all cores at 1,600, 6,400 and 25,600 particles

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
