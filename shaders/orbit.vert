#version 410 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in float aAlpha;

uniform mat4 uView;
uniform mat4 uProjection;

out float vAlpha;

void main() {
    gl_Position = uProjection * uView * vec4(aPosition, 1.0);
    vAlpha = aAlpha;
}
