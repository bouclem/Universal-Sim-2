#include "procedural/MoonGenerator.h"
#include <cmath>
#include <algorithm>

namespace usim {

namespace {

float hashFloat(uint32_t seed, uint32_t salt) {
    uint32_t h = seed ^ (salt * 2654435761u);
    h = ((h >> 16) ^ h) * 0x45d9f3bu;
    h = ((h >> 16) ^ h) * 0x45d9f3bu;
    h = (h >> 16) ^ h;
    return static_cast<float>(h) / static_cast<float>(0xFFFFFFFFu);
}

glm::vec3 hsvToRgb(float h, float s, float v) {
    float c = v * s;
    float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float r = 0, g = 0, b = 0;

    if (h < 60)       { r = c; g = x; }
    else if (h < 120) { r = x; g = c; }
    else if (h < 180) { g = c; b = x; }
    else if (h < 240) { g = x; b = c; }
    else if (h < 300) { r = x; b = c; }
    else              { r = c; b = x; }

    return glm::vec3(r + m, g + m, b + m);
}

} // anonymous namespace

int MoonGenerator::moonCount(uint32_t planetSeed, PlanetType type) {
    float roll = hashFloat(planetSeed, 200);

    switch (type) {
        case PlanetType::Rocky:
            // 40% chance of 0, 40% chance of 1, 20% chance of 2
            if (roll < 0.4f) return 0;
            if (roll < 0.8f) return 1;
            return 2;

        case PlanetType::GasGiant:
            // 1-5 moons
            return 1 + static_cast<int>(roll * 5.0f);

        case PlanetType::IceGiant:
            // 1-3 moons
            return 1 + static_cast<int>(roll * 3.0f);
    }
    return 0;
}

MoonProperties MoonGenerator::generate(uint32_t planetSeed, int moonIndex,
                                        float planetRadius)
{
    uint32_t moonSeed = planetSeed ^ (static_cast<uint32_t>(moonIndex) * 6271u);
    MoonProperties moon{};

    // Moons are small relative to their parent
    moon.radius = planetRadius * (0.05f + hashFloat(moonSeed, 1) * 0.2f);
    // Minimum visible size
    moon.radius = std::max(moon.radius, 0.1f);

    // Orbital distance: progressively farther out
    float baseOrbitDist = planetRadius * 2.0f +
                          static_cast<float>(moonIndex) * planetRadius * 1.5f;
    float jitter = (hashFloat(moonSeed, 2) - 0.5f) * planetRadius * 0.5f;
    moon.orbitalDistance = baseOrbitDist + jitter;

    // Orbital period: Kepler-like, proportional to distance^1.5
    float distRatio = moon.orbitalDistance / planetRadius;
    moon.orbitalPeriod = 3.0f * std::pow(distRatio, 1.5f);

    // Slight inclination
    moon.orbitalInclination = (hashFloat(moonSeed, 3) - 0.5f) * 0.3f;

    // Random start angle
    moon.startAngle = hashFloat(moonSeed, 4) * 6.2831853f;

    // Noise
    moon.noiseScale = 2.0f + hashFloat(moonSeed, 5) * 3.0f;
    moon.noiseSeed = hashFloat(moonSeed, 6) * 1000.0f;

    // Moons are mostly grey/brown rocky bodies
    float hue = hashFloat(moonSeed, 10) * 40.0f; // 0-40 range (greys/browns)
    float sat = 0.1f + hashFloat(moonSeed, 11) * 0.2f;
    float val = 0.3f + hashFloat(moonSeed, 12) * 0.4f;
    moon.colorPrimary = hsvToRgb(hue, sat, val);
    moon.colorSecondary = hsvToRgb(hue + 10.0f, sat * 0.8f, val + 0.1f);
    moon.colorAccent = hsvToRgb(20.0f, 0.15f, val + 0.2f);

    return moon;
}

} // namespace usim
