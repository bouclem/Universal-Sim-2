#include "physics/NBodySimulation.h"
#include <cmath>

namespace usim {

void NBodySimulation::step(std::vector<CelestialBody*>& bodies, float deltaTime) {
    size_t n = bodies.size();
    if (n < 2) return;

    // Velocity Verlet integration:
    // 1. Compute current accelerations
    // 2. Update positions: x += v*dt + 0.5*a*dt^2
    // 3. Compute new accelerations
    // 4. Update velocities: v += 0.5*(a_old + a_new)*dt

    // Step 1: current accelerations
    std::vector<glm::vec3> accelOld(n);
    for (size_t i = 0; i < n; ++i) {
        accelOld[i] = computeAcceleration(bodies, i);
    }

    // Step 2: update positions
    for (size_t i = 0; i < n; ++i) {
        bodies[i]->position += bodies[i]->velocity * deltaTime
                             + 0.5f * accelOld[i] * deltaTime * deltaTime;
    }

    // Step 3: new accelerations at updated positions
    std::vector<glm::vec3> accelNew(n);
    for (size_t i = 0; i < n; ++i) {
        accelNew[i] = computeAcceleration(bodies, i);
    }

    // Step 4: update velocities
    for (size_t i = 0; i < n; ++i) {
        bodies[i]->velocity += 0.5f * (accelOld[i] + accelNew[i]) * deltaTime;
    }
}

glm::vec3 NBodySimulation::computeAcceleration(
    const std::vector<CelestialBody*>& bodies, size_t i)
{
    glm::vec3 accel(0.0f);
    const glm::vec3& posI = bodies[i]->position;

    for (size_t j = 0; j < bodies.size(); ++j) {
        if (j == i) continue;

        glm::vec3 diff = bodies[j]->position - posI;
        float distSq = glm::dot(diff, diff) + SOFTENING * SOFTENING;
        float dist = std::sqrt(distSq);
        float force = G * bodies[j]->mass / distSq;

        accel += (diff / dist) * force;
    }

    return accel;
}

glm::vec3 NBodySimulation::circularOrbitalVelocity(const glm::vec3& bodyPos,
                                                     const glm::vec3& centralPos,
                                                     float centralMass)
{
    glm::vec3 r = bodyPos - centralPos;
    float dist = glm::length(r);
    if (dist < 0.001f) return glm::vec3(0.0f);

    // v = sqrt(G * M / r) perpendicular to radius
    float speed = std::sqrt(G * centralMass / dist);

    // Direction: perpendicular to r in the XZ plane, then tilt by inclination
    glm::vec3 rNorm = glm::normalize(r);
    // Cross with up to get tangent direction
    glm::vec3 tangent = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), rNorm));

    // If r is nearly vertical, use a different reference
    if (glm::length(tangent) < 0.001f) {
        tangent = glm::normalize(glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), rNorm));
    }

    return tangent * speed;
}

} // namespace usim
