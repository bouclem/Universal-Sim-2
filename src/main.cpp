#include "core/Window.h"
#include "core/Camera.h"
#include "core/Shader.h"
#include "rendering/LODSphere.h"
#include "rendering/RingMesh.h"
#include "scene/SolarSystem.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
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
    auto yOffset = static_cast<float>(g_lastY - ypos);
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
    std::vector<Vertex> vertices = {
        {{-1, -1, -1}, {0,0,0}}, {{ 1, -1, -1}, {0,0,0}},
        {{ 1,  1, -1}, {0,0,0}}, {{-1,  1, -1}, {0,0,0}},
        {{-1, -1,  1}, {0,0,0}}, {{ 1, -1,  1}, {0,0,0}},
        {{ 1,  1,  1}, {0,0,0}}, {{-1,  1,  1}, {0,0,0}},
    };
    std::vector<uint32_t> indices = {
        0,1,2, 2,3,0,
        4,6,5, 6,4,7,
        0,3,7, 7,4,0,
        1,5,6, 6,2,1,
        3,2,6, 6,7,3,
        0,4,5, 5,1,0,
    };
    Mesh mesh;
    mesh.upload(vertices, indices);
    return mesh;
}

int main() {
    try {
        // --- Init ---
        Window window(1280, 720, "Universal Sim 2 - v0.2.0");
        Camera camera(glm::vec3(0.0f, 10.0f, 60.0f));
        g_camera = &camera;

        glfwSetCursorPosCallback(window.handle(), mouseCallback);
        glfwSetScrollCallback(window.handle(), scrollCallback);

        // --- Shaders ---
        Shader starShader("shaders/star.vert", "shaders/star.frag");
        Shader planetShader("shaders/planet.vert", "shaders/planet.frag");
        Shader bgShader("shaders/background.vert", "shaders/background.frag");
        Shader ringShader("shaders/ring.vert", "shaders/ring.frag");
        Shader atmoShader("shaders/atmosphere.vert", "shaders/atmosphere.frag");

        // --- Meshes ---
        LODSphere lodSphere;
        RingMesh ringMesh;
        Mesh skybox = buildSkyboxMesh();

        // --- Generate solar system ---
        SolarSystem solarSystem;
        uint32_t seed = static_cast<uint32_t>(std::time(nullptr));
        solarSystem.generate(seed);

        std::cout << "Solar system generated (seed: " << seed << ")\n";
        std::cout << "Star temperature: " << solarSystem.star().temperature << " K\n";
        std::cout << "Planets: " << solarSystem.planets().size() << "\n";
        for (size_t i = 0; i < solarSystem.planets().size(); ++i) {
            const auto& p = solarSystem.planets()[i];
            std::cout << "  Planet " << i << ": "
                      << (p.planetType == 0 ? "Rocky" : p.planetType == 1 ? "Gas Giant" : "Ice Giant")
                      << ", moons=" << p.moons.size()
                      << ", rings=" << (p.rings.hasRings ? "yes" : "no")
                      << ", atmo=" << (p.atmosphere.hasAtmosphere ? "yes" : "no")
                      << "\n";
        }

        // --- Timing ---
        float deltaTime = 0.0f;
        float lastFrame = 0.0f;
        float totalTime = 0.0f;
        float timeScale = 1.0f;
        bool paused = false;
        bool pKeyWasPressed = false;
        bool f11WasPressed = false;

        // --- Main loop ---
        while (!window.shouldClose()) {
            float currentFrame = static_cast<float>(glfwGetTime());
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;
            totalTime += deltaTime;

            // --- Input ---
            window.pollEvents();

            if (glfwGetKey(window.handle(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window.handle(), true);

            // Regenerate with R
            if (glfwGetKey(window.handle(), GLFW_KEY_R) == GLFW_PRESS) {
                seed = static_cast<uint32_t>(std::time(nullptr))
                     ^ static_cast<uint32_t>(totalTime * 1000.0f);
                solarSystem.generate(seed);
                std::cout << "Regenerated (seed: " << seed << ")\n";
            }

            // Pause/unpause with P (toggle)
            bool pKeyPressed = glfwGetKey(window.handle(), GLFW_KEY_P) == GLFW_PRESS;
            if (pKeyPressed && !pKeyWasPressed) {
                paused = !paused;
                std::cout << (paused ? "Paused" : "Resumed") << "\n";
            }
            pKeyWasPressed = pKeyPressed;

            // Fullscreen toggle with F11
            bool f11Pressed = glfwGetKey(window.handle(), GLFW_KEY_F11) == GLFW_PRESS;
            if (f11Pressed && !f11WasPressed) {
                window.toggleFullscreen();
            }
            f11WasPressed = f11Pressed;

            // Time scale: + / - keys
            if (glfwGetKey(window.handle(), GLFW_KEY_EQUAL) == GLFW_PRESS)
                timeScale = std::min(timeScale + deltaTime * 2.0f, 20.0f);
            if (glfwGetKey(window.handle(), GLFW_KEY_MINUS) == GLFW_PRESS)
                timeScale = std::max(timeScale - deltaTime * 2.0f, 0.1f);

            camera.processKeyboard(window.handle(), deltaTime);

            // --- Update orbits ---
            if (!paused) {
                solarSystem.update(deltaTime, timeScale);
            }

            // --- Render ---
            glClearColor(0.0f, 0.0f, 0.01f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::mat4 view = camera.viewMatrix();
            glm::mat4 projection = camera.projectionMatrix(window.aspectRatio());

            // Background starfield
            glDepthFunc(GL_LEQUAL);
            bgShader.use();
            bgShader.setMat4("uView", view);
            bgShader.setMat4("uProjection", projection);
            skybox.draw();
            glDepthFunc(GL_LESS);

            // --- Render all solid bodies (star + planets + moons) ---
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

            // --- Render rings (alpha blended, after solid geometry) ---
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_CULL_FACE);

            for (const auto& planet : solarSystem.planets()) {
                if (!planet.rings.hasRings) continue;

                ringShader.use();

                // Model: translate to planet, no extra scale (radii in shader)
                glm::mat4 ringModel = glm::translate(glm::mat4(1.0f), planet.position);

                ringShader.setMat4("uModel", ringModel);
                ringShader.setMat4("uView", view);
                ringShader.setMat4("uProjection", projection);
                ringShader.setFloat("uInnerRadius",
                    planet.radius * planet.rings.innerRadius);
                ringShader.setFloat("uOuterRadius",
                    planet.radius * planet.rings.outerRadius);
                ringShader.setVec3("uRingColor", planet.rings.color);
                ringShader.setFloat("uOpacity", planet.rings.opacity);
                ringShader.setFloat("uNoiseSeed", planet.rings.noiseSeed);
                ringShader.setVec3("uStarPos", solarSystem.star().position);
                ringShader.setVec3("uStarColor", solarSystem.star().starColor);
                ringShader.setVec3("uPlanetPos", planet.position);
                ringShader.setFloat("uPlanetRadius", planet.radius);

                ringMesh.mesh().draw();
            }

            // --- Render atmospheres (alpha blended shells) ---
            for (const auto* body : bodies) {
                if (body->isStar || !body->atmosphere.hasAtmosphere) continue;

                float atmoRadius = body->radius * (1.0f + body->atmosphere.thickness);
                float dist = glm::length(camera.position() - body->position);
                int lod = lodSphere.selectLOD(dist, atmoRadius, 60.0f,
                                               window.height());

                glm::mat4 atmoModel = glm::translate(glm::mat4(1.0f), body->position);
                atmoModel = glm::scale(atmoModel, glm::vec3(atmoRadius));

                atmoShader.use();
                atmoShader.setMat4("uModel", atmoModel);
                atmoShader.setMat4("uView", view);
                atmoShader.setMat4("uProjection", projection);
                atmoShader.setVec3("uAtmosphereColor", body->atmosphere.color);
                atmoShader.setFloat("uDensity", body->atmosphere.density);
                atmoShader.setVec3("uCameraPos", camera.position());
                atmoShader.setVec3("uStarPos", solarSystem.star().position);
                atmoShader.setVec3("uStarColor", solarSystem.star().starColor);

                lodSphere.mesh(lod).draw();
            }

            glDisable(GL_BLEND);
            glEnable(GL_CULL_FACE);

            window.swapBuffers();
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
