#version 410 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;
layout(location = 2) in float aLuminosity;

uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uCameraPos;
uniform float uPointScale; // Screen-space scaling factor

out vec3 vColor;
out float vLuminosity;

void main() {
    gl_Position = uProjection * uView * vec4(aPosition, 1.0);

    // Point size based on distance and luminosity
    float dist = length(aPosition - uCameraPos);
    float size = uPointScale * (1.0 + sqrt(aLuminosity)) / max(dist * 0.01, 0.1);
    gl_PointSize = clamp(size, 1.0, 20.0);

    vColor = aColor;
    vLuminosity = aLuminosity;
}
