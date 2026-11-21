# declares minimum CMAKE version script needs
cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# names the project
project(ClothSimulation)

set(CMAKE_CXX_STANDARD 20)

find_package(Threads REQUIRED)

add_library(ClothPhysics STATIC src/cloth.cpp src/spatial_hash.cpp src/thread_pool.cpp)
target_include_directories(ClothPhysics PUBLIC include include/glm)
set_target_properties(ClothPhysics PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(ClothPhysics PUBLIC Threads::Threads)

add_executable(ClothSimulation src/main.cpp src/glad.c src/shader.cpp)

# Tell compiler where to find headers
target_include_directories(ClothSimulation PRIVATE include include/glm)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find library
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

# glm is header-only and vendored in include/glm (git submodule), so no
# find_package/link is needed for it.

# link libraries into the executable
target_link_libraries(ClothSimulation PRIVATE ClothPhysics glfw OpenGL::GL)

# Copy the shaders next to the executable so the app can be run from any
# directory, not just build/.
add_custom_command(TARGET ClothSimulation POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders
            $<TARGET_FILE_DIR:ClothSimulation>/shaders)
add_executable(ClothBenchmark bench/bench.cpp)
target_link_libraries(ClothBenchmark PRIVATE ClothPhysics)
