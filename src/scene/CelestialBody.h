#pragma once

#include <glm/glm.hpp>

namespace usim {

/// Base data for any celestial body (star or planet).
struct CelestialBody {
    glm::vec3 position = glm::vec3(0.0f);
    float radius = 1.0f;
    bool isStar = false;

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
};

} // namespace usim
