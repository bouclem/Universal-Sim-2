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

    // Fresnel rim effect: atmosphere visible at edges
    float rim = 1.0 - max(dot(normal, viewDir), 0.0);
    rim = pow(rim, 2.5);

    // Rayleigh scattering approximation
    // Shorter wavelengths scatter more: blue scatters ~5.5x more than red
    float cosAngle = dot(viewDir, -lightDir);

    // Rayleigh phase function: (3/16pi)(1 + cos^2(theta))
    float rayleighPhase = 0.75 * (1.0 + cosAngle * cosAngle);

    // Mie scattering: strong forward scattering (sun glow through atmosphere)
    float mieG = 0.76;
    float miePhase = (1.0 - mieG * mieG) /
        (4.0 * 3.14159 * pow(1.0 + mieG * mieG - 2.0 * mieG * cosAngle, 1.5));

    // Diffuse lighting on the atmosphere shell
    float diff = max(dot(normal, lightDir), 0.0);

    // Wavelength-dependent scattering (Rayleigh)
    vec3 rayleighColor = uAtmosphereColor;
    // Enhance blue channel scattering
    rayleighColor.r *= 0.7;
    rayleighColor.b *= 1.3;

    // Sunset/sunrise coloring at the terminator
    float terminatorAngle = dot(normal, lightDir);
    float sunsetFactor = smoothstep(-0.1, 0.15, terminatorAngle) *
                         smoothstep(0.4, 0.15, terminatorAngle);
    vec3 sunsetColor = vec3(1.0, 0.4, 0.1) * uStarColor;

    // Combine scattering
    vec3 scatterColor = rayleighColor * rayleighPhase * 0.4 +
                        uStarColor * miePhase * 0.15;

    // Base atmosphere color
    vec3 color = uAtmosphereColor * uStarColor;
    color *= (diff * 0.5 + 0.12); // ambient + diffuse

    // Add scattering
    color += scatterColor * rim;

    // Add sunset coloring
    color = mix(color, sunsetColor, sunsetFactor * rim * 0.4);

    // Alpha: rim-based with density
    float alpha = rim * uDensity;
    // Boost on lit side
    alpha *= (0.5 + diff * 0.5);
    // Mie forward scattering adds glow
    alpha += miePhase * 0.05 * rim;
    alpha = clamp(alpha, 0.0, 0.85);

    // Slight color shift toward orange at very edge (atmospheric extinction)
    float edgeExtinction = pow(rim, 4.0);
    color = mix(color, color * vec3(1.2, 0.9, 0.7), edgeExtinction * 0.3);

    FragColor = vec4(color, alpha);
}
