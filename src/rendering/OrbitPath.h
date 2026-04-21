#pragma once

#include "core/Shader.h"
#include "scene/CelestialBody.h"
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace usim {

/// Renders predicted orbit ellipses as line loops.
class OrbitPath {
public:
    OrbitPath();
    ~OrbitPath();

    OrbitPath(const OrbitPath&) = delete;
    OrbitPath& operator=(const OrbitPath&) = delete;

    /// Draw a predicted orbit ellipse for a body orbiting a parent.
    /// Uses current velocity and position to compute the Keplerian ellipse.
    void draw(const CelestialBody& body,
              const glm::vec3& parentPos,
              float parentMass,
              float G,
              const glm::vec3& color,
              const glm::mat4& view,
              const glm::mat4& projection);

private:
    /// Compute points along the predicted orbit ellipse.
    std::vector<glm::vec3> computeOrbitPoints(
        const glm::vec3& pos,
        const glm::vec3& vel,
        const glm::vec3& parentPos,
        float parentMass,
        float G,
        int numPoints) const;

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    std::unique_ptr<Shader> m_shader;
    size_t m_bufferCapacity = 0;
};

} // namespace usim
