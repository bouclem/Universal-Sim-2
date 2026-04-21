#include "rendering/AsteroidBelt.h"
#include <cmath>
#include <algorithm>

namespace usim {

namespace {

float hashFloat(uint32_t seed, uint32_t salt) {
    uint32_t h = seed ^ (salt * 2654435761u);
    h = ((h >> 16) ^ h) * 0x45d9f3bu;
    h = ((h >> 16) ^ h) * 0x45d9f3bu;
    h = (h >> 16) ^ h;
    return static_cast<float>(h) / static_cast<float>(0xFFFFFFFFu);
}

} // anonymous namespace

AsteroidBelt::AsteroidBelt() {
    m_shader = std::make_unique<Shader>(
        "shaders/asteroid.vert", "shaders/asteroid.frag");
    buildMesh();
}

AsteroidBelt::~AsteroidBelt() {
    if (m_instanceVbo) glDeleteBuffers(1, &m_instanceVbo);
    if (m_meshEbo) glDeleteBuffers(1, &m_meshEbo);
    if (m_meshVbo) glDeleteBuffers(1, &m_meshVbo);
    if (m_meshVao) glDeleteVertexArrays(1, &m_meshVao);
}

void AsteroidBelt::buildMesh() {
    // Low-poly irregular rock (deformed icosahedron, 0 subdivisions)
    const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;

    std::vector<glm::vec3> positions = {
        glm::normalize(glm::vec3(-1,  t,  0)),
        glm::normalize(glm::vec3( 1,  t,  0)),
        glm::normalize(glm::vec3(-1, -t,  0)),
        glm::normalize(glm::vec3( 1, -t,  0)),
        glm::normalize(glm::vec3( 0, -1,  t)),
        glm::normalize(glm::vec3( 0,  1,  t)),
        glm::normalize(glm::vec3( 0, -1, -t)),
        glm::normalize(glm::vec3( 0,  1, -t)),
        glm::normalize(glm::vec3( t,  0, -1)),
        glm::normalize(glm::vec3( t,  0,  1)),
        glm::normalize(glm::vec3(-t,  0, -1)),
        glm::normalize(glm::vec3(-t,  0,  1)),
    };

    // Deform slightly for irregular rock shape
    for (size_t i = 0; i < positions.size(); ++i) {
        float deform = 0.7f + 0.6f * hashFloat(static_cast<uint32_t>(i), 999);
        positions[i] *= deform;
    }

    std::vector<uint32_t> indices = {
        0,11,5,  0,5,1,   0,1,7,   0,7,10,  0,10,11,
        1,5,9,   5,11,4,  11,10,2, 10,7,6,  7,1,8,
        3,9,4,   3,4,2,   3,2,6,   3,6,8,   3,8,9,
        4,9,5,   2,4,11,  6,2,10,  8,6,7,   9,8,1,
    };

    // Build vertex data with normals
    struct MeshVertex {
        glm::vec3 position;
        glm::vec3 normal;
    };

    std::vector<MeshVertex> vertices;
    vertices.reserve(positions.size());
    for (const auto& p : positions) {
        vertices.push_back({p, glm::normalize(p)});
    }

    m_meshIndexCount = static_cast<uint32_t>(indices.size());

    // Create VAO with mesh + instance attributes
    glGenVertexArrays(1, &m_meshVao);
    glGenBuffers(1, &m_meshVbo);
    glGenBuffers(1, &m_meshEbo);
    glGenBuffers(1, &m_instanceVbo);

    glBindVertexArray(m_meshVao);

    // Mesh vertex data
    glBindBuffer(GL_ARRAY_BUFFER, m_meshVbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(MeshVertex)),
                 vertices.data(), GL_STATIC_DRAW);

    // Position (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          reinterpret_cast<void*>(0));
    // Normal (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          reinterpret_cast<void*>(sizeof(glm::vec3)));

    // Index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_meshEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
                 indices.data(), GL_STATIC_DRAW);

    // Instance data buffer (will be filled later)
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVbo);

    // Instance position (location 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
                          reinterpret_cast<void*>(0));
    glVertexAttribDivisor(2, 1);

    // Instance scale (location 3)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));
    glVertexAttribDivisor(3, 1);

    // Instance color (location 4)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
                          reinterpret_cast<void*>(4 * sizeof(float)));
    glVertexAttribDivisor(4, 1);

    glBindVertexArray(0);
}

void AsteroidBelt::generate(uint32_t seed, const glm::vec3& center,
                             float innerRadius, float outerRadius, int count)
{
    m_asteroids.clear();
    m_asteroids.reserve(static_cast<size_t>(count));
    m_count = count;

    for (int i = 0; i < count; ++i) {
        uint32_t as = seed ^ (static_cast<uint32_t>(i) * 6271u);
        AsteroidInstance ast{};

        // Random orbital radius within the belt
        float t = hashFloat(as, 1);
        // Bias toward middle of belt
        t = 0.5f + (t - 0.5f) * 0.8f;
        ast.orbitalRadius = innerRadius + t * (outerRadius - innerRadius);

        // Random angle
        ast.orbitalAngle = hashFloat(as, 2) * 6.2831853f;

        // Orbital speed (Kepler: faster closer in)
        ast.orbitalSpeed = 1.0f / std::pow(ast.orbitalRadius / innerRadius, 1.5f);

        // Slight inclination and vertical offset
        ast.inclination = (hashFloat(as, 3) - 0.5f) * 0.15f;
        ast.yOffset = (hashFloat(as, 4) - 0.5f) * (outerRadius - innerRadius) * 0.08f;

        // Scale: mostly tiny, few larger
        float sizeRoll = hashFloat(as, 5);
        ast.scale = 0.02f + sizeRoll * sizeRoll * 0.15f;

        // Color: grey-brown rocky
        float grey = 0.25f + hashFloat(as, 6) * 0.25f;
        float warmth = hashFloat(as, 7) * 0.1f;
        ast.color = glm::vec3(grey + warmth, grey, grey - warmth * 0.5f);

        // Initial position
        float cosA = std::cos(ast.orbitalAngle);
        float sinA = std::sin(ast.orbitalAngle);
        float cosI = std::cos(ast.inclination);
        float sinI = std::sin(ast.inclination);
        ast.position = center + glm::vec3(
            cosA * ast.orbitalRadius,
            sinA * ast.orbitalRadius * sinI + ast.yOffset,
            sinA * ast.orbitalRadius * cosI
        );

        m_asteroids.push_back(ast);
    }

    updateInstanceBuffer();
}

void AsteroidBelt::update(float deltaTime, float timeScale,
                           const glm::vec3& center, float /*centralMass*/,
                           float /*G*/)
{
    float dt = deltaTime * timeScale;

    for (auto& ast : m_asteroids) {
        ast.orbitalAngle += ast.orbitalSpeed * dt * 0.3f;

        float cosA = std::cos(ast.orbitalAngle);
        float sinA = std::sin(ast.orbitalAngle);
        float cosI = std::cos(ast.inclination);
        float sinI = std::sin(ast.inclination);
        ast.position = center + glm::vec3(
            cosA * ast.orbitalRadius,
            sinA * ast.orbitalRadius * sinI + ast.yOffset,
            sinA * ast.orbitalRadius * cosI
        );
    }

    updateInstanceBuffer();
}

void AsteroidBelt::updateInstanceBuffer() {
    if (m_asteroids.empty()) return;

    // Pack instance data: position(3) + scale(1) + color(3) = 7 floats
    std::vector<float> data;
    data.reserve(m_asteroids.size() * 7);

    for (const auto& ast : m_asteroids) {
        data.push_back(ast.position.x);
        data.push_back(ast.position.y);
        data.push_back(ast.position.z);
        data.push_back(ast.scale);
        data.push_back(ast.color.r);
        data.push_back(ast.color.g);
        data.push_back(ast.color.b);
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVbo);
    auto dataSize = static_cast<GLsizeiptr>(data.size() * sizeof(float));
    glBufferData(GL_ARRAY_BUFFER, dataSize, data.data(), GL_DYNAMIC_DRAW);
}

void AsteroidBelt::draw(const glm::vec3& starPos, const glm::vec3& starColor,
                          const glm::mat4& view, const glm::mat4& projection)
{
    if (m_count <= 0) return;

    m_shader->use();
    m_shader->setMat4("uView", view);
    m_shader->setMat4("uProjection", projection);
    m_shader->setVec3("uStarPos", starPos);
    m_shader->setVec3("uStarColor", starColor);

    glBindVertexArray(m_meshVao);
    glDrawElementsInstanced(GL_TRIANGLES,
                            static_cast<GLsizei>(m_meshIndexCount),
                            GL_UNSIGNED_INT, nullptr, m_count);
    glBindVertexArray(0);
}

} // namespace usim
