#version 410 core

in float vAlpha;

uniform vec3 uColor;

out vec4 FragColor;

void main() {
    FragColor = vec4(uColor, vAlpha * 0.7);
}
