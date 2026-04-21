#pragma once

#include "core/Shader.h"
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <memory>

namespace usim {

/// Simple bitmap font text renderer for HUD overlays.
/// Uses a procedurally generated 8x8 pixel font atlas.
class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    /// Render a string at screen position (pixels from top-left).
    /// screenW/screenH: current viewport dimensions.
    void renderText(const std::string& text, float x, float y,
                    float scale, const glm::vec3& color,
                    int screenW, int screenH);

private:
    void generateFontAtlas();
    void setupQuad();

    GLuint m_fontTexture = 0;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    std::unique_ptr<Shader> m_shader;

    static constexpr int GLYPH_W = 8;
    static constexpr int GLYPH_H = 8;
    static constexpr int ATLAS_COLS = 16;
    static constexpr int ATLAS_ROWS = 8;
    static constexpr int ATLAS_W = GLYPH_W * ATLAS_COLS; // 128
    static constexpr int ATLAS_H = GLYPH_H * ATLAS_ROWS; // 64
};

} // namespace usim
