#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <algorithm>

#include "../header/shader.h"
#include "../header/cloth.h"

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    // macOS only grants a 3.3 core context if forward compatibility is requested
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(800, 600, "ClothSim", nullptr, nullptr);
    if (!window) {
        std::cout << "Failed to create window\n";
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Load OpenGL functions (IMPORTANT)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to init GLAD\n";
        return -1;
    }

    std::vector<glm::vec3> positions;

    // cloth
    Cloth cloth;
    
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);
    
    GLuint VAO, VBO;
    std::vector<glm::vec3> particlePositions = cloth.getPositions();
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, particlePositions.size()*sizeof(glm::vec3), particlePositions.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    // CMake copies shaders/ next to the executable at build time
    Shader clothShader("shaders/cloth.vert", "shaders/cloth.frag");
    GLint mvpLocation = glGetUniformLocation(clothShader.id(), "uMVP");

    // camera position: move further back along z to zoom out
    glm::vec3 cameraPos(0, 0, 15);

    float last = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        clothShader.use();

        // re-read the size each frame so the aspect ratio survives a resize
        glfwGetFramebufferSize(window, &w, &h);
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)w/(float)h, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, glm::value_ptr(proj * view));

        float now = glfwGetTime();
        float dt = now - last;
        dt = std::min(dt, 1.0f/30.0f); // clamp so that dt does not explode
        last = now;
        cloth.update(dt);

        auto positions = cloth.getPositions();
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size()*sizeof(glm::vec3), positions.data());
        glBindVertexArray(VAO);
        glPointSize(5.0f);
        glDrawArrays(GL_POINTS, 0, positions.size());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}