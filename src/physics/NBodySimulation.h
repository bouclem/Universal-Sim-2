#pragma once

#include "scene/CelestialBody.h"
#include <vector>

namespace usim {

/// N-body gravitational simulation using Velocity Verlet integration.
class NBodySimulation {
public:
    /// Gravitational constant (v0.6.0: tuned for larger masses and wider orbits).
    static constexpr float G = 20.0f;

    /// Advance the simulation by deltaTime seconds.
    /// Operates on a flat list of body pointers for simplicity.
    void step(std::vector<CelestialBody*>& bodies, float deltaTime);

    /// Compute the circular orbital velocity for a body at a given distance
    /// from a central mass. Used for initial conditions.
    static glm::vec3 circularOrbitalVelocity(const glm::vec3& bodyPos,
                                              const glm::vec3& centralPos,
                                              float centralMass);

private:
    /// Compute gravitational acceleration on body i from all other bodies.
    static glm::vec3 computeAcceleration(const std::vector<CelestialBody*>& bodies,
                                          size_t i);

    /// Softening factor to prevent singularities at close range.
    static constexpr float SOFTENING = 0.5f;
};

} // namespace usim
