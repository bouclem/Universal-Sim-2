#include "scene/SolarSystem.h"
#include "procedural/StarGenerator.h"
#include "procedural/PlanetGenerator.h"
#include "procedural/MoonGenerator.h"
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
    m_simTime = 0.0f;
    m_planets.clear();

    // Generate star
    auto starProps = StarGenerator::generate(seed);
    m_star = CelestialBody{};
    m_star.position = glm::vec3(0.0f);
    m_star.radius = starProps.radius;
    m_star.isStar = true;
    m_star.temperature = starProps.temperature;
    m_star.starColor = starProps.color;
    m_star.luminosity = starProps.luminosity;

    // Number of planets: 4 to 8
    int numPlanets = 4 + static_cast<int>(hashFloat(seed, 100) * 5.0f);

    for (int i = 0; i < numPlanets; ++i) {
        auto pp = PlanetGenerator::generate(seed, i, numPlanets);

        CelestialBody body;
        body.isStar = false;
        body.isMoon = false;
        body.radius = pp.radius;
        body.planetType = static_cast<int>(pp.type);
        body.noiseScale = pp.noiseScale;
        body.noiseSeed = pp.noiseSeed;
        body.colorPrimary = pp.colorPrimary;
        body.colorSecondary = pp.colorSecondary;
        body.colorAccent = pp.colorAccent;

        // Orbital parameters
        body.orbit.semiMajorAxis = pp.orbitalDistance;
        body.orbit.eccentricity = pp.eccentricity;
        body.orbit.inclination = pp.inclination;
        body.orbit.orbitalPeriod = pp.orbitalPeriod;
        body.orbit.startAngle = pp.orbitalAngle;

        // Rings
        body.rings.hasRings = pp.hasRings;
        body.rings.innerRadius = pp.ringInnerRadius;
        body.rings.outerRadius = pp.ringOuterRadius;
        body.rings.opacity = pp.ringOpacity;
        body.rings.color = pp.ringColor;
        body.rings.noiseSeed = pp.ringNoiseSeed;

        // Atmosphere
        body.atmosphere.hasAtmosphere = pp.hasAtmosphere;
        body.atmosphere.thickness = pp.atmosphereThickness;
        body.atmosphere.color = pp.atmosphereColor;
        body.atmosphere.density = pp.atmosphereDensity;

        // Initial position
        body.position = computeOrbitalPosition(body.orbit, 0.0f, m_star.position);

        // Generate moons
        uint32_t planetSeed = seed ^ (static_cast<uint32_t>(i) * 7919u);
        int numMoons = MoonGenerator::moonCount(planetSeed, pp.type);
        for (int m = 0; m < numMoons; ++m) {
            auto mp = MoonGenerator::generate(planetSeed, m, pp.radius);

            CelestialBody moon;
            moon.isStar = false;
            moon.isMoon = true;
            moon.radius = mp.radius;
            moon.planetType = 0; // Moons are always rocky
            moon.noiseScale = mp.noiseScale;
            moon.noiseSeed = mp.noiseSeed;
            moon.colorPrimary = mp.colorPrimary;
            moon.colorSecondary = mp.colorSecondary;
            moon.colorAccent = mp.colorAccent;
            moon.parentIndex = i;

            moon.orbit.semiMajorAxis = mp.orbitalDistance;
            moon.orbit.eccentricity = 0.02f; // Nearly circular
            moon.orbit.inclination = mp.orbitalInclination;
            moon.orbit.orbitalPeriod = mp.orbitalPeriod;
            moon.orbit.startAngle = mp.startAngle;

            moon.position = computeOrbitalPosition(moon.orbit, 0.0f, body.position);

            body.moons.push_back(moon);
        }

        m_planets.push_back(body);
    }
}

void SolarSystem::update(float deltaTime, float timeScale) {
    m_simTime += deltaTime * timeScale;

    // Update planet positions
    for (auto& planet : m_planets) {
        planet.position = computeOrbitalPosition(planet.orbit, m_simTime,
                                                  m_star.position);

        // Update moon positions relative to their parent planet
        for (auto& moon : planet.moons) {
            moon.position = computeOrbitalPosition(moon.orbit, m_simTime,
                                                    planet.position);
        }
    }
}

glm::vec3 SolarSystem::computeOrbitalPosition(const OrbitalParams& orbit,
                                                float time,
                                                const glm::vec3& parentPos)
{
    // Mean anomaly: angle progresses linearly with time
    float meanAnomaly = orbit.startAngle +
                        (6.2831853f / orbit.orbitalPeriod) * time;

    // Solve Kepler's equation iteratively: E - e*sin(E) = M
    // For low eccentricity, a few iterations suffice
    float E = meanAnomaly;
    for (int i = 0; i < 5; ++i) {
        E = meanAnomaly + orbit.eccentricity * std::sin(E);
    }

    // True anomaly from eccentric anomaly
    float cosE = std::cos(E);
    float sinE = std::sin(E);
    float cosV = (cosE - orbit.eccentricity) /
                 (1.0f - orbit.eccentricity * cosE);
    float sinV = (std::sqrt(1.0f - orbit.eccentricity * orbit.eccentricity) * sinE) /
                 (1.0f - orbit.eccentricity * cosE);

    // Distance from focus
    float r = orbit.semiMajorAxis * (1.0f - orbit.eccentricity * cosE);

    // Position in orbital plane
    float x = r * cosV;
    float z = r * sinV;

    // Apply inclination (rotate around x-axis)
    float cosI = std::cos(orbit.inclination);
    float sinI = std::sin(orbit.inclination);
    float y = z * sinI;
    z = z * cosI;

    return parentPos + glm::vec3(x, y, z);
}

std::vector<const CelestialBody*> SolarSystem::allBodies() const {
    std::vector<const CelestialBody*> bodies;
    bodies.push_back(&m_star);
    for (const auto& p : m_planets) {
        bodies.push_back(&p);
        for (const auto& m : p.moons) {
            bodies.push_back(&m);
        }
    }
    return bodies;
}

} // namespace usim
