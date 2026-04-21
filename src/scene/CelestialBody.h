#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <deque>

namespace usim {

/// Orbital parameters for any orbiting body.
struct OrbitalParams {
    float semiMajorAxis = 20.0f;   // Distance from parent
    float eccentricity = 0.0f;     // 0 = circle, <1 = ellipse
    float inclination = 0.0f;      // Radians, tilt from ecliptic
    float orbitalPeriod = 10.0f;   // Seconds for one full orbit
    float startAngle = 0.0f;       // Initial true anomaly (radians)
};

/// Ring parameters for gas/ice giants.
struct RingParams {
    bool hasRings = false;
    float innerRadius = 1.3f;      // Multiplier of planet radius
    float outerRadius = 2.2f;      // Multiplier of planet radius
    float opacity = 0.6f;
    glm::vec3 color = glm::vec3(0.8f, 0.75f, 0.65f);
    float noiseSeed = 0.0f;
};

/// Atmosphere parameters.
struct AtmosphereParams {
    bool hasAtmosphere = false;
    float thickness = 0.15f;       // Multiplier of planet radius added on top
    glm::vec3 color = glm::vec3(0.4f, 0.6f, 1.0f);
    float density = 0.5f;          // 0-1, affects opacity
};

/// Base data for any celestial body (star, planet, or moon).
struct CelestialBody {
    std::string name;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 velocity = glm::vec3(0.0f);
    float radius = 1.0f;
    float mass = 1.0f;             // Arbitrary mass units
    bool isStar = false;
    bool isMoon = false;
    bool markedForRemoval = false;  // Flagged after collision absorption

    // Star-specific
    float temperature = 5778.0f;
    glm::vec3 starColor = glm::vec3(1.0f);
    float luminosity = 1.0f;

    // Planet-specific
    int planetType = 0;       // 0=rocky, 1=gas, 2=ice
    float noiseScale = 2.0f;
    float noiseSeed = 0.0f;
    glm::vec3 colorPrimary = glm::vec3(0.5f);
    glm::vec3 colorSecondary = glm::vec3(0.3f);
    glm::vec3 colorAccent = glm::vec3(0.7f);

    // Orbital mechanics (used for initial placement)
    OrbitalParams orbit;

    // Rings (planets only)
    RingParams rings;

    // Atmosphere (planets only)
    AtmosphereParams atmosphere;

    // Moons (planets only, not recursive)
    std::vector<CelestialBody> moons;

    // Index of parent planet in the planets array (-1 for star/planets)
    int parentIndex = -1;

    // Orbit trail: recent positions for visualization
    std::deque<glm::vec3> trail;
    static constexpr size_t MAX_TRAIL_POINTS = 512;

    // Collision flash timer (visual feedback)
    float collisionFlash = 0.0f;
};

} // namespace usim
