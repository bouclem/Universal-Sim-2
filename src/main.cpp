#include "core/Window.h"
#include "core/Camera.h"
#include "core/Shader.h"
#include "rendering/LODSphere.h"
#include "rendering/RingMesh.h"
#include "rendering/TextRenderer.h"
#include "rendering/OrbitTrail.h"
#include "rendering/OrbitPath.h"
#include "rendering/AsteroidBelt.h"
#include "rendering/GalaxyRenderer.h"
#include "scene/Galaxy.h"
#include "scene/SolarSystem.h"
#include "physics/NBodySimulation.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <ctime>
#include <sstream>
#include <iomanip>

using namespace usim;

static Camera* g_camera = nullptr;
static bool g_firstMouse = true;
static double g_lastX = 640.0;
static double g_lastY = 360.0;

void mouseCallback(GLFWwindow*, double xpos, double ypos) {
    if (g_firstMouse) {
        g_lastX = xpos;
        g_lastY = ypos;
        g_firstMouse = false;
    }
    auto xOff = static_cast<float>(xpos - g_lastX);
    auto yOff = static_cast<float>(g_lastY - ypos);
    g_lastX = xpos;
    g_lastY = ypos;
    if (g_camera) g_camera->processMouseMovement(xOff, yOff);
}

void scrollCallback(GLFWwindow*, double, double yoffset) {
    if (g_camera) g_camera->processScroll(static_cast<float>(yoffset));
}

Mesh buildSkyboxMesh() {
    std::vector<Vertex> verts = {
        {{-1,-1,-1},{0,0,0}}, {{1,-1,-1},{0,0,0}},
        {{1,1,-1},{0,0,0}},   {{-1,1,-1},{0,0,0}},
        {{-1,-1,1},{0,0,0}},  {{1,-1,1},{0,0,0}},
        {{1,1,1},{0,0,0}},    {{-1,1,1},{0,0,0}},
    };
    std::vector<uint32_t> idx = {
        0,1,2, 2,3,0,  4,6,5, 6,4,7,
        0,3,7, 7,4,0,  1,5,6, 6,2,1,
        3,2,6, 6,7,3,  0,4,5, 5,1,0,
    };
    Mesh m;
    m.upload(verts, idx);
    return m;
}

std::string fmtFloat(float val, int precision = 1) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision) << val;
    return ss.str();
}

std::string scaleLabel(ViewScale s) {
    switch (s) {
        case ViewScale::Galaxy: return "GALAXY VIEW";
        case ViewScale::StarField: return "STAR FIELD";
        case ViewScale::SolarSystem: return "SOLAR SYSTEM";
    }
    return "";
}

int main() {
    try {
        // --- Init ---
        Window window(1280, 720, "Universal Sim 2 - v0.6.0");
        Camera camera(glm::vec3(0.0f, 20.0f, 100.0f));
        g_camera = &camera;

        glfwSetCursorPosCallback(window.handle(), mouseCallback);
        glfwSetScrollCallback(window.handle(), scrollCallback);

        // --- Shaders ---
        Shader starShader("shaders/star.vert", "shaders/star.frag");
        Shader planetShader("shaders/planet.vert", "shaders/planet.frag");
        Shader bgShader("shaders/background.vert", "shaders/background.frag");
        Shader ringShader("shaders/ring.vert", "shaders/ring.frag");
        Shader atmoShader("shaders/atmosphere.vert", "shaders/atmosphere.frag");

        // --- Meshes & renderers ---
        LODSphere lodSphere;
        RingMesh ringMesh;
        Mesh skybox = buildSkyboxMesh();
        TextRenderer textRenderer;
        OrbitTrail orbitTrail;
        OrbitPath orbitPath;
        GalaxyRenderer galaxyRenderer;

        // --- Generate galaxy ---
        Galaxy galaxy;
        uint32_t galaxySeed = static_cast<uint32_t>(std::time(nullptr));
        galaxy.generate(galaxySeed, 3000);
        galaxyRenderer.uploadStars(galaxy);

        // Start inside the nearest star system
        if (!galaxy.stars().empty()) {
            camera = Camera(galaxy.stars()[0].position + glm::vec3(0, 20, 100));
        }
        g_camera = &camera;
        glfwSetCursorPosCallback(window.handle(), mouseCallback);
        glfwSetScrollCallback(window.handle(), scrollCallback);

        // --- State ---
        float deltaTime = 0.0f;
        float lastFrame = 0.0f;
        float totalTime = 0.0f;
        float timeScale = 1.0f;
        bool paused = false;
        bool pKeyWasPressed = false;
        bool f11WasPressed = false;
        bool rKeyWasPressed = false;
        bool hKeyWasPressed = false;
        bool tKeyWasPressed = false;
        bool fKeyWasPressed = false;
        bool oKeyWasPressed = false;
        bool showHUD = true;
        bool showTrails = true;
        bool showOrbits = true;
        const CelestialBody* followTarget = nullptr;
        std::string followTargetName;
        ViewScale currentScale = ViewScale::SolarSystem;

        // Force initial system load
        galaxy.updateActiveSystem(camera.position());

        // --- Main loop ---
        while (!window.shouldClose()) {
            float currentFrame = static_cast<float>(glfwGetTime());
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;
            totalTime += deltaTime;

            // --- Determine viewing scale ---
            ViewScale newScale = galaxy.determineScale(camera.position());

            // Update speed scale based on viewing distance
            switch (newScale) {
                case ViewScale::Galaxy:
                    camera.setSpeedScale(100.0f);
                    camera.setFarPlane(500000.0f);
                    break;
                case ViewScale::StarField:
                    camera.setSpeedScale(20.0f);
                    camera.setFarPlane(200000.0f);
                    break;
                case ViewScale::SolarSystem:
                    camera.setSpeedScale(1.0f);
                    camera.setFarPlane(100000.0f);
                    break;
            }

            if (newScale != currentScale) {
                // Scale transition: clear follow if leaving solar system
                if (newScale != ViewScale::SolarSystem && camera.isFollowing()) {
                    camera.clearFollowTarget();
                    followTarget = nullptr;
                    followTargetName.clear();
                }
                currentScale = newScale;
            }

            // --- Input ---
            window.pollEvents();

            if (glfwGetKey(window.handle(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                if (camera.isFollowing()) {
                    camera.clearFollowTarget();
                    followTarget = nullptr;
                    followTargetName.clear();
                } else {
                    glfwSetWindowShouldClose(window.handle(), true);
                }
            }

            // Regenerate galaxy with R
            bool rKeyPressed = glfwGetKey(window.handle(), GLFW_KEY_R) == GLFW_PRESS;
            if (rKeyPressed && !rKeyWasPressed) {
                galaxySeed = static_cast<uint32_t>(std::time(nullptr))
                           ^ static_cast<uint32_t>(totalTime * 1000.0f);
                galaxy.generate(galaxySeed, 3000);
                galaxyRenderer.uploadStars(galaxy);
                camera.clearFollowTarget();
                followTarget = nullptr;
                followTargetName.clear();
                if (!galaxy.stars().empty()) {
                    camera = Camera(galaxy.stars()[0].position + glm::vec3(0, 20, 100));
                    g_camera = &camera;
                    glfwSetCursorPosCallback(window.handle(), mouseCallback);
                    glfwSetScrollCallback(window.handle(), scrollCallback);
                }
                galaxy.updateActiveSystem(camera.position());
                std::cout << "Galaxy regenerated (seed: " << galaxySeed << ")\n";
            }
            rKeyWasPressed = rKeyPressed;

            // Pause
            bool pKeyPressed = glfwGetKey(window.handle(), GLFW_KEY_P) == GLFW_PRESS;
            if (pKeyPressed && !pKeyWasPressed) paused = !paused;
            pKeyWasPressed = pKeyPressed;

            // Fullscreen
            bool f11Pressed = glfwGetKey(window.handle(), GLFW_KEY_F11) == GLFW_PRESS;
            if (f11Pressed && !f11WasPressed) window.toggleFullscreen();
            f11WasPressed = f11Pressed;

            // Toggle HUD
            bool hKeyPressed = glfwGetKey(window.handle(), GLFW_KEY_H) == GLFW_PRESS;
            if (hKeyPressed && !hKeyWasPressed) showHUD = !showHUD;
            hKeyWasPressed = hKeyPressed;

            // Toggle trails
            bool tKeyPressed = glfwGetKey(window.handle(), GLFW_KEY_T) == GLFW_PRESS;
            if (tKeyPressed && !tKeyWasPressed) showTrails = !showTrails;
            tKeyWasPressed = tKeyPressed;

            // Toggle orbits
            bool oKeyPressed = glfwGetKey(window.handle(), GLFW_KEY_O) == GLFW_PRESS;
            if (oKeyPressed && !oKeyWasPressed) showOrbits = !showOrbits;
            oKeyWasPressed = oKeyPressed;

            // Follow body (only in solar system view)
            bool fKeyPressed = glfwGetKey(window.handle(), GLFW_KEY_F) == GLFW_PRESS;
            if (fKeyPressed && !fKeyWasPressed) {
                if (camera.isFollowing()) {
                    camera.clearFollowTarget();
                    followTarget = nullptr;
                    followTargetName.clear();
                } else if (currentScale == ViewScale::SolarSystem && galaxy.activeSystem()) {
                    glm::mat4 tempView = camera.viewMatrix();
                    glm::vec3 camFwd = glm::normalize(
                        glm::vec3(glm::inverse(tempView) * glm::vec4(0,0,-1,0)));
                    const CelestialBody* sel =
                        galaxy.activeSystem()->findClosestToRay(camera.position(), camFwd);
                    if (sel) {
                        followTarget = sel;
                        followTargetName = sel->name;
                        camera.setFollowTarget(sel->position, sel->radius);
                    }
                }
            }
            fKeyWasPressed = fKeyPressed;

            // Time scale
            if (glfwGetKey(window.handle(), GLFW_KEY_EQUAL) == GLFW_PRESS)
                timeScale = std::min(timeScale + deltaTime * 2.0f, 20.0f);
            if (glfwGetKey(window.handle(), GLFW_KEY_MINUS) == GLFW_PRESS)
                timeScale = std::max(timeScale - deltaTime * 2.0f, 0.1f);

            camera.processKeyboard(window.handle(), deltaTime);

            // --- Update active system based on camera position ---
            galaxy.updateActiveSystem(camera.position());

            // --- Update follow camera ---
            if (!followTargetName.empty() && camera.isFollowing() && galaxy.activeSystem()) {
                const CelestialBody* found = nullptr;
                for (const auto* body : galaxy.activeSystem()->allBodies()) {
                    if (body->name == followTargetName) {
                        found = body;
                        break;
                    }
                }
                if (found) {
                    followTarget = found;
                    camera.updateFollow(found->position, deltaTime);
                } else {
                    camera.clearFollowTarget();
                    followTarget = nullptr;
                    followTargetName.clear();
                }
            }

            // --- Update physics ---
            if (!paused) {
                galaxy.updatePhysics(deltaTime, timeScale);
            }

            // --- Matrices ---
            glm::mat4 view = camera.viewMatrix();
            glm::mat4 projection = camera.projectionMatrix(window.aspectRatio());

            // --- Selection (solar system only) ---
            const CelestialBody* selected = nullptr;
            if (currentScale == ViewScale::SolarSystem && galaxy.activeSystem()) {
                glm::vec3 camFwd = glm::normalize(
                    glm::vec3(glm::inverse(view) * glm::vec4(0,0,-1,0)));
                selected = galaxy.activeSystem()->findClosestToRay(
                    camera.position(), camFwd);
            }

            // --- Render ---
            glClearColor(0.0f, 0.0f, 0.01f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Background starfield (always)
            glDepthFunc(GL_LEQUAL);
            bgShader.use();
            bgShader.setMat4("uView", view);
            bgShader.setMat4("uProjection", projection);
            skybox.draw();
            glDepthFunc(GL_LESS);

            // --- Galaxy-scale rendering ---
            if (currentScale == ViewScale::Galaxy) {
                // Draw galaxy dust cloud
                galaxyRenderer.drawDust(galaxy, view, projection);
            }

            // --- Star field rendering (galaxy + star field scales) ---
            if (currentScale == ViewScale::Galaxy || currentScale == ViewScale::StarField) {
                galaxyRenderer.drawStars(galaxy, camera.position(),
                                          currentScale, view, projection);
            }

            // --- Solar system rendering ---
            SolarSystem* sys = galaxy.activeSystem();
            if (currentScale == ViewScale::SolarSystem && sys) {
                // Orbit prediction lines
                if (showOrbits) {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glDepthMask(GL_FALSE);

                    for (const auto& planet : sys->planets()) {
                        glm::vec3 oColor(0.2f, 0.4f, 0.6f);
                        if (planet.planetType == 1) oColor = glm::vec3(0.6f, 0.4f, 0.2f);
                        if (planet.planetType == 2) oColor = glm::vec3(0.3f, 0.5f, 0.7f);

                        orbitPath.draw(planet, sys->star().position,
                                       sys->star().mass, NBodySimulation::G,
                                       oColor, view, projection);

                        for (const auto& moon : planet.moons) {
                            orbitPath.draw(moon, planet.position, planet.mass,
                                           NBodySimulation::G,
                                           glm::vec3(0.3f, 0.3f, 0.4f),
                                           view, projection);
                        }
                    }

                    glDepthMask(GL_TRUE);
                    glDisable(GL_BLEND);
                }

                // Orbit trails
                if (showTrails) {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glDepthMask(GL_FALSE);

                    for (const auto& planet : sys->planets()) {
                        if (planet.trail.size() >= 2) {
                            glm::vec3 tc(0.3f, 0.5f, 0.8f);
                            if (planet.planetType == 1) tc = glm::vec3(0.8f, 0.6f, 0.3f);
                            if (planet.planetType == 2) tc = glm::vec3(0.4f, 0.7f, 0.9f);
                            orbitTrail.draw(planet.trail, tc, view, projection);
                        }
                        for (const auto& moon : planet.moons) {
                            if (moon.trail.size() >= 2) {
                                orbitTrail.draw(moon.trail, glm::vec3(0.5f),
                                               view, projection);
                            }
                        }
                    }

                    glDepthMask(GL_TRUE);
                    glDisable(GL_BLEND);
                }

                // Solid bodies
                auto bodies = sys->allBodies();
                for (const auto* body : bodies) {
                    float dist = glm::length(camera.position() - body->position);
                    int lod = lodSphere.selectLOD(dist, body->radius, 60.0f,
                                                   window.height());

                    glm::mat4 model = glm::translate(glm::mat4(1.0f), body->position);
                    model = glm::scale(model, glm::vec3(body->radius));

                    // v0.6.0: compute rotation matrix (axial tilt + spin)
                    glm::mat4 tilt = glm::rotate(glm::mat4(1.0f), body->axialTilt,
                                                  glm::vec3(0.0f, 0.0f, 1.0f));
                    glm::mat4 spin = glm::rotate(glm::mat4(1.0f), body->rotationAngle,
                                                  glm::vec3(0.0f, 1.0f, 0.0f));
                    glm::mat3 rotation = glm::mat3(tilt * spin);

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
                        planetShader.setMat3("uRotation", rotation);
                        planetShader.setVec3("uStarPos", sys->star().position);
                        planetShader.setVec3("uStarColor", sys->star().starColor);
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

                // Asteroid belt
                AsteroidBelt* belt = galaxy.activeBelt();
                if (belt) {
                    belt->draw(sys->star().position, sys->star().starColor,
                               view, projection);
                }

                // Rings
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDisable(GL_CULL_FACE);

                for (const auto& planet : sys->planets()) {
                    if (!planet.rings.hasRings) continue;

                    ringShader.use();
                    glm::mat4 rm = glm::translate(glm::mat4(1.0f), planet.position);

                    ringShader.setMat4("uModel", rm);
                    ringShader.setMat4("uView", view);
                    ringShader.setMat4("uProjection", projection);
                    ringShader.setFloat("uInnerRadius",
                        planet.radius * planet.rings.innerRadius);
                    ringShader.setFloat("uOuterRadius",
                        planet.radius * planet.rings.outerRadius);
                    ringShader.setVec3("uRingColor", planet.rings.color);
                    ringShader.setFloat("uOpacity", planet.rings.opacity);
                    ringShader.setFloat("uNoiseSeed", planet.rings.noiseSeed);
                    ringShader.setVec3("uStarPos", sys->star().position);
                    ringShader.setVec3("uStarColor", sys->star().starColor);
                    ringShader.setVec3("uPlanetPos", planet.position);
                    ringShader.setFloat("uPlanetRadius", planet.radius);

                    ringMesh.mesh().draw();
                }

                // Atmospheres
                for (const auto* body : bodies) {
                    if (body->isStar || !body->atmosphere.hasAtmosphere) continue;

                    float atmoR = body->radius * (1.0f + body->atmosphere.thickness);
                    float dist = glm::length(camera.position() - body->position);
                    int lod = lodSphere.selectLOD(dist, atmoR, 60.0f, window.height());

                    glm::mat4 am = glm::translate(glm::mat4(1.0f), body->position);
                    am = glm::scale(am, glm::vec3(atmoR));

                    atmoShader.use();
                    atmoShader.setMat4("uModel", am);
                    atmoShader.setMat4("uView", view);
                    atmoShader.setMat4("uProjection", projection);
                    atmoShader.setVec3("uAtmosphereColor", body->atmosphere.color);
                    atmoShader.setFloat("uDensity", body->atmosphere.density);
                    atmoShader.setVec3("uCameraPos", camera.position());
                    atmoShader.setVec3("uStarPos", sys->star().position);
                    atmoShader.setVec3("uStarColor", sys->star().starColor);

                    lodSphere.mesh(lod).draw();
                }

                glDisable(GL_BLEND);
                glEnable(GL_CULL_FACE);
            }

            // --- HUD ---
            if (showHUD) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDisable(GL_DEPTH_TEST);

                int sw = window.width();
                int sh = window.height();
                float scale = 2.0f;
                float lineH = 18.0f;

                // Top-left
                textRenderer.renderText(
                    "Universal Sim 2  v0.6.0", 10, 10, scale,
                    glm::vec3(0.8f), sw, sh);
                textRenderer.renderText(
                    scaleLabel(currentScale), 10, 10 + lineH, scale,
                    glm::vec3(0.5f, 0.8f, 1.0f), sw, sh);
                textRenderer.renderText(
                    std::string("Speed: ") + fmtFloat(timeScale, 1) + "x"
                    + (paused ? "  [PAUSED]" : ""),
                    10, 10 + lineH * 2, scale,
                    glm::vec3(0.6f, 0.8f, 1.0f), sw, sh);

                // Star count / system info
                if (currentScale == ViewScale::SolarSystem && sys) {
                    auto bodies = sys->allBodies();
                    textRenderer.renderText(
                        "System: " + sys->star().name
                        + "  Bodies: " + std::to_string(bodies.size()),
                        10, 10 + lineH * 3, scale,
                        glm::vec3(0.6f, 0.8f, 1.0f), sw, sh);
                } else {
                    textRenderer.renderText(
                        "Stars: " + std::to_string(galaxy.stars().size()),
                        10, 10 + lineH * 3, scale,
                        glm::vec3(0.6f, 0.8f, 1.0f), sw, sh);
                }

                if (camera.isFollowing()) {
                    textRenderer.renderText(
                        "FOLLOWING: " + followTargetName,
                        10, 10 + lineH * 4, scale,
                        glm::vec3(1.0f, 0.8f, 0.3f), sw, sh);
                }

                // Selected body info (solar system only)
                if (selected && currentScale == ViewScale::SolarSystem) {
                    float dist = glm::length(camera.position() - selected->position);
                    float speed = glm::length(selected->velocity);

                    std::string typeStr;
                    if (selected->isStar) typeStr = "Star";
                    else if (selected->isMoon) typeStr = "Moon";
                    else {
                        switch (selected->planetType) {
                            case 0: typeStr = "Rocky Planet"; break;
                            case 1: typeStr = "Gas Giant"; break;
                            case 2: typeStr = "Ice Giant"; break;
                            default: typeStr = "Unknown"; break;
                        }
                    }

                    float baseY = static_cast<float>(sh) - 10 - lineH * 5;
                    textRenderer.renderText(selected->name, 10, baseY,
                        scale * 1.2f, glm::vec3(1.0f, 0.9f, 0.7f), sw, sh);
                    textRenderer.renderText(typeStr, 10, baseY + lineH * 1.2f,
                        scale, glm::vec3(0.7f), sw, sh);
                    textRenderer.renderText(
                        "Radius: " + fmtFloat(selected->radius, 2),
                        10, baseY + lineH * 2.2f, scale,
                        glm::vec3(0.6f, 0.8f, 1.0f), sw, sh);
                    textRenderer.renderText(
                        "Mass: " + fmtFloat(selected->mass, 1),
                        10, baseY + lineH * 3.2f, scale,
                        glm::vec3(0.6f, 0.8f, 1.0f), sw, sh);
                    textRenderer.renderText(
                        "Dist: " + fmtFloat(dist, 1) + "  Vel: " + fmtFloat(speed, 1),
                        10, baseY + lineH * 4.2f, scale,
                        glm::vec3(0.6f, 0.8f, 1.0f), sw, sh);

                    if (selected->isStar) {
                        textRenderer.renderText(
                            "Temp: " + fmtFloat(selected->temperature, 0) + " K",
                            10, baseY + lineH * 5.2f, scale,
                            glm::vec3(1.0f, 0.8f, 0.5f), sw, sh);
                    }
                }

                // Nearest star info (star field view)
                if (currentScale == ViewScale::StarField) {
                    int nearest = galaxy.findNearestStar(camera.position());
                    if (nearest >= 0) {
                        const auto& ns = galaxy.stars()[static_cast<size_t>(nearest)];
                        float dist = glm::length(camera.position() - ns.position);
                        float baseY = static_cast<float>(sh) - 10 - lineH * 3;
                        textRenderer.renderText(
                            "Nearest: " + ns.name, 10, baseY,
                            scale * 1.2f, glm::vec3(1.0f, 0.9f, 0.7f), sw, sh);
                        textRenderer.renderText(
                            "Dist: " + fmtFloat(dist, 0)
                            + (ns.isBinary ? "  [BINARY]" : ""),
                            10, baseY + lineH * 1.2f, scale,
                            glm::vec3(0.6f, 0.8f, 1.0f), sw, sh);
                        textRenderer.renderText(
                            "Temp: " + fmtFloat(ns.temperature, 0) + " K",
                            10, baseY + lineH * 2.2f, scale,
                            glm::vec3(1.0f, 0.8f, 0.5f), sw, sh);
                    }
                }

                // Controls
                float rightX = static_cast<float>(sw) - 300;
                textRenderer.renderText("H: Toggle HUD", rightX, 10, scale,
                    glm::vec3(0.5f), sw, sh);
                textRenderer.renderText("T: Toggle Trails", rightX, 10 + lineH, scale,
                    glm::vec3(0.5f), sw, sh);
                textRenderer.renderText("O: Toggle Orbits", rightX, 10 + lineH * 2, scale,
                    glm::vec3(0.5f), sw, sh);
                textRenderer.renderText("F: Follow Body", rightX, 10 + lineH * 3, scale,
                    glm::vec3(0.5f), sw, sh);
                textRenderer.renderText("P: Pause  +/-: Speed", rightX, 10 + lineH * 4, scale,
                    glm::vec3(0.5f), sw, sh);
                textRenderer.renderText("R: Regenerate Galaxy", rightX, 10 + lineH * 5, scale,
                    glm::vec3(0.5f), sw, sh);

                // Crosshair
                textRenderer.renderText("+",
                    static_cast<float>(sw) / 2.0f - 4.0f * scale,
                    static_cast<float>(sh) / 2.0f - 4.0f * scale,
                    scale, glm::vec3(1.0f), sw, sh);

                glEnable(GL_DEPTH_TEST);
                glDisable(GL_BLEND);
            }

            window.swapBuffers();
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
