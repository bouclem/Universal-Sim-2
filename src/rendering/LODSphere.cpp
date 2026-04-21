#include "rendering/LODSphere.h"
#include <cmath>
#include <algorithm>

namespace usim {

LODSphere::LODSphere() {
    generateAll();
}

int LODSphere::selectLOD(float distance, float radius, float fovDegrees,
                         int screenHeight) const
{
    if (distance <= 0.0f) return MAX_LOD;

    // Compute approximate screen-space pixel size of the object
    float fovRad = glm::radians(fovDegrees);
    float projectedSize = (radius / distance) * static_cast<float>(screenHeight)
                          / (2.0f * std::tan(fovRad * 0.5f));

    // Map pixel size to LOD level (v0.6.0: 8 levels for close-up detail)
    // Larger on screen -> higher LOD
    if (projectedSize > 600.0f) return 7;
    if (projectedSize > 400.0f) return 6;
    if (projectedSize > 250.0f) return 5;
    if (projectedSize > 140.0f) return 4;
    if (projectedSize > 60.0f)  return 3;
    if (projectedSize > 25.0f)  return 2;
    if (projectedSize > 8.0f)   return 1;
    return 0;
}

const Mesh& LODSphere::mesh(int lod) const {
    int clamped = std::clamp(lod, 0, MAX_LOD);
    return m_meshes[static_cast<size_t>(clamped)];
}

void LODSphere::generateAll() {
    for (int lod = 0; lod < LOD_COUNT; ++lod) {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        generateIcosphere(lod, vertices, indices);
        m_meshes[static_cast<size_t>(lod)].upload(vertices, indices);
    }
}

void LODSphere::generateIcosphere(int subdivisions,
                                   std::vector<Vertex>& outVertices,
                                   std::vector<uint32_t>& outIndices)
{
    // Start with an icosahedron
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

    std::vector<uint32_t> triangles = {
        0,11,5,  0,5,1,   0,1,7,   0,7,10,  0,10,11,
        1,5,9,   5,11,4,  11,10,2, 10,7,6,  7,1,8,
        3,9,4,   3,4,2,   3,2,6,   3,6,8,   3,8,9,
        4,9,5,   2,4,11,  6,2,10,  8,6,7,   9,8,1,
    };

    // Subdivide
    std::map<EdgeKey, uint32_t> cache;
    for (int i = 0; i < subdivisions; ++i) {
        std::vector<uint32_t> newTriangles;
        cache.clear();

        for (size_t j = 0; j < triangles.size(); j += 3) {
            uint32_t v0 = triangles[j];
            uint32_t v1 = triangles[j + 1];
            uint32_t v2 = triangles[j + 2];

            uint32_t a = getMiddlePoint(v0, v1, positions, cache);
            uint32_t b = getMiddlePoint(v1, v2, positions, cache);
            uint32_t c = getMiddlePoint(v2, v0, positions, cache);

            newTriangles.insert(newTriangles.end(), {v0, a, c});
            newTriangles.insert(newTriangles.end(), {v1, b, a});
            newTriangles.insert(newTriangles.end(), {v2, c, b});
            newTriangles.insert(newTriangles.end(), {a, b, c});
        }
        triangles = std::move(newTriangles);
    }

    // Build output
    outVertices.reserve(positions.size());
    for (const auto& p : positions) {
        outVertices.push_back({p, p}); // Normal = position for unit sphere
    }
    outIndices = std::move(triangles);
}

uint32_t LODSphere::getMiddlePoint(uint32_t p1, uint32_t p2,
                                    std::vector<glm::vec3>& positions,
                                    std::map<EdgeKey, uint32_t>& cache)
{
    EdgeKey key = (p1 < p2) ? EdgeKey{p1, p2} : EdgeKey{p2, p1};
    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second;
    }

    glm::vec3 mid = glm::normalize(positions[p1] + positions[p2]);
    auto idx = static_cast<uint32_t>(positions.size());
    positions.push_back(mid);
    cache[key] = idx;
    return idx;
}

} // namespace usim
