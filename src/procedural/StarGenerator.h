#pragma once

#include <glm/glm.hpp>
#include <cstdint>

namespace usim {

/// Properties of a procedurally generated star.
struct StarProperties {
    float temperature;   // Kelvin (2000 - 40000)
    float radius;        // Arbitrary units
    float luminosity;    // Relative brightness
    glm::vec3 color;     // RGB from blackbody approximation
};

/// Generates star properties from a seed.
class StarGenerator {
public:
    /// Generate star properties from a seed value.
    static StarProperties generate(uint32_t seed);

    /// Approximate blackbody color for a given temperature in Kelvin.
    static glm::vec3 blackbodyColor(float temperature);
};

} // namespace usim
