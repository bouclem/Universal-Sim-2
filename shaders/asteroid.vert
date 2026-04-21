#version 410 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

// Per-instance data
layout(location = 2) in vec3 aInstancePos;
layout(location = 3) in float aInstanceScale;
layout(location = 4) in vec3 aInstanceColor;

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vNormal;
out vec3 vWorldPos;
out vec3 vColor;

void main() {
    vec3 worldPos = aPosition * aInstanceScale + aInstancePos;
    vWorldPos = worldPos;
    vNormal = aNormal; // Approximate: no rotation
    vColor = aInstanceColor;
    gl_Position = uProjection * uView * vec4(worldPos, 1.0);
}
