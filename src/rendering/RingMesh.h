#pragma once

#include "rendering/Mesh.h"
#include <glm/glm.hpp>

namespace usim {

/// Generates a flat annulus (ring) mesh on the XZ plane.
class RingMesh {
public:
    RingMesh(int segments = 128);

    /// The mesh is a unit annulus (inner=0, outer=1).
    /// Scale via uniforms in the shader.
    const Mesh& mesh() const { return m_mesh; }

private:
    Mesh m_mesh;
};

} // namespace usim
