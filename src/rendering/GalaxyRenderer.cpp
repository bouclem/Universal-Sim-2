#include "rendering/GalaxyRenderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>

namespace usim {

GalaxyRenderer::GalaxyRenderer() {
    m_starShader = std::make_unique<Shader>(
        "shaders/galaxy_star.vert", "shaders/galaxy_star.frag");
    m_dustShader = std::make_unique<Shader>(
        "shaders/galaxy_dust.vert", "shaders/galaxy_dust.frag");

    // Create star VAO
    glGenVertexArrays(1, &m_starVao);
    glGenBuffers(1, &m_starVbo);

    glBindVertexArray(m_starVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_starVbo);

    // Layout: position(3) + color(3) + luminosity(1) = 7 floats
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
                          reinterpret_cast<void*>(6 * sizeof(float)));

    glBindVertexArray(0);
}

GalaxyRenderer::~GalaxyRenderer() {
    if (m_starVbo) glDeleteBuffers(1, &m_starVbo);
    if (m_starVao) glDeleteVertexArrays(1, &m_starVao);
}

void GalaxyRenderer::uploadStars(const Galaxy& galaxy) {
    const auto& stars = galaxy.stars();
    m_starCount = static_cast<int>(stars.size());

    std::vector<float> data;
    data.reserve(stars.size() * 7);

    for (const auto& star : stars) {
        data.push_back(star.position.x);
        data.push_back(star.position.y);
        data.push_back(star.position.z);
        data.push_back(star.color.r);
        data.push_back(star.color.g);
        data.push_back(star.color.b);
        data.push_back(star.luminosity);
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_starVbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(data.size() * sizeof(float)),
                 data.data(), GL_STATIC_DRAW);

    // Build dust mesh sized to the galaxy
    buildDustMesh(galaxy.radius());
}

void GalaxyRenderer::drawStars(const Galaxy& galaxy, const glm::vec3& cameraPos,
                                ViewScale scale,
                                const glm::mat4& view, const glm::mat4& projection)
{
    if (m_starCount <= 0) return;

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending for stars

    m_starShader->use();
    m_starShader->setMat4("uView", view);
    m_starShader->setMat4("uProjection", projection);
    m_starShader->setVec3("uCameraPos", cameraPos);

    // Adjust point scale based on viewing scale
    float pointScale;
    switch (scale) {
        case ViewScale::Galaxy:
            pointScale = 30.0f;
            break;
        case ViewScale::StarField:
            pointScale = 80.0f;
            break;
        default:
            pointScale = 50.0f;
            break;
    }
    m_starShader->setFloat("uPointScale", pointScale);

    glBindVertexArray(m_starVao);
    glDrawArrays(GL_POINTS, 0, m_starCount);
    glBindVertexArray(0);

    glDisable(GL_PROGRAM_POINT_SIZE);
    glDisable(GL_BLEND);
}

void GalaxyRenderer::drawDust(const Galaxy& galaxy,
                               const glm::mat4& view, const glm::mat4& projection)
{
    if (!m_dustMesh.isValid()) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), galaxy.center());

    m_dustShader->use();
    m_dustShader->setMat4("uModel", model);
    m_dustShader->setMat4("uView", view);
    m_dustShader->setMat4("uProjection", projection);
    m_dustShader->setVec3("uGalaxyCenter", galaxy.center());
    m_dustShader->setFloat("uGalaxyRadius", galaxy.radius());

    m_dustMesh.draw();

    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
}

void GalaxyRenderer::buildDustMesh(float radius) {
    // Flat disk with subdivisions for the galaxy dust cloud
    const int rings = 40;
    const int segments = 64;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Center vertex
    vertices.push_back({glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)});

    for (int r = 1; r <= rings; ++r) {
        float t = static_cast<float>(r) / static_cast<float>(rings);
        float ringRadius = t * radius;

        for (int s = 0; s < segments; ++s) {
            float angle = static_cast<float>(s) / static_cast<float>(segments) * 6.2831853f;
            float x = ringRadius * std::cos(angle);
            float z = ringRadius * std::sin(angle);

            vertices.push_back({glm::vec3(x, 0.0f, z), glm::vec3(0.0f, 1.0f, 0.0f)});
        }
    }

    // Triangles: center to first ring
    for (int s = 0; s < segments; ++s) {
        int next = (s + 1) % segments;
        indices.push_back(0);
        indices.push_back(static_cast<uint32_t>(1 + s));
        indices.push_back(static_cast<uint32_t>(1 + next));
    }

    // Triangles: ring to ring
    for (int r = 0; r < rings - 1; ++r) {
        int ringStart = 1 + r * segments;
        int nextRingStart = 1 + (r + 1) * segments;

        for (int s = 0; s < segments; ++s) {
            int next = (s + 1) % segments;

            indices.push_back(static_cast<uint32_t>(ringStart + s));
            indices.push_back(static_cast<uint32_t>(nextRingStart + s));
            indices.push_back(static_cast<uint32_t>(nextRingStart + next));

            indices.push_back(static_cast<uint32_t>(ringStart + s));
            indices.push_back(static_cast<uint32_t>(nextRingStart + next));
            indices.push_back(static_cast<uint32_t>(ringStart + next));
        }
    }

    m_dustMesh.upload(vertices, indices);
}

} // namespace usim
