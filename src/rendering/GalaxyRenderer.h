#pragma once

#include "core/Shader.h"
#include "rendering/Mesh.h"
#include "scene/Galaxy.h"
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <memory>

namespace usim {

/// Renders the galaxy at different scales:
/// - Galaxy view: dust cloud + star points
/// - Star field: individual stars as glowing points
class GalaxyRenderer {
public:
    GalaxyRenderer();
    ~GalaxyRenderer();

    GalaxyRenderer(const GalaxyRenderer&) = delete;
    GalaxyRenderer& operator=(const GalaxyRenderer&) = delete;

    /// Upload star data from the galaxy.
    void uploadStars(const Galaxy& galaxy);

    /// Draw stars as point sprites.
    void drawStars(const Galaxy& galaxy, const glm::vec3& cameraPos,
                   ViewScale scale,
                   const glm::mat4& view, const glm::mat4& projection);

    /// Draw the galaxy dust cloud (only in Galaxy view).
    void drawDust(const Galaxy& galaxy,
                  const glm::mat4& view, const glm::mat4& projection);

private:
    void buildDustMesh(float radius);

    // Star point rendering
    GLuint m_starVao = 0;
    GLuint m_starVbo = 0;
    int m_starCount = 0;
    std::unique_ptr<Shader> m_starShader;

    // Galaxy dust cloud (flat disk mesh)
    Mesh m_dustMesh;
    std::unique_ptr<Shader> m_dustShader;
};

} // namespace usim
