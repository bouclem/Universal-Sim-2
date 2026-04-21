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

    if (isStar) {
        int num = static_cast<int>(hashFloat(seed, 503) * 900.0f) + 100;
        name += "-" + std::to_string(num);
    }

    return name;
}

void SolarSystem::generate(uint32_t seed, bool binary) {
    m_seed = seed;
    m_simTime = 0.0f;
    m_trailTimer = 0.0f;
    m_planets.clear();
    m_isBinary = binary;

    // Generate primary star
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
    m_star.mass = 500.0f + starProps.luminosity * 200.0f;

    // Generate companion star if binary
    m_companionStar = CelestialBody{};
    if (m_isBinary) {
        uint32_t compSeed = seed ^ 0xDEADBEEFu;
        auto compProps = StarGenerator::generate(compSeed);
        m_companionStar.name = generateName(compSeed, true);
        m_companionStar.isStar = true;
        m_companionStar.radius = compProps.radius * 0.7f; // Smaller companion
        m_companionStar.temperature = compProps.temperature;
        m_companionStar.starColor = compProps.color;
        m_companionStar.luminosity = compProps.luminosity * 0.5f;
        m_companionStar.mass = m_star.mass * 0.6f;

        // Binary orbit: place companion at a distance, give orbital velocity
        float binaryDist = 8.0f + hashFloat(seed, 600) * 6.0f;
        m_companionStar.position = glm::vec3(binaryDist, 0.0f, 0.0f);

        // Orbital velocities for binary (both orbit the barycenter)
        float totalMass = m_star.mass + m_companionStar.mass;
        float starDist = binaryDist * m_companionStar.mass / totalMass;
        float compDist = binaryDist * m_star.mass / totalMass;

        float orbitalSpeed = std::sqrt(NBodySimulation::G * totalMass / binaryDist);
        float starSpeed = orbitalSpeed * m_companionStar.mass / totalMass;
        float compSpeed = orbitalSpeed * m_star.mass / totalMass;

        m_star.position = glm::vec3(-starDist, 0.0f, 0.0f);
        m_star.velocity = glm::vec3(0.0f, 0.0f, starSpeed);
        m_companionStar.position = glm::vec3(compDist, 0.0f, 0.0f);
        m_companionStar.velocity = glm::vec3(0.0f, 0.0f, -compSpeed);
    }

    // Number of planets: 4 to 8
    int numPlanets = 4 + static_cast<int>(hashFloat(seed, 100) * 5.0f);

    // Track orbital distances for asteroid belt placement
    std::vector<float> orbitalDistances;

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

        float r3 = pp.radius * pp.radius * pp.radius;
        switch (pp.type) {
            case PlanetType::Rocky:    body.mass = r3 * 2.0f; break;
            case PlanetType::GasGiant: body.mass = r3 * 0.5f; break;
            case PlanetType::IceGiant: body.mass = r3 * 0.8f; break;
        }

        body.orbit.semiMajorAxis = pp.orbitalDistance;
        body.orbit.eccentricity = pp.eccentricity;
        body.orbit.inclination = pp.inclination;
        body.orbit.orbitalPeriod = pp.orbitalPeriod;
        body.orbit.startAngle = pp.orbitalAngle;

        body.rings.hasRings = pp.hasRings;
        body.rings.innerRadius = pp.ringInnerRadius;
        body.rings.outerRadius = pp.ringOuterRadius;
        body.rings.opacity = pp.ringOpacity;
        body.rings.color = pp.ringColor;
        body.rings.noiseSeed = pp.ringNoiseSeed;

        body.atmosphere.hasAtmosphere = pp.hasAtmosphere;
        body.atmosphere.thickness = pp.atmosphereThickness;
        body.atmosphere.color = pp.atmosphereColor;
        body.atmosphere.density = pp.atmosphereDensity;

        body.position = computeOrbitalPosition(body.orbit, 0.0f, m_star.position);
        body.velocity = NBodySimulation::circularOrbitalVelocity(
            body.position, m_star.position, m_star.mass);

        orbitalDistances.push_back(pp.orbitalDistance);

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

    // Determine asteroid belt placement: find the largest gap between
    // rocky and gas/ice planets (like the Mars-Jupiter gap)
    std::sort(orbitalDistances.begin(), orbitalDistances.end());
    float maxGap = 0.0f;
    int gapIndex = -1;
    for (size_t i = 1; i < orbitalDistances.size(); ++i) {
        float gap = orbitalDistances[i] - orbitalDistances[i - 1];
        if (gap > maxGap) {
            maxGap = gap;
            gapIndex = static_cast<int>(i);
        }
    }

    if (gapIndex > 0 && maxGap > 8.0f) {
        float gapCenter = (orbitalDistances[static_cast<size_t>(gapIndex - 1)] +
                           orbitalDistances[static_cast<size_t>(gapIndex)]) * 0.5f;
        float beltWidth = maxGap * 0.4f;
        m_asteroidBeltInner = gapCenter - beltWidth * 0.5f;
        m_asteroidBeltOuter = gapCenter + beltWidth * 0.5f;
    } else {
        // Fallback: place belt at 60% of the way out
        float outerDist = orbitalDistances.empty() ? 50.0f : orbitalDistances.back();
        m_asteroidBeltInner = outerDist * 0.55f;
        m_asteroidBeltOuter = outerDist * 0.65f;
    }
}

void SolarSystem::update(float deltaTime, float timeScale) {
    float dt = deltaTime * timeScale;
    m_simTime += dt;

    auto bodies = allBodiesMut();
    m_physics.step(bodies, dt);

    // Decay collision flash timers
    for (auto* body : bodies) {
        if (body->collisionFlash > 0.0f) {
            body->collisionFlash -= deltaTime;
            if (body->collisionFlash < 0.0f) body->collisionFlash = 0.0f;
        }
    }

    // Record trails periodically
    m_trailTimer += dt;
    if (m_trailTimer > 0.05f) {
        m_trailTimer = 0.0f;
        recordTrails();
    }
}

int SolarSystem::handleCollisions() {
    int collisionCount = 0;
    auto bodies = allBodiesMut();

    // Check all pairs for overlap
    for (size_t i = 0; i < bodies.size(); ++i) {
        if (bodies[i]->markedForRemoval) continue;

        for (size_t j = i + 1; j < bodies.size(); ++j) {
            if (bodies[j]->markedForRemoval) continue;

            float dist = glm::length(bodies[i]->position - bodies[j]->position);
            float minDist = bodies[i]->radius + bodies[j]->radius;

            if (dist < minDist * 0.8f) {
                // Collision! Larger body absorbs smaller.
                CelestialBody* larger = bodies[i];
                CelestialBody* smaller = bodies[j];
                if (smaller->mass > larger->mass) {
                    std::swap(larger, smaller);
                }

                // Conservation of momentum
                glm::vec3 totalMomentum = larger->mass * larger->velocity +
                                          smaller->mass * smaller->velocity;
                float totalMass = larger->mass + smaller->mass;
                larger->velocity = totalMomentum / totalMass;
                larger->mass = totalMass;

                // Grow radius (volume addition)
                float r1 = larger->radius;
                float r2 = smaller->radius;
                larger->radius = std::cbrt(r1 * r1 * r1 + r2 * r2 * r2);

                // Visual feedback
                larger->collisionFlash = 1.0f;

                // Mark smaller for removal
                smaller->markedForRemoval = true;
                collisionCount++;
            }
        }
    }

    // Remove marked bodies
    if (collisionCount > 0) {
        // Remove marked moons from planets
        for (auto& planet : m_planets) {
            planet.moons.erase(
                std::remove_if(planet.moons.begin(), planet.moons.end(),
                    [](const CelestialBody& m) { return m.markedForRemoval; }),
                planet.moons.end());
        }

        // Remove marked planets
        m_planets.erase(
            std::remove_if(m_planets.begin(), m_planets.end(),
                [](const CelestialBody& p) { return p.markedForRemoval; }),
            m_planets.end());
    }

    return collisionCount;
}

void SolarSystem::recordTrails() {
    for (auto& planet : m_planets) {
        planet.trail.push_back(planet.position);
        if (planet.trail.size() > CelestialBody::MAX_TRAIL_POINTS) {
            planet.trail.pop_front();
        }
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
        glm::vec3 oc = body->position - origin;
        float t = glm::dot(oc, direction);
        if (t < 0.0f) continue;

        glm::vec3 closestPoint = origin + direction * t;
        float dist = glm::length(closestPoint - body->position);

        float hitRadius = body->radius * 2.0f;
        if (dist < hitRadius && t < closestDist) {
            closestDist = t;
            closest = body;
        }
    }

    if (!closest) {
        float bestAngle = 1e30f;
        for (const auto* body : bodies) {
            glm::vec3 toBody = glm::normalize(body->position - origin);
            float angle = std::acos(std::clamp(glm::dot(toBody, direction), -1.0f, 1.0f));
            if (angle < bestAngle && angle < 0.15f) {
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
    if (m_isBinary) {
        bodies.push_back(&m_companionStar);
    }
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
    if (m_isBinary) {
        bodies.push_back(&m_companionStar);
    }
    for (auto& p : m_planets) {
        bodies.push_back(&p);
        for (auto& m : p.moons) {
            bodies.push_back(&m);
        }
    }
    return bodies;
}

} // namespace usim
