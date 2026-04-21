#pragma once

#include "core/Shader.h"
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include <deque>
#include <memory>

namespace usim {

/// Renders orbit trails as fading line strips.
class OrbitTrail {
public:
    OrbitTrail();
    ~OrbitTrail();

    OrbitTrail(const OrbitTrail&) = delete;
    OrbitTrail& operator=(const OrbitTrail&) = delete;

    /// Draw a trail from a deque of positions.
    /// color: base trail color, alpha fades from newest to oldest.
    void draw(const std::deque<glm::vec3>& points,
              const glm::vec3& color,
              const glm::mat4& view,
              const glm::mat4& projection);

private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    std::unique_ptr<Shader> m_shader;
    size_t m_bufferCapacity = 0;
};

} // namespace usim
