#include "procedural/StarGenerator.h"
#include <cmath>
#include <algorithm>

namespace usim {

namespace {

// Simple hash for deterministic randomness from seed
float hashFloat(uint32_t seed, uint32_t salt) {
    uint32_t h = seed ^ (salt * 2654435761u);
    h = ((h >> 16) ^ h) * 0x45d9f3bu;
    h = ((h >> 16) ^ h) * 0x45d9f3bu;
    h = (h >> 16) ^ h;
    return static_cast<float>(h) / static_cast<float>(0xFFFFFFFFu);
}

} // anonymous namespace

StarProperties StarGenerator::generate(uint32_t seed) {
    StarProperties star{};

    // Temperature: weighted toward cooler stars (more realistic distribution)
    float t = hashFloat(seed, 1);
    // Use power distribution to favor cooler stars
    t = std::pow(t, 2.0f);
    star.temperature = 2000.0f + t * 38000.0f;

    // Radius: roughly correlated with temperature (v0.6.0: larger stars)
    // Typical range: 6-18 units, hot stars can reach ~36
    float baseRadius = 6.0f + hashFloat(seed, 2) * 8.0f;
    if (star.temperature > 10000.0f) {
        baseRadius *= 1.5f + hashFloat(seed, 3) * 1.5f;
    }
    star.radius = baseRadius;

    // Luminosity from Stefan-Boltzmann (simplified)
    float tempRatio = star.temperature / 5778.0f; // relative to Sun
    float radiusRatio = star.radius / 10.0f;
    star.luminosity = radiusRatio * radiusRatio * std::pow(tempRatio, 4.0f);

    // Mass: rough main-sequence relation (v0.6.0: scaled for new G)
    star.mass = 2000.0f + star.luminosity * 500.0f;

    star.color = blackbodyColor(star.temperature);

    return star;
}

glm::vec3 StarGenerator::blackbodyColor(float temperature) {
    // Attempt to approximate blackbody radiation color.
    // Based on Tanner Helland's algorithm.
    float temp = temperature / 100.0f;
    float r, g, b;

    // Red
    if (temp <= 66.0f) {
        r = 255.0f;
    } else {
        r = 329.698727446f * std::pow(temp - 60.0f, -0.1332047592f);
    }

    // Green
    if (temp <= 66.0f) {
        g = 99.4708025861f * std::log(temp) - 161.1195681661f;
    } else {
        g = 288.1221695283f * std::pow(temp - 60.0f, -0.0755148492f);
    }

    // Blue
    if (temp >= 66.0f) {
        b = 255.0f;
    } else if (temp <= 19.0f) {
        b = 0.0f;
    } else {
        b = 138.5177312231f * std::log(temp - 10.0f) - 305.0447927307f;
    }

    r = std::clamp(r, 0.0f, 255.0f) / 255.0f;
    g = std::clamp(g, 0.0f, 255.0f) / 255.0f;
    b = std::clamp(b, 0.0f, 255.0f) / 255.0f;

    return glm::vec3(r, g, b);
}

} // namespace usim
