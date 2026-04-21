#pragma once

#include "procedural/PlanetGenerator.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

namespace usim {

/// Properties of a procedurally generated moon.
struct MoonProperties {
    float radius;
    float orbitalDistance;    // Distance from parent planet
    float orbitalPeriod;     // Seconds per orbit
    float orbitalInclination;
    float startAngle;
    float noiseScale;
    float noiseSeed;
    glm::vec3 colorPrimary;
    glm::vec3 colorSecondary;
    glm::vec3 colorAccent;

    // v0.6.0: rotation
    float rotationSpeed;
    float axialTilt;
};

/// Generates moon properties for a given planet.
class MoonGenerator {
public:
    /// Determine how many moons a planet gets.
    static int moonCount(uint32_t planetSeed, PlanetType type);

    /// Generate properties for a specific moon.
    static MoonProperties generate(uint32_t planetSeed, int moonIndex,
                                   float planetRadius);
};

} // namespace usim
