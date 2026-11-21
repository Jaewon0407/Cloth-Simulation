#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>

#include "../header/shader.h"
#include "../header/cloth.h"

static const glm::vec3 kSphereStart(-0.3f, 0.0f, 0.1f);

static void setupScene(Cloth& cloth)
{
    CollisionWorld& world = cloth.colliders();
    world.spheres = { SphereCollider{kSphereStart, 0.45f} };
    world.boxes = { BoxCollider{glm::vec3(0.6f, -1.1f, 0.3f), glm::vec3(0.35f, 0.4f, 0.35f)} };
    world.ground = GroundCollider{true, -1.5f};
}

static void appendCircle(std::vector<glm::vec3>& lines, const glm::vec3& center, float radius,
                         const glm::vec3& u, const glm::vec3& v)
{
    const int segments = 48;
    for (int k = 0; k < segments; ++k) {
        float a0 = glm::two_pi<float>() * k / segments;
        float a1 = glm::two_pi<float>() * (k + 1) / segments;
        lines.push_back(center + radius * (std::cos(a0) * u + std::sin(a0) * v));
        lines.push_back(center + radius * (std::cos(a1) * u + std::sin(a1) * v));
    }
}

static std::vector<glm::vec3> buildColliderLines(const CollisionWorld& world)
{
    std::vector<glm::vec3> lines;
    const glm::vec3 X(1, 0, 0), Y(0, 1, 0), Z(0, 0, 1);

    for (const auto& sphere : world.spheres) {
        appendCircle(lines, sphere.center, sphere.radius, X, Y);
        appendCircle(lines, sphere.center, sphere.radius, X, Z);
        appendCircle(lines, sphere.center, sphere.radius, Y, Z);
    }

    for (const auto& box : world.boxes) {
        glm::vec3 corners[8];
        for (int k = 0; k < 8; ++k) {
            glm::vec3 sign((k & 1) ? 1.0f : -1.0f, (k & 2) ? 1.0f : -1.0f, (k & 4) ? 1.0f : -1.0f);
            corners[k] = box.center + sign * box.halfExtents;
        }
        for (int k = 0; k < 8; ++k) {
            for (int bit : {1, 2, 4}) {
                if (!(k & bit)) {
                    lines.push_back(corners[k]);
                    lines.push_back(corners[k | bit]);
                }
            }
        }
    }

    if (world.ground.enabled) {
        const float y = world.ground.height;
        for (float t = -3.0f; t <= 3.001f; t += 0.5f) {
            lines.push_back(glm::vec3(t, y, -3.0f));
            lines.push_back(glm::vec3(t, y, 3.0f));
            lines.push_back(glm::vec3(-3.0f, y, t));
            lines.push_back(glm::vec3(3.0f, y, t));
        }
    }
    return lines;
}

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
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);

    // Load OpenGL functions (IMPORTANT)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to init GLAD\n";
        return -1;
    }

    std::vector<glm::vec3> positions;

    // cloth
    Cloth cloth;
    setupScene(cloth);
    
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);
    
    GLuint VAO, VBO, EBO;
    std::vector<glm::vec3> particlePositions = cloth.getPositions();
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, particlePositions.size()*sizeof(glm::vec3), particlePositions.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    std::vector<GLuint> lineIndices;
    auto uploadLineIndices = [&]() {
        lineIndices.clear();
        for (const auto& c : cloth.constraints()) {
            if (c.type != ConstraintType::Structural)
                continue;
            lineIndices.push_back(c.a);
            lineIndices.push_back(c.b);
        }
        glBindVertexArray(VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, lineIndices.size()*sizeof(GLuint), lineIndices.data(), GL_DYNAMIC_DRAW);
    };
    uploadLineIndices();

    GLuint debugVAO, debugVBO;
    glGenVertexArrays(1, &debugVAO);
    glGenBuffers(1, &debugVBO);
    glBindVertexArray(debugVAO);
    glBindBuffer(GL_ARRAY_BUFFER, debugVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    // CMake copies shaders/ next to the executable at build time
    Shader clothShader("shaders/cloth.vert", "shaders/cloth.frag");
    GLint mvpLocation = glGetUniformLocation(clothShader.id(), "uMVP");
    GLint colorLocation = glGetUniformLocation(clothShader.id(), "uColor");

    glm::vec3 cameraPos(3.2f, 1.2f, 4.2f);
    glm::vec3 cameraTarget(0.0f, -0.4f, -0.2f);

    bool spaceWasDown = false;
    bool resetWasDown = false;

    glEnable(GL_DEPTH_TEST);

    float last = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        clothShader.use();

        // re-read the size each frame so the aspect ratio survives a resize
        glfwGetFramebufferSize(window, &w, &h);
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)w/(float)h, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, glm::vec3(0, 1, 0));
        glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, glm::value_ptr(proj * view));

        float now = glfwGetTime();
        float dt = now - last;
        dt = std::min(dt, 1.0f/30.0f); // clamp so that dt does not explode
        last = now;

        bool spaceDown = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (spaceDown && !spaceWasDown)
            cloth.releasePins();
        spaceWasDown = spaceDown;

        bool resetDown = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
        if (resetDown && !resetWasDown) {
            cloth.reset();
            setupScene(cloth);
            uploadLineIndices();
        }
        resetWasDown = resetDown;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glm::vec3 move(0.0f);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) move.x -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) move.x += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) move.z -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) move.z += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) move.y += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) move.y -= 1.0f;
        if (!cloth.colliders().spheres.empty())
            cloth.colliders().spheres[0].center += move * 1.5f * dt;

        cloth.update(dt);

        auto positions = cloth.getPositions();
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size()*sizeof(glm::vec3), positions.data());
        glBindVertexArray(VAO);
        glUniform3f(colorLocation, 0.9f, 0.9f, 0.9f);
        glDrawElements(GL_LINES, (GLsizei)lineIndices.size(), GL_UNSIGNED_INT, nullptr);

        std::vector<glm::vec3> colliderLines = buildColliderLines(cloth.colliders());
        glBindVertexArray(debugVAO);
        glBindBuffer(GL_ARRAY_BUFFER, debugVBO);
        glBufferData(GL_ARRAY_BUFFER, colliderLines.size()*sizeof(glm::vec3), colliderLines.data(), GL_DYNAMIC_DRAW);
        glUniform3f(colorLocation, 0.35f, 0.6f, 0.95f);
        glDrawArrays(GL_LINES, 0, (GLsizei)colliderLines.size());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}