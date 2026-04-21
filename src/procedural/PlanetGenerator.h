#pragma once

#include <glm/glm.hpp>
#include <cstdint>

namespace usim {

enum class PlanetType {
    Rocky,
    GasGiant,
    IceGiant
};

/// Properties of a procedurally generated planet.
struct PlanetProperties {
    PlanetType type;
    float radius;           // Arbitrary units
    float orbitalDistance;   // Distance from star
    float orbitalAngle;     // Current angle in orbit (radians)
    float noiseScale;       // Controls terrain/cloud frequency
    float noiseSeed;        // Offset for noise sampling
    glm::vec3 colorPrimary;
    glm::vec3 colorSecondary;
    glm::vec3 colorAccent;
};

/// Generates planet properties from a seed and orbital index.
class PlanetGenerator {
public:
    /// Generate planet properties.
    /// index: 0-based planet index (inner to outer)
    /// totalPlanets: total number of planets in the system
    static PlanetProperties generate(uint32_t seed, int index, int totalPlanets);
};

} // namespace usim
