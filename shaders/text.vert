#version 410 core

layout(location = 0) in vec4 aVertex; // xy = position, zw = texcoord

uniform mat4 uProjection;

out vec2 vTexCoord;

void main() {
    gl_Position = uProjection * vec4(aVertex.xy, 0.0, 1.0);
    vTexCoord = aVertex.zw;
}
