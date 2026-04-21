#include "scene/SolarSystem.h"
#include "procedural/StarGenerator.h"
#include "procedural/PlanetGenerator.h"
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

} // anonymous namespace

// Syllable-based name generator
std::string SolarSystem::generateName(uint32_t seed, bool isStar) {
    static const char* prefixes[] = {
        "Al", "Be", "Ca", "De", "El", "Fa", "Ga", "Ha",
        "Io", "Ja", "Ka", "Le", "Ma", "Ne", "Or", "Pa",
        "Qu", "Ra", "Si", "Ta", "Ur", "Ve", "Wa", "Xe",
        "Zy", "Ar", "Bo", "Cr", "Dr", "Ep"
    };
    static const char* middles[] = {
        "ri", "la", "no", "te", "si", "mu", "ka", "do",
        "ve", "na", "go", "pe", "lu", "ra", "xi", "to",
        "mi", "sa", "ke", "da", "fi", "nu", "ba", "ze"
    };
    static const char* suffixes[] = {
        "on", "us", "is", "ar", "en", "ia", "os", "um",
        "ax", "ix", "or", "an", "el", "as", "ur", "es"
    };

    int nPre = 30, nMid = 24, nSuf = 16;
    int pi = static_cast<int>(hashFloat(seed, 500) * static_cast<float>(nPre)) % nPre;
    int mi = static_cast<int>(hashFloat(seed, 501) * static_cast<float>(nMid)) % nMid;
    int si = static_cast<int>(hashFloat(seed, 502) * static_cast<float>(nSuf)) % nSuf;

    std::string name = std::string(prefixes[pi]) + middles[mi] + suffixes[si];

    // Stars get a catalog-style suffix
    if (isStar) {
        int num = static_cast<int>(hashFloat(seed, 503) * 900.0f) + 100;
        name += "-" + std::to_string(num);
    }

    return name;
}

void SolarSystem::generate(uint32_t seed) {
    m_seed = seed;
    m_simTime = 0.0f;
    m_trailTimer = 0.0f;
    m_planets.clear();

    // Generate star
    auto starProps = StarGenerator::generate(seed);
    m_star = CelestialBody{};
    m_star.name = generateName(seed, true);
    m_star.position = glm::vec3(0.0f);
    m_star.velocity = glm::vec3(0.0f);
    m_star.radius = starProps.radius;
    m_star.isStar = true;
    m_star.temperature = starProps.temperature;
    m_star.starColor = starProps.color;
    m_star.luminosity = starProps.luminosity;

    // Star mass: proportional to luminosity (rough main-sequence relation)
    m_star.mass = 500.0f + starProps.luminosity * 200.0f;

    // Number of planets: 4 to 8
    int numPlanets = 4 + static_cast<int>(hashFloat(seed, 100) * 5.0f);

    for (int i = 0; i < numPlanets; ++i) {
        auto pp = PlanetGenerator::generate(seed, i, numPlanets);

        CelestialBody body;
        body.name = generateName(seed ^ (static_cast<uint32_t>(i + 1) * 3571u), false);
        body.isStar = false;
        body.isMoon = false;
        body.radius = pp.radius;
        body.planetType = static_cast<int>(pp.type);
        body.noiseScale = pp.noiseScale;
        body.noiseSeed = pp.noiseSeed;
        body.colorPrimary = pp.colorPrimary;
        body.colorSecondary = pp.colorSecondary;
        body.colorAccent = pp.colorAccent;

        // Mass based on type and radius
        float r3 = pp.radius * pp.radius * pp.radius;
        switch (pp.type) {
            case PlanetType::Rocky:    body.mass = r3 * 2.0f; break;
            case PlanetType::GasGiant: body.mass = r3 * 0.5f; break;
            case PlanetType::IceGiant: body.mass = r3 * 0.8f; break;
        }

        // Orbital parameters (kept for reference)
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

        // Initial position from Kepler orbit
        body.position = computeOrbitalPosition(body.orbit, 0.0f, m_star.position);

        // Initial velocity for circular orbit (n-body will take over)
        body.velocity = NBodySimulation::circularOrbitalVelocity(
            body.position, m_star.position, m_star.mass);

        // Generate moons
        uint32_t planetSeed = seed ^ (static_cast<uint32_t>(i) * 7919u);
        int numMoons = MoonGenerator::moonCount(planetSeed, pp.type);
        for (int m = 0; m < numMoons; ++m) {
            auto mp = MoonGenerator::generate(planetSeed, m, pp.radius);

            CelestialBody moon;
            moon.name = body.name + " " + static_cast<char>('a' + m);
            moon.isStar = false;
            moon.isMoon = true;
            moon.radius = mp.radius;
            moon.planetType = 0;
            moon.noiseScale = mp.noiseScale;
            moon.noiseSeed = mp.noiseSeed;
            moon.colorPrimary = mp.colorPrimary;
            moon.colorSecondary = mp.colorSecondary;
            moon.colorAccent = mp.colorAccent;
            moon.parentIndex = i;
            moon.mass = mp.radius * mp.radius * mp.radius * 0.5f;

            moon.orbit.semiMajorAxis = mp.orbitalDistance;
            moon.orbit.eccentricity = 0.02f;
            moon.orbit.inclination = mp.orbitalInclination;
            moon.orbit.orbitalPeriod = mp.orbitalPeriod;
            moon.orbit.startAngle = mp.startAngle;

            moon.position = computeOrbitalPosition(moon.orbit, 0.0f, body.position);
            moon.velocity = body.velocity +
                NBodySimulation::circularOrbitalVelocity(
                    moon.position, body.position, body.mass);

            body.moons.push_back(moon);
        }

        m_planets.push_back(body);
    }
}

void SolarSystem::update(float deltaTime, float timeScale) {
    float dt = deltaTime * timeScale;
    m_simTime += dt;

    // Gather all bodies for n-body simulation
    auto bodies = allBodiesMut();
    m_physics.step(bodies, dt);

    // Record trails periodically
    m_trailTimer += dt;
    if (m_trailTimer > 0.05f) {
        m_trailTimer = 0.0f;
        recordTrails();
    }
}

void SolarSystem::recordTrails() {
    // Record planet trails
    for (auto& planet : m_planets) {
        planet.trail.push_back(planet.position);
        if (planet.trail.size() > CelestialBody::MAX_TRAIL_POINTS) {
            planet.trail.pop_front();
        }
        // Moon trails
        for (auto& moon : planet.moons) {
            moon.trail.push_back(moon.position);
            if (moon.trail.size() > CelestialBody::MAX_TRAIL_POINTS) {
                moon.trail.pop_front();
            }
        }
    }
}

const CelestialBody* SolarSystem::findClosestToRay(const glm::vec3& origin,
                                                     const glm::vec3& direction) const
{
    const CelestialBody* closest = nullptr;
    float closestDist = 1e30f;

    auto bodies = allBodies();
    for (const auto* body : bodies) {
        // Ray-sphere intersection test (approximate: use closest point on ray)
        glm::vec3 oc = body->position - origin;
        float t = glm::dot(oc, direction);
        if (t < 0.0f) continue; // Behind camera

        glm::vec3 closestPoint = origin + direction * t;
        float dist = glm::length(closestPoint - body->position);

        // Hit if within a generous radius (for selection ease)
        float hitRadius = body->radius * 2.0f;
        if (dist < hitRadius && t < closestDist) {
            closestDist = t;
            closest = body;
        }
    }

    // If nothing hit, find the body closest to the ray direction
    if (!closest) {
        float bestAngle = 1e30f;
        for (const auto* body : bodies) {
            glm::vec3 toBody = glm::normalize(body->position - origin);
            float angle = std::acos(std::clamp(glm::dot(toBody, direction), -1.0f, 1.0f));
            if (angle < bestAngle && angle < 0.15f) { // ~8.5 degree cone
                bestAngle = angle;
                closest = body;
            }
        }
    }

    return closest;
}

glm::vec3 SolarSystem::computeOrbitalPosition(const OrbitalParams& orbit,
                                                float time,
                                                const glm::vec3& parentPos)
{
    float meanAnomaly = orbit.startAngle +
                        (6.2831853f / orbit.orbitalPeriod) * time;

    float E = meanAnomaly;
    for (int i = 0; i < 5; ++i) {
        E = meanAnomaly + orbit.eccentricity * std::sin(E);
    }

    float cosE = std::cos(E);
    float sinE = std::sin(E);
    float cosV = (cosE - orbit.eccentricity) /
                 (1.0f - orbit.eccentricity * cosE);
    float sinV = (std::sqrt(1.0f - orbit.eccentricity * orbit.eccentricity) * sinE) /
                 (1.0f - orbit.eccentricity * cosE);

    float r = orbit.semiMajorAxis * (1.0f - orbit.eccentricity * cosE);

    float x = r * cosV;
    float z = r * sinV;

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

std::vector<CelestialBody*> SolarSystem::allBodiesMut() {
    std::vector<CelestialBody*> bodies;
    bodies.push_back(&m_star);
    for (auto& p : m_planets) {
        bodies.push_back(&p);
        for (auto& m : p.moons) {
            bodies.push_back(&m);
        }
    }
    return bodies;
}

} // namespace usim
