#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 0) out vec4 outColor;

void main() {
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 norm = normalize(fragNormal);
    float diff = max(dot(norm, lightDir), 0.0);
    float ambient = 0.15;
    float brightness = ambient + diff * 0.85;
    vec3 starColor = vec3(1.0, 0.9, 0.2);
    outColor = vec4(starColor * brightness, 1.0);
}
