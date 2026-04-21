#include "rendering/OrbitPath.h"
#include <cmath>
#include <algorithm>

namespace usim {

OrbitPath::OrbitPath() {
    m_shader = std::make_unique<Shader>("shaders/orbit.vert", "shaders/orbit.frag");

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    // Position (vec3) + alpha (float) = 4 floats per vertex
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));

    glBindVertexArray(0);
}

OrbitPath::~OrbitPath() {
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

std::vector<glm::vec3> OrbitPath::computeOrbitPoints(
    const glm::vec3& pos,
    const glm::vec3& vel,
    const glm::vec3& parentPos,
    float parentMass,
    float G,
    int numPoints) const
{
    std::vector<glm::vec3> points;

    glm::vec3 r = pos - parentPos;
    float rMag = glm::length(r);
    if (rMag < 0.001f) return points;

    float vMag = glm::length(vel);
    float mu = G * parentMass;

    // Specific orbital energy
    float energy = 0.5f * vMag * vMag - mu / rMag;

    // If energy >= 0, orbit is hyperbolic/parabolic — draw a partial arc
    bool isBound = (energy < 0.0f);

    // Semi-major axis
    float a = -mu / (2.0f * energy);
    if (!isBound) {
        // For unbound orbits, just draw a short prediction arc
        a = rMag * 2.0f;
    }

    // Specific angular momentum
    glm::vec3 h = glm::cross(r, vel);
    float hMag = glm::length(h);
    if (hMag < 0.0001f) return points;

    // Eccentricity vector
    glm::vec3 eVec = glm::cross(vel, h) / mu - glm::normalize(r);
    float e = glm::length(eVec);

    // Orbital plane basis vectors
    glm::vec3 hNorm = glm::normalize(h);
    glm::vec3 periDir;
    if (e > 0.001f) {
        periDir = glm::normalize(eVec);
    } else {
        periDir = glm::normalize(r);
    }
    glm::vec3 sideDir = glm::cross(hNorm, periDir);

    // Generate points along the ellipse
    float angleRange = isBound ? 6.2831853f : 4.0f; // Full orbit or partial arc
    for (int i = 0; i <= numPoints; ++i) {
        float theta = (static_cast<float>(i) / static_cast<float>(numPoints)) * angleRange;
        if (!isBound) {
            theta -= 2.0f; // Center the arc
        }

        float rTheta;
        if (isBound && e < 1.0f) {
            // Elliptical orbit: r = a(1-e^2) / (1 + e*cos(theta))
            float denom = 1.0f + e * std::cos(theta);
            if (std::abs(denom) < 0.001f) continue;
            rTheta = a * (1.0f - e * e) / denom;
        } else {
            // Approximate for unbound
            float denom = 1.0f + e * std::cos(theta);
            if (denom <= 0.01f) continue;
            rTheta = hMag * hMag / (mu * denom);
            if (rTheta > a * 5.0f) continue; // Clip far points
        }

        if (rTheta < 0.0f || rTheta > 100000.0f) continue;

        glm::vec3 point = parentPos +
            periDir * (rTheta * std::cos(theta)) +
            sideDir * (rTheta * std::sin(theta));
        points.push_back(point);
    }

    return points;
}

void OrbitPath::draw(const CelestialBody& body,
                      const glm::vec3& parentPos,
                      float parentMass,
                      float G,
                      const glm::vec3& color,
                      const glm::mat4& view,
                      const glm::mat4& projection)
{
    auto points = computeOrbitPoints(
        body.position, body.velocity, parentPos, parentMass, G, 128);

    if (points.size() < 2) return;

    // Build vertex data: position + alpha
    std::vector<float> data;
    data.reserve(points.size() * 4);

    for (size_t i = 0; i < points.size(); ++i) {
        float alpha = 0.6f; // Uniform alpha for prediction lines
        data.push_back(points[i].x);
        data.push_back(points[i].y);
        data.push_back(points[i].z);
        data.push_back(alpha);
    }

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    auto dataSize = static_cast<GLsizeiptr>(data.size() * sizeof(float));
    if (data.size() > m_bufferCapacity) {
        glBufferData(GL_ARRAY_BUFFER, dataSize, data.data(), GL_DYNAMIC_DRAW);
        m_bufferCapacity = data.size();
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, data.data());
    }

    m_shader->use();
    m_shader->setMat4("uView", view);
    m_shader->setMat4("uProjection", projection);
    m_shader->setVec3("uColor", color);
    m_shader->setFloat("uDash", 0.0f);

    glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(points.size()));
    glBindVertexArray(0);
}

} // namespace usim
