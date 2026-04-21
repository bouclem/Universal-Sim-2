#pragma once

#include "scene/CelestialBody.h"
#include "physics/NBodySimulation.h"
#include <vector>
#include <cstdint>

namespace usim {

/// Generates and holds a complete procedural solar system.
class SolarSystem {
public:
    /// Generate a solar system from a seed.
    /// If binary is true, generates a binary star system.
    void generate(uint32_t seed, bool binary = false);

    /// Update using n-body physics simulation.
    /// timeScale: 1.0 = normal, 0 = paused, >1 = fast forward
    void update(float deltaTime, float timeScale = 1.0f);

    /// Record current positions into orbit trails.
    void recordTrails();

    /// Check for collisions and merge bodies.
    /// Returns the number of collisions that occurred.
    int handleCollisions();

    CelestialBody& star() { return m_star; }
    const CelestialBody& star() const { return m_star; }
    bool isBinary() const { return m_isBinary; }
    CelestialBody& companionStar() { return m_companionStar; }
    const CelestialBody& companionStar() const { return m_companionStar; }
    const std::vector<CelestialBody>& planets() const { return m_planets; }
    std::vector<CelestialBody>& planets() { return m_planets; }

    /// Get all bodies (star + planets + moons) for rendering.
    std::vector<const CelestialBody*> allBodies() const;

    /// Get mutable pointers to all bodies (for physics).
    std::vector<CelestialBody*> allBodiesMut();

    /// Find the body closest to a world-space ray (for selection).
    const CelestialBody* findClosestToRay(const glm::vec3& origin,
                                           const glm::vec3& direction) const;

    /// Get the asteroid belt inner/outer radius for this system.
    float asteroidBeltInner() const { return m_asteroidBeltInner; }
    float asteroidBeltOuter() const { return m_asteroidBeltOuter; }

    uint32_t seed() const { return m_seed; }
    float simulationTime() const { return m_simTime; }

private:
    /// Compute position on an elliptical orbit (used for initial placement).
    static glm::vec3 computeOrbitalPosition(const OrbitalParams& orbit,
                                              float time,
                                              const glm::vec3& parentPos);

    /// Generate a procedural name from a seed.
    static std::string generateName(uint32_t seed, bool isStar);

    uint32_t m_seed = 0;
    float m_simTime = 0.0f;
    float m_trailTimer = 0.0f;
    CelestialBody m_star;
    CelestialBody m_companionStar;  // Binary companion (if any)
    bool m_isBinary = false;
    std::vector<CelestialBody> m_planets;
    NBodySimulation m_physics;

    // Asteroid belt placement
    float m_asteroidBeltInner = 0.0f;
    float m_asteroidBeltOuter = 0.0f;
};

} // namespace usim
