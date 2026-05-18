cmake_minimum_required(VERSION 3.16)
project(ClothSimulation)

set(CMAKE_CXX_STANDARD 20)

add_executable(ClothSimulation src/main.cpp src/glad.c)

# Tell compiler where to find headers
target_include_directories(ClothSimulation PRIVATE include)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

target_link_libraries(ClothSimulation PRIVATE glfw OpenGL::GL)