#pragma once

#include <glm/glm.hpp>

namespace usim {

/// Simplex noise utilities for procedural generation.
namespace noise {

/// 3D simplex noise, returns value in [-1, 1].
float simplex3D(const glm::vec3& v);

/// Fractal Brownian Motion using simplex noise.
/// octaves: number of noise layers
/// persistence: amplitude decay per octave
/// lacunarity: frequency increase per octave
float fbm(const glm::vec3& v, int octaves = 6,
          float persistence = 0.5f, float lacunarity = 2.0f);

} // namespace noise
} // namespace usim
