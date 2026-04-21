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

    const CelestialBody& star() const { return m_star; }
    const std::vector<CelestialBody>& planets() const { return m_planets; }

    /// Get all bodies (star + planets) for rendering.
    std::vector<const CelestialBody*> allBodies() const;

    uint32_t seed() const { return m_seed; }

private:
    uint32_t m_seed = 0;
    CelestialBody m_star;
    std::vector<CelestialBody> m_planets;
};

} // namespace usim
