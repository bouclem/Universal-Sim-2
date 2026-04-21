#version 410 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uInnerRadius;
uniform float uOuterRadius;

out vec3 vWorldPos;
out float vRadialT; // 0 at inner edge, 1 at outer edge

void main() {
    // aNormal.x: 0 = inner vertex, 1 = outer vertex
    float isOuter = aNormal.x;
    float radius = mix(uInnerRadius, uOuterRadius, isOuter);
    vRadialT = isOuter;

    // aPosition is a unit circle direction on XZ plane
    vec3 localPos = aPosition * radius;
    localPos.y = 0.0;

    vec4 worldPos = uModel * vec4(localPos, 1.0);
    vWorldPos = worldPos.xyz;
    gl_Position = uProjection * uView * worldPos;
}
