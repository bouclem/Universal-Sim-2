#pragma once

#include "scene/SolarSystem.h"
#include "rendering/AsteroidBelt.h"
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>

namespace usim {

/// Viewing scale determines what gets rendered.
enum class ViewScale {
    Galaxy,      // Far out: see the whole galaxy shape
    StarField,   // Medium: see individual stars as points/spheres
    SolarSystem  // Close: inside a star system, full detail
};

/// Minimal data for a star in the galaxy (before generating its system).
struct GalaxyStar {
    glm::vec3 position;       // Galaxy-space position
    glm::vec3 color;          // Blackbody color
    float temperature;
    float luminosity;          // Affects apparent brightness
    float radius;              // Visual radius at star-field scale
    uint32_t seed;             // Seed for generating its solar system
    bool isBinary = false;     // Has a companion star
    std::string name;
};

/// A procedural spiral galaxy containing thousands of stars.
/// Only the nearest star's solar system is fully generated.
class Galaxy {
public:
    /// Generate the galaxy from a master seed.
    void generate(uint32_t seed, int starCount = 3000);

    /// Determine the current viewing scale based on camera position.
    ViewScale determineScale(const glm::vec3& cameraPos) const;

    /// Get the index of the nearest star to a position.
    /// Returns -1 if none found.
    int findNearestStar(const glm::vec3& pos) const;

    /// Get the active solar system (the one the camera is inside).
    /// Returns nullptr if not inside any system.
    SolarSystem* activeSystem() { return m_activeSystem.get(); }
    const SolarSystem* activeSystem() const { return m_activeSystem.get(); }

    /// Get the active asteroid belt.
    AsteroidBelt* activeBelt() { return m_activeBelt.get(); }

    /// Update: check if we need to load/unload a solar system.
    /// Returns true if the active system changed.
    bool updateActiveSystem(const glm::vec3& cameraPos);

    /// Update physics for the active system.
    void updatePhysics(float deltaTime, float timeScale);

    /// Access star data.
    const std::vector<GalaxyStar>& stars() const { return m_stars; }
    int activeStarIndex() const { return m_activeStarIndex; }
    const GalaxyStar* activeStar() const;

    /// Galaxy properties.
    glm::vec3 center() const { return m_center; }
    float radius() const { return m_radius; }
    uint32_t seed() const { return m_seed; }

    /// Threshold distances for scale transitions (v0.6.0: wider systems).
    static constexpr float SYSTEM_ENTER_DIST = 400.0f;   // Enter solar system view
    static constexpr float SYSTEM_EXIT_DIST = 500.0f;    // Exit solar system view
    static constexpr float GALAXY_VIEW_DIST = 8000.0f;   // Switch to galaxy overview

private:
    void generateSpiralArm(uint32_t armSeed, float armAngle, int count,
                           float galaxyRadius);

    uint32_t m_seed = 0;
    glm::vec3 m_center = glm::vec3(0.0f);
    float m_radius = 5000.0f;

    std::vector<GalaxyStar> m_stars;

    // Active solar system (loaded on demand)
    int m_activeStarIndex = -1;
    std::unique_ptr<SolarSystem> m_activeSystem;
    std::unique_ptr<AsteroidBelt> m_activeBelt;
};

} // namespace usim
