#include "core/Window.h"
#include "core/Camera.h"
#include "core/Shader.h"
#include "rendering/LODSphere.h"
#include "scene/SolarSystem.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <random>
#include <ctime>

using namespace usim;

// Globals for mouse callback
static Camera* g_camera = nullptr;
static bool g_firstMouse = true;
static double g_lastX = 640.0;
static double g_lastY = 360.0;

void mouseCallback(GLFWwindow* /*window*/, double xpos, double ypos) {
    if (g_firstMouse) {
        g_lastX = xpos;
        g_lastY = ypos;
        g_firstMouse = false;
    }

    auto xOffset = static_cast<float>(xpos - g_lastX);
    auto yOffset = static_cast<float>(g_lastY - ypos); // Reversed: y goes bottom-to-top
    g_lastX = xpos;
    g_lastY = ypos;

    if (g_camera) {
        g_camera->processMouseMovement(xOffset, yOffset);
    }
}

void scrollCallback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset) {
    if (g_camera) {
        g_camera->processScroll(static_cast<float>(yoffset));
    }
}

/// Build a simple cube mesh for the background skybox.
Mesh buildSkyboxMesh() {
    // Large cube centered at origin
    std::vector<Vertex> vertices = {
        {{-1, -1, -1}, {0,0,0}}, {{ 1, -1, -1}, {0,0,0}},
        {{ 1,  1, -1}, {0,0,0}}, {{-1,  1, -1}, {0,0,0}},
        {{-1, -1,  1}, {0,0,0}}, {{ 1, -1,  1}, {0,0,0}},
        {{ 1,  1,  1}, {0,0,0}}, {{-1,  1,  1}, {0,0,0}},
    };
    std::vector<uint32_t> indices = {
        0,1,2, 2,3,0, // back
        4,6,5, 6,4,7, // front
        0,3,7, 7,4,0, // left
        1,5,6, 6,2,1, // right
        3,2,6, 6,7,3, // top
        0,4,5, 5,1,0, // bottom
    };
    Mesh mesh;
    mesh.upload(vertices, indices);
    return mesh;
}

int main() {
    try {
        // --- Init ---
        Window window(1280, 720, "Universal Sim 2 - v0.1.0");
        Camera camera(glm::vec3(0.0f, 10.0f, 60.0f));
        g_camera = &camera;

        glfwSetCursorPosCallback(window.handle(), mouseCallback);
        glfwSetScrollCallback(window.handle(), scrollCallback);

        // --- Shaders ---
        Shader starShader("shaders/star.vert", "shaders/star.frag");
        Shader planetShader("shaders/planet.vert", "shaders/planet.frag");
        Shader bgShader("shaders/background.vert", "shaders/background.frag");

        // --- LOD sphere ---
        LODSphere lodSphere;

        // --- Background skybox ---
        Mesh skybox = buildSkyboxMesh();

        // --- Generate solar system ---
        SolarSystem solarSystem;
        uint32_t seed = static_cast<uint32_t>(std::time(nullptr));
        solarSystem.generate(seed);

        std::cout << "Solar system generated (seed: " << seed << ")\n";
        std::cout << "Star temperature: " << solarSystem.star().temperature << " K\n";
        std::cout << "Planets: " << solarSystem.planets().size() << "\n";

        // --- Timing ---
        float deltaTime = 0.0f;
        float lastFrame = 0.0f;
        float totalTime = 0.0f;

        // --- Main loop ---
        while (!window.shouldClose()) {
            float currentFrame = static_cast<float>(glfwGetTime());
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;
            totalTime += deltaTime;

            // Input
            window.pollEvents();
            if (glfwGetKey(window.handle(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window.handle(), true);
            }

            // Regenerate system with R key
            if (glfwGetKey(window.handle(), GLFW_KEY_R) == GLFW_PRESS) {
                seed = static_cast<uint32_t>(std::time(nullptr)) ^ static_cast<uint32_t>(totalTime * 1000.0f);
                solarSystem.generate(seed);
                std::cout << "Regenerated (seed: " << seed << ")\n";
            }

            camera.processKeyboard(window.handle(), deltaTime);

            // --- Render ---
            glClearColor(0.0f, 0.0f, 0.01f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::mat4 view = camera.viewMatrix();
            glm::mat4 projection = camera.projectionMatrix(window.aspectRatio());

            // Background starfield (render first, depth test <=)
            glDepthFunc(GL_LEQUAL);
            bgShader.use();
            bgShader.setMat4("uView", view);
            bgShader.setMat4("uProjection", projection);
            skybox.draw();
            glDepthFunc(GL_LESS);

            // Render all celestial bodies
            auto bodies = solarSystem.allBodies();
            for (const auto* body : bodies) {
                float dist = glm::length(camera.position() - body->position);
                int lod = lodSphere.selectLOD(dist, body->radius, 60.0f,
                                               window.height());

                glm::mat4 model = glm::translate(glm::mat4(1.0f), body->position);
                model = glm::scale(model, glm::vec3(body->radius));

                if (body->isStar) {
                    starShader.use();
                    starShader.setMat4("uModel", model);
                    starShader.setMat4("uView", view);
                    starShader.setMat4("uProjection", projection);
                    starShader.setVec3("uStarColor", body->starColor);
                    starShader.setVec3("uCameraPos", camera.position());
                    starShader.setFloat("uTime", totalTime);
                } else {
                    planetShader.use();
                    planetShader.setMat4("uModel", model);
                    planetShader.setMat4("uView", view);
                    planetShader.setMat4("uProjection", projection);
                    planetShader.setVec3("uStarPos", solarSystem.star().position);
                    planetShader.setVec3("uStarColor", solarSystem.star().starColor);
                    planetShader.setVec3("uCameraPos", camera.position());
                    planetShader.setVec3("uColorPrimary", body->colorPrimary);
                    planetShader.setVec3("uColorSecondary", body->colorSecondary);
                    planetShader.setVec3("uColorAccent", body->colorAccent);
                    planetShader.setFloat("uNoiseScale", body->noiseScale);
                    planetShader.setFloat("uNoiseSeed", body->noiseSeed);
                    planetShader.setInt("uPlanetType", body->planetType);
                }

                lodSphere.mesh(lod).draw();
            }

            window.swapBuffers();
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
