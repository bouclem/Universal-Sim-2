#include "rendering/RingMesh.h"
#include <cmath>
#include <vector>

namespace usim {

RingMesh::RingMesh(int segments) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Generate annulus vertices: inner ring at radius 0, outer at radius 1.
    // Actual inner/outer radii are set via shader uniforms.
    // We store the radial parameter (0=inner, 1=outer) in the normal.x
    // so the shader can compute the actual position.
    const float twoPi = 6.2831853f;

    for (int i = 0; i <= segments; ++i) {
        float angle = static_cast<float>(i) / static_cast<float>(segments) * twoPi;
        float cosA = std::cos(angle);
        float sinA = std::sin(angle);

        // Inner vertex (normal.x = 0 marks inner edge)
        vertices.push_back({
            glm::vec3(cosA, 0.0f, sinA),
            glm::vec3(0.0f, 1.0f, 0.0f) // normal up
        });

        // Outer vertex (normal.x = 1 marks outer edge)
        vertices.push_back({
            glm::vec3(cosA, 0.0f, sinA),
            glm::vec3(1.0f, 1.0f, 0.0f) // normal.x=1 flags outer
        });
    }

    // Build quads as triangle pairs
    for (int i = 0; i < segments; ++i) {
        uint32_t base = static_cast<uint32_t>(i * 2);
        // Triangle 1
        indices.push_back(base);
        indices.push_back(base + 2);
        indices.push_back(base + 1);
        // Triangle 2
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }

    m_mesh.upload(vertices, indices);
}

} // namespace usim
