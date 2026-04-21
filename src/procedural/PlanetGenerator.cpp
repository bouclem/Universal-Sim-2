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

    // --- Orbital mechanics (v0.2.0) ---
    // Eccentricity: mostly low, occasional higher
    planet.eccentricity = hashFloat(planetSeed, 20) * 0.15f;
    // Inclination: small tilt from ecliptic
    planet.inclination = (hashFloat(planetSeed, 21) - 0.5f) * 0.15f;
    // Orbital period: Kepler's 3rd law (T^2 ∝ a^3), scaled for gameplay
    float distAU = planet.orbitalDistance / 15.0f; // normalize
    planet.orbitalPeriod = 15.0f * std::pow(distAU, 1.5f);

    // --- Rings (v0.2.0) ---
    float ringRoll = hashFloat(planetSeed, 30);
    switch (planet.type) {
        case PlanetType::GasGiant:
            planet.hasRings = (ringRoll > 0.4f); // 60% chance
            break;
        case PlanetType::IceGiant:
            planet.hasRings = (ringRoll > 0.7f); // 30% chance
            break;
        case PlanetType::Rocky:
            planet.hasRings = false;
            break;
    }
    if (planet.hasRings) {
        planet.ringInnerRadius = 1.3f + hashFloat(planetSeed, 31) * 0.3f;
        planet.ringOuterRadius = planet.ringInnerRadius + 0.5f + hashFloat(planetSeed, 32) * 1.0f;
        planet.ringOpacity = 0.3f + hashFloat(planetSeed, 33) * 0.5f;
        float ringHue = hashFloat(planetSeed, 34) * 50.0f + 20.0f; // warm tones
        planet.ringColor = hsvToRgb(ringHue, 0.2f + hashFloat(planetSeed, 35) * 0.2f,
                                    0.6f + hashFloat(planetSeed, 36) * 0.3f);
        planet.ringNoiseSeed = hashFloat(planetSeed, 37) * 1000.0f;
    } else {
        planet.ringInnerRadius = 0.0f;
        planet.ringOuterRadius = 0.0f;
        planet.ringOpacity = 0.0f;
        planet.ringColor = glm::vec3(0.0f);
        planet.ringNoiseSeed = 0.0f;
    }

    // --- Atmosphere (v0.2.0) ---
    float atmoRoll = hashFloat(planetSeed, 40);
    switch (planet.type) {
        case PlanetType::Rocky:
            planet.hasAtmosphere = (atmoRoll > 0.5f); // 50% chance
            if (planet.hasAtmosphere) {
                planet.atmosphereThickness = 0.05f + hashFloat(planetSeed, 41) * 0.1f;
                // Thin, could be reddish (Mars-like) or bluish (Earth-like)
                float atmoHue = hashFloat(planetSeed, 42) > 0.5f ? 210.0f : 15.0f;
                planet.atmosphereColor = hsvToRgb(atmoHue, 0.5f, 0.8f);
                planet.atmosphereDensity = 0.2f + hashFloat(planetSeed, 43) * 0.3f;
            }
            break;
        case PlanetType::GasGiant:
            planet.hasAtmosphere = true;
            planet.atmosphereThickness = 0.1f + hashFloat(planetSeed, 41) * 0.15f;
            planet.atmosphereColor = hsvToRgb(30.0f + hashFloat(planetSeed, 42) * 30.0f,
                                              0.3f, 0.9f);
            planet.atmosphereDensity = 0.5f + hashFloat(planetSeed, 43) * 0.3f;
            break;
        case PlanetType::IceGiant:
            planet.hasAtmosphere = true;
            planet.atmosphereThickness = 0.08f + hashFloat(planetSeed, 41) * 0.12f;
            planet.atmosphereColor = hsvToRgb(200.0f + hashFloat(planetSeed, 42) * 40.0f,
                                              0.4f, 0.85f);
            planet.atmosphereDensity = 0.4f + hashFloat(planetSeed, 43) * 0.3f;
            break;
    }
    if (!planet.hasAtmosphere) {
        planet.atmosphereThickness = 0.0f;
        planet.atmosphereColor = glm::vec3(0.0f);
        planet.atmosphereDensity = 0.0f;
    }

    // Colors based on planet type (unchanged from v0.1.0)
    switch (planet.type) {
        case PlanetType::Rocky: {
            float hue = hashFloat(planetSeed, 10) * 60.0f;
            planet.colorPrimary = hsvToRgb(hue, 0.4f + hashFloat(planetSeed, 11) * 0.3f,
                                           0.4f + hashFloat(planetSeed, 12) * 0.3f);
            planet.colorSecondary = hsvToRgb(hue + 20.0f, 0.3f,
                                             0.6f + hashFloat(planetSeed, 13) * 0.2f);
            float accentHue = hashFloat(planetSeed, 14) > 0.5f ? 210.0f : 30.0f;
            planet.colorAccent = hsvToRgb(accentHue, 0.5f, 0.7f);
            break;
        }
        case PlanetType::GasGiant: {
            float hue = 20.0f + hashFloat(planetSeed, 10) * 40.0f;
            planet.colorPrimary = hsvToRgb(hue, 0.6f,
                                           0.7f + hashFloat(planetSeed, 11) * 0.2f);
            planet.colorSecondary = hsvToRgb(hue + 15.0f, 0.5f,
                                             0.5f + hashFloat(planetSeed, 12) * 0.3f);
            planet.colorAccent = hsvToRgb(hue - 10.0f, 0.7f, 0.8f);
            break;
        }
        case PlanetType::IceGiant: {
            float hue = 180.0f + hashFloat(planetSeed, 10) * 60.0f;
            planet.colorPrimary = hsvToRgb(hue, 0.4f + hashFloat(planetSeed, 11) * 0.3f,
                                           0.6f + hashFloat(planetSeed, 12) * 0.2f);
            planet.colorSecondary = hsvToRgb(hue + 20.0f, 0.3f, 0.7f);
            planet.colorAccent = hsvToRgb(hue - 20.0f, 0.5f, 0.8f);
            break;
        }
    }

    return planet;
}

} // namespace usim
