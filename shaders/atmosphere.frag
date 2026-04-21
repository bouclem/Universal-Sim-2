#version 410 core

in vec3 vNormal;
in vec3 vWorldPos;

uniform vec3 uAtmosphereColor;
uniform float uDensity;
uniform vec3 uCameraPos;
uniform vec3 uStarPos;
uniform vec3 uStarColor;

out vec4 FragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(uCameraPos - vWorldPos);
    vec3 lightDir = normalize(uStarPos - vWorldPos);

    // Fresnel-like rim effect: atmosphere is most visible at the edges
    float rim = 1.0 - max(dot(normal, viewDir), 0.0);
    rim = pow(rim, 2.0);

    // Rayleigh-like scattering approximation
    // Forward scattering: brighter when looking toward the star through atmosphere
    float scatter = dot(viewDir, -lightDir) * 0.5 + 0.5;
    scatter = pow(scatter, 3.0) * 0.3;

    // Diffuse lighting on the atmosphere shell
    float diff = max(dot(normal, lightDir), 0.0);

    // Combine
    vec3 color = uAtmosphereColor * uStarColor;
    color *= (diff * 0.6 + scatter + 0.15); // ambient + diffuse + scatter

    float alpha = rim * uDensity;
    // Boost alpha slightly on the lit side
    alpha *= (0.6 + diff * 0.4);
    alpha = clamp(alpha, 0.0, 0.85);

    FragColor = vec4(color, alpha);
}
