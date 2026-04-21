#include "rendering/OrbitTrail.h"
#include <algorithm>

namespace usim {

OrbitTrail::OrbitTrail() {
    m_shader = std::make_unique<Shader>("shaders/trail.vert", "shaders/trail.frag");

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

OrbitTrail::~OrbitTrail() {
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

void OrbitTrail::draw(const std::deque<glm::vec3>& points,
                       const glm::vec3& color,
                       const glm::mat4& view,
                       const glm::mat4& projection)
{
    if (points.size() < 2) return;

    // Build vertex data: position + alpha
    std::vector<float> data;
    data.reserve(points.size() * 4);

    float n = static_cast<float>(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        float alpha = static_cast<float>(i) / n; // 0 = oldest, 1 = newest
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

    glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(points.size()));
    glBindVertexArray(0);
}

} // namespace usim
