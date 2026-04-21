#version 410 core

in float vAlpha;

uniform vec3 uColor;
uniform float uDash; // 0 = solid, 1 = dashed

out vec4 FragColor;

void main() {
    float alpha = vAlpha * 0.5;
    FragColor = vec4(uColor, alpha);
}
