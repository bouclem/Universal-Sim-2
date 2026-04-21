#pragma once

#include "rendering/Mesh.h"
#include <glm/glm.hpp>
#include <vector>
#include <array>
#include <map>

namespace usim {

/// Generates icosphere meshes at multiple LOD levels.
/// LOD 0 = lowest detail (icosahedron), LOD 5 = highest detail.
/// Picks the right LOD based on screen-space projected size.
class LODSphere {
public:
    static constexpr int MAX_LOD = 5;
    static constexpr int LOD_COUNT = MAX_LOD + 1;

    LODSphere();

    /// Select the appropriate LOD level based on distance and object radius.
    /// Returns a value in [0, MAX_LOD].
    int selectLOD(float distance, float radius, float fovDegrees,
                  int screenHeight) const;

    /// Get the mesh for a specific LOD level.
    const Mesh& mesh(int lod) const;

private:
    void generateAll();
    void generateIcosphere(int subdivisions,
                           std::vector<Vertex>& outVertices,
                           std::vector<uint32_t>& outIndices);

    /// Subdivide a triangle into 4 smaller triangles.
    using EdgeKey = std::pair<uint32_t, uint32_t>;
    uint32_t getMiddlePoint(uint32_t p1, uint32_t p2,
                            std::vector<glm::vec3>& positions,
                            std::map<EdgeKey, uint32_t>& cache);

    std::array<Mesh, LOD_COUNT> m_meshes;
};

} // namespace usim
