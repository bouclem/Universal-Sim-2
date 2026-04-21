#version 410 core

in vec2 vTexCoord;

uniform sampler2D uFontAtlas;
uniform vec3 uColor;

out vec4 FragColor;

void main() {
    float alpha = texture(uFontAtlas, vTexCoord).r;
    if (alpha < 0.5) discard;
    FragColor = vec4(uColor, alpha);
}
