#pragma once

#include "core/Shader.h"
#include "rendering/Mesh.h"
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <cstdint>

namespace usim {

/// Renders an asteroid belt using instanced rendering.
class AsteroidBelt {
public:
    AsteroidBelt();
    ~AsteroidBelt();

    AsteroidBelt(const AsteroidBelt&) = delete;
    AsteroidBelt& operator=(const AsteroidBelt&) = delete;

    /// Generate asteroid positions for a belt between innerRadius and outerRadius
    /// centered at the given position (typically the star).
    void generate(uint32_t seed, const glm::vec3& center,
                  float innerRadius, float outerRadius, int count);

    /// Update asteroid positions (simple orbital motion).
    void update(float deltaTime, float timeScale,
                const glm::vec3& center, float centralMass, float G);

    /// Draw all asteroids.
    void draw(const glm::vec3& starPos, const glm::vec3& starColor,
              const glm::mat4& view, const glm::mat4& projection);

    int count() const { return m_count; }
    bool isGenerated() const { return m_count > 0; }

private:
    struct AsteroidInstance {
        glm::vec3 position;
        float scale;
        glm::vec3 color;
        // Orbital data
        float orbitalRadius;
        float orbitalAngle;
        float orbitalSpeed;
        float inclination;
        float yOffset;
    };

    void buildMesh();
    void updateInstanceBuffer();

    std::vector<AsteroidInstance> m_asteroids;
    int m_count = 0;

    // Mesh data (low-poly rock)
    GLuint m_meshVao = 0;
    GLuint m_meshVbo = 0;
    GLuint m_meshEbo = 0;
    uint32_t m_meshIndexCount = 0;

    // Instance buffer
    GLuint m_instanceVbo = 0;

    std::unique_ptr<Shader> m_shader;
};

} // namespace usim
