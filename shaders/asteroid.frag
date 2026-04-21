#version 410 core

in vec3 vNormal;
in vec3 vWorldPos;
in vec3 vColor;

uniform vec3 uStarPos;
uniform vec3 uStarColor;

out vec4 FragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(uStarPos - vWorldPos);
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 ambient = vColor * 0.08;
    vec3 diffuse = vColor * diff * uStarColor;

    FragColor = vec4(ambient + diffuse, 1.0);
}
