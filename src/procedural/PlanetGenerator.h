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
    float orbitalAngle;     // Starting angle in orbit (radians)
    float noiseScale;       // Controls terrain/cloud frequency
    float noiseSeed;        // Offset for noise sampling
    glm::vec3 colorPrimary;
    glm::vec3 colorSecondary;
    glm::vec3 colorAccent;

    // v0.2.0: orbital mechanics
    float eccentricity;     // 0 = circle, <1 = ellipse
    float inclination;      // Radians, tilt from ecliptic
    float orbitalPeriod;    // Seconds for one full orbit

    // v0.2.0: rings
    bool hasRings;
    float ringInnerRadius;  // Multiplier of planet radius
    float ringOuterRadius;  // Multiplier of planet radius
    float ringOpacity;
    glm::vec3 ringColor;
    float ringNoiseSeed;

    // v0.2.0: atmosphere
    bool hasAtmosphere;
    float atmosphereThickness; // Multiplier of planet radius
    glm::vec3 atmosphereColor;
    float atmosphereDensity;
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
