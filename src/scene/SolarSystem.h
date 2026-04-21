#pragma once

#include "scene/CelestialBody.h"
#include <vector>
#include <cstdint>

namespace usim {

/// Generates and holds a complete procedural solar system.
class SolarSystem {
public:
    /// Generate a solar system from a seed.
    void generate(uint32_t seed);

    /// Update orbital positions. Call each frame.
    /// timeScale: 1.0 = normal, 0 = paused, >1 = fast forward
    void update(float deltaTime, float timeScale = 1.0f);

    const CelestialBody& star() const { return m_star; }
    const std::vector<CelestialBody>& planets() const { return m_planets; }
    std::vector<CelestialBody>& planets() { return m_planets; }

    /// Get all bodies (star + planets + moons) for rendering.
    std::vector<const CelestialBody*> allBodies() const;

    uint32_t seed() const { return m_seed; }
    float simulationTime() const { return m_simTime; }

private:
    /// Compute position on an elliptical orbit.
    static glm::vec3 computeOrbitalPosition(const OrbitalParams& orbit,
                                              float time,
                                              const glm::vec3& parentPos);

    uint32_t m_seed = 0;
    float m_simTime = 0.0f;
    CelestialBody m_star;
    std::vector<CelestialBody> m_planets;
};

} // namespace usim
