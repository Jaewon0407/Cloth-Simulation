# Cloth Simulation

A real-time cloth simulation in C++ with OpenGL 3.3.

- **Physics System** — particle integration
- **Constraint System** — springs holding the cloth's shape
- **Rendering System** — OpenGL + GLM math

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
