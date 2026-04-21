#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>

namespace usim {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

/// GPU mesh: VAO + VBO + EBO. Owns the OpenGL resources.
class Mesh {
public:
    Mesh() = default;
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    /// Upload vertex and index data to the GPU.
    void upload(const std::vector<Vertex>& vertices,
                const std::vector<uint32_t>& indices);

    void draw() const;
    bool isValid() const { return m_vao != 0; }
    uint32_t indexCount() const { return m_indexCount; }

private:
    void cleanup();

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    uint32_t m_indexCount = 0;
};

} // namespace usim
