#include "scene/SolarSystem.h"
#include "procedural/StarGenerator.h"
#include "procedural/PlanetGenerator.h"
#include <cmath>

namespace usim {

namespace {

float hashFloat(uint32_t seed, uint32_t salt) {
    uint32_t h = seed ^ (salt * 2654435761u);
    h = ((h >> 16) ^ h) * 0x45d9f3bu;
    h = ((h >> 16) ^ h) * 0x45d9f3bu;
    h = (h >> 16) ^ h;
    return static_cast<float>(h) / static_cast<float>(0xFFFFFFFFu);
}

} // anonymous namespace

void SolarSystem::generate(uint32_t seed) {
    m_seed = seed;
    m_planets.clear();

    // Generate star
    auto starProps = StarGenerator::generate(seed);
    m_star.position = glm::vec3(0.0f);
    m_star.radius = starProps.radius;
    m_star.isStar = true;
    m_star.temperature = starProps.temperature;
    m_star.starColor = starProps.color;
    m_star.luminosity = starProps.luminosity;

    // Number of planets: 4 to 8
    int numPlanets = 4 + static_cast<int>(hashFloat(seed, 100) * 5.0f);

    for (int i = 0; i < numPlanets; ++i) {
        auto planetProps = PlanetGenerator::generate(seed, i, numPlanets);

        CelestialBody body;
        body.isStar = false;
        body.radius = planetProps.radius;
        body.planetType = static_cast<int>(planetProps.type);
        body.noiseScale = planetProps.noiseScale;
        body.noiseSeed = planetProps.noiseSeed;
        body.colorPrimary = planetProps.colorPrimary;
        body.colorSecondary = planetProps.colorSecondary;
        body.colorAccent = planetProps.colorAccent;

        // Place planet in orbit
        float x = std::cos(planetProps.orbitalAngle) * planetProps.orbitalDistance;
        float z = std::sin(planetProps.orbitalAngle) * planetProps.orbitalDistance;
        body.position = glm::vec3(x, 0.0f, z);

        m_planets.push_back(body);
    }
}

std::vector<const CelestialBody*> SolarSystem::allBodies() const {
    std::vector<const CelestialBody*> bodies;
    bodies.reserve(1 + m_planets.size());
    bodies.push_back(&m_star);
    for (const auto& p : m_planets) {
        bodies.push_back(&p);
    }
    return bodies;
}

} // namespace usim
