#include "scene/Galaxy.h"
#include "procedural/StarGenerator.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace usim {

namespace {

float hashFloat(uint32_t seed, uint32_t salt) {
    uint32_t h = seed ^ (salt * 2654435761u);
    h = ((h >> 16) ^ h) * 0x45d9f3bu;
    h = ((h >> 16) ^ h) * 0x45d9f3bu;
    h = (h >> 16) ^ h;
    return static_cast<float>(h) / static_cast<float>(0xFFFFFFFFu);
}

std::string generateStarName(uint32_t seed) {
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
    int num = static_cast<int>(hashFloat(seed, 503) * 900.0f) + 100;
    name += "-" + std::to_string(num);
    return name;
}

} // anonymous namespace

void Galaxy::generate(uint32_t seed, int starCount) {
    m_seed = seed;
    m_stars.clear();
    m_stars.reserve(static_cast<size_t>(starCount));
    m_activeStarIndex = -1;
    m_activeSystem.reset();
    m_activeBelt.reset();
    m_center = glm::vec3(0.0f);
    m_radius = 5000.0f;

    // Generate spiral arms (4 arms)
    int starsPerArm = starCount / 5; // 4 arms + core
    int coreStars = starCount - starsPerArm * 4;

    for (int arm = 0; arm < 4; ++arm) {
        float armAngle = static_cast<float>(arm) * 1.5707963f; // 90 degrees apart
        uint32_t armSeed = seed ^ (static_cast<uint32_t>(arm + 1) * 7919u);
        generateSpiralArm(armSeed, armAngle, starsPerArm, m_radius);
    }

    // Core / bulge stars (denser, more random)
    for (int i = 0; i < coreStars; ++i) {
        uint32_t starSeed = seed ^ (static_cast<uint32_t>(i + 10000) * 6271u);

        GalaxyStar star{};
        star.seed = starSeed;

        // Core distribution: Gaussian-like, concentrated at center
        float r = hashFloat(starSeed, 1);
        r = r * r * m_radius * 0.3f; // Concentrated in inner 30%
        float angle = hashFloat(starSeed, 2) * 6.2831853f;
        float height = (hashFloat(starSeed, 3) - 0.5f) * m_radius * 0.05f;

        star.position = m_center + glm::vec3(
            r * std::cos(angle),
            height,
            r * std::sin(angle)
        );

        // Star properties
        auto props = StarGenerator::generate(starSeed);
        star.color = props.color;
        star.temperature = props.temperature;
        star.luminosity = props.luminosity;
        star.radius = props.radius;
        star.name = generateStarName(starSeed);

        // Binary chance: ~15%
        star.isBinary = hashFloat(starSeed, 10) > 0.85f;

        m_stars.push_back(star);
    }

    std::cout << "Galaxy generated: " << m_stars.size() << " stars, seed=" << seed << "\n";
}

void Galaxy::generateSpiralArm(uint32_t armSeed, float armAngle, int count,
                                float galaxyRadius)
{
    for (int i = 0; i < count; ++i) {
        uint32_t starSeed = armSeed ^ (static_cast<uint32_t>(i) * 3571u);

        GalaxyStar star{};
        star.seed = starSeed;

        // Position along the spiral arm
        float t = static_cast<float>(i) / static_cast<float>(count);

        // Logarithmic spiral: r = a * e^(b*theta)
        float spiralAngle = armAngle + t * 4.0f * 3.14159f; // ~2 full turns
        float r = galaxyRadius * 0.1f + t * galaxyRadius * 0.85f;

        // Scatter perpendicular to the arm
        float scatter = (hashFloat(starSeed, 1) - 0.5f) * galaxyRadius * 0.12f;
        float scatterPerp = (hashFloat(starSeed, 2) - 0.5f) * galaxyRadius * 0.08f;

        // Vertical scatter (thin disk)
        float height = (hashFloat(starSeed, 3) - 0.5f) * galaxyRadius * 0.02f;
        // Thicker at center, thinner at edges
        height *= (1.0f - t * 0.7f);

        float x = (r + scatter) * std::cos(spiralAngle) + scatterPerp * std::sin(spiralAngle);
        float z = (r + scatter) * std::sin(spiralAngle) - scatterPerp * std::cos(spiralAngle);

        star.position = m_center + glm::vec3(x, height, z);

        // Star properties
        auto props = StarGenerator::generate(starSeed);
        star.color = props.color;
        star.temperature = props.temperature;
        star.luminosity = props.luminosity;
        star.radius = props.radius;
        star.name = generateStarName(starSeed);

        // Binary chance
        star.isBinary = hashFloat(starSeed, 10) > 0.85f;

        m_stars.push_back(star);
    }
}

ViewScale Galaxy::determineScale(const glm::vec3& cameraPos) const {
    // If we have an active system, check distance to its star
    if (m_activeStarIndex >= 0) {
        float distToStar = glm::length(cameraPos - m_stars[static_cast<size_t>(m_activeStarIndex)].position);
        if (distToStar < SYSTEM_EXIT_DIST) {
            return ViewScale::SolarSystem;
        }
    }

    // Check distance to nearest star
    int nearest = findNearestStar(cameraPos);
    if (nearest >= 0) {
        float dist = glm::length(cameraPos - m_stars[static_cast<size_t>(nearest)].position);
        if (dist < SYSTEM_ENTER_DIST) {
            return ViewScale::SolarSystem;
        }
    }

    // Check distance from galaxy center
    float distFromCenter = glm::length(cameraPos - m_center);
    if (distFromCenter > GALAXY_VIEW_DIST) {
        return ViewScale::Galaxy;
    }

    return ViewScale::StarField;
}

int Galaxy::findNearestStar(const glm::vec3& pos) const {
    int nearest = -1;
    float minDist = 1e30f;

    for (size_t i = 0; i < m_stars.size(); ++i) {
        float dist = glm::length(pos - m_stars[i].position);
        if (dist < minDist) {
            minDist = dist;
            nearest = static_cast<int>(i);
        }
    }

    return nearest;
}

bool Galaxy::updateActiveSystem(const glm::vec3& cameraPos) {
    int nearest = findNearestStar(cameraPos);
    if (nearest < 0) return false;

    float dist = glm::length(cameraPos - m_stars[static_cast<size_t>(nearest)].position);

    // Should we enter a system?
    if (dist < SYSTEM_ENTER_DIST && nearest != m_activeStarIndex) {
        m_activeStarIndex = nearest;
        const auto& gStar = m_stars[static_cast<size_t>(nearest)];

        // Generate the solar system for this star
        m_activeSystem = std::make_unique<SolarSystem>();
        m_activeSystem->generate(gStar.seed, gStar.isBinary);

        // Override the star's position to match galaxy position
        m_activeSystem->star().position = gStar.position;

        // Offset all bodies relative to the star's galaxy position
        glm::vec3 offset = gStar.position;
        for (auto& planet : m_activeSystem->planets()) {
            planet.position += offset;
            for (auto& moon : planet.moons) {
                moon.position += offset;
            }
        }

        // Regenerate asteroid belt at the correct position
        m_activeBelt = std::make_unique<AsteroidBelt>();
        m_activeBelt->generate(gStar.seed, gStar.position,
                               m_activeSystem->asteroidBeltInner(),
                               m_activeSystem->asteroidBeltOuter(),
                               2000);

        std::cout << "Entered system: " << gStar.name
                  << " (planets: " << m_activeSystem->planets().size() << ")\n";
        return true;
    }

    // Should we exit the current system?
    if (m_activeStarIndex >= 0) {
        float distToActive = glm::length(
            cameraPos - m_stars[static_cast<size_t>(m_activeStarIndex)].position);
        if (distToActive > SYSTEM_EXIT_DIST) {
            std::cout << "Left system: " << m_stars[static_cast<size_t>(m_activeStarIndex)].name << "\n";
            m_activeStarIndex = -1;
            m_activeSystem.reset();
            m_activeBelt.reset();
            return true;
        }
    }

    return false;
}

void Galaxy::updatePhysics(float deltaTime, float timeScale) {
    if (m_activeSystem) {
        m_activeSystem->update(deltaTime, timeScale);
        m_activeSystem->handleCollisions();

        if (m_activeBelt && m_activeStarIndex >= 0) {
            m_activeBelt->update(deltaTime, timeScale,
                                 m_stars[static_cast<size_t>(m_activeStarIndex)].position,
                                 m_activeSystem->star().mass,
                                 NBodySimulation::G);
        }
    }
}

const GalaxyStar* Galaxy::activeStar() const {
    if (m_activeStarIndex >= 0 && m_activeStarIndex < static_cast<int>(m_stars.size())) {
        return &m_stars[static_cast<size_t>(m_activeStarIndex)];
    }
    return nullptr;
}

} // namespace usim
