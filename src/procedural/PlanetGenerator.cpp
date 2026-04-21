#include "procedural/PlanetGenerator.h"
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

PlanetProperties PlanetGenerator::generate(uint32_t seed, int index,
                                            int totalPlanets)
{
    uint32_t planetSeed = seed ^ (static_cast<uint32_t>(index) * 7919u);
    PlanetProperties planet{};

    float t = static_cast<float>(index) / static_cast<float>(totalPlanets);

    // Inner planets tend to be rocky, outer ones gas/ice
    float typeRoll = hashFloat(planetSeed, 1);
    if (t < 0.4f) {
        planet.type = PlanetType::Rocky;
    } else if (t < 0.7f) {
        planet.type = (typeRoll > 0.3f) ? PlanetType::GasGiant : PlanetType::Rocky;
    } else {
        planet.type = (typeRoll > 0.5f) ? PlanetType::IceGiant : PlanetType::GasGiant;
    }

    // Radius depends on type
    switch (planet.type) {
        case PlanetType::Rocky:
            planet.radius = 0.3f + hashFloat(planetSeed, 2) * 0.8f;
            break;
        case PlanetType::GasGiant:
            planet.radius = 1.5f + hashFloat(planetSeed, 2) * 2.5f;
            break;
        case PlanetType::IceGiant:
            planet.radius = 1.0f + hashFloat(planetSeed, 2) * 1.5f;
            break;
    }

    // Orbital distance: Titius-Bode-like spacing
    float baseDistance = 15.0f + static_cast<float>(index) * 12.0f;
    float jitter = (hashFloat(planetSeed, 3) - 0.5f) * 4.0f;
    planet.orbitalDistance = baseDistance + jitter;

    // Random starting orbital angle
    planet.orbitalAngle = hashFloat(planetSeed, 4) * 6.2831853f;

    // Noise parameters
    planet.noiseScale = 1.0f + hashFloat(planetSeed, 5) * 4.0f;
    planet.noiseSeed = hashFloat(planetSeed, 6) * 1000.0f;

    // Colors based on planet type
    switch (planet.type) {
        case PlanetType::Rocky: {
            float hue = hashFloat(planetSeed, 10) * 60.0f; // browns/reds/oranges
            planet.colorPrimary = hsvToRgb(hue, 0.4f + hashFloat(planetSeed, 11) * 0.3f, 0.4f + hashFloat(planetSeed, 12) * 0.3f);
            planet.colorSecondary = hsvToRgb(hue + 20.0f, 0.3f, 0.6f + hashFloat(planetSeed, 13) * 0.2f);
            // Accent: could be ice caps or oceans
            float accentHue = hashFloat(planetSeed, 14) > 0.5f ? 210.0f : 30.0f;
            planet.colorAccent = hsvToRgb(accentHue, 0.5f, 0.7f);
            break;
        }
        case PlanetType::GasGiant: {
            float hue = 20.0f + hashFloat(planetSeed, 10) * 40.0f; // oranges/yellows
            planet.colorPrimary = hsvToRgb(hue, 0.6f, 0.7f + hashFloat(planetSeed, 11) * 0.2f);
            planet.colorSecondary = hsvToRgb(hue + 15.0f, 0.5f, 0.5f + hashFloat(planetSeed, 12) * 0.3f);
            planet.colorAccent = hsvToRgb(hue - 10.0f, 0.7f, 0.8f);
            break;
        }
        case PlanetType::IceGiant: {
            float hue = 180.0f + hashFloat(planetSeed, 10) * 60.0f; // cyans/blues
            planet.colorPrimary = hsvToRgb(hue, 0.4f + hashFloat(planetSeed, 11) * 0.3f, 0.6f + hashFloat(planetSeed, 12) * 0.2f);
            planet.colorSecondary = hsvToRgb(hue + 20.0f, 0.3f, 0.7f);
            planet.colorAccent = hsvToRgb(hue - 20.0f, 0.5f, 0.8f);
            break;
        }
    }

    return planet;
}

} // namespace usim
