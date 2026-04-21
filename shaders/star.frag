#version 410 core

in vec3 vNormal;
in vec3 vWorldPos;

uniform vec3 uStarColor;
uniform vec3 uCameraPos;
uniform float uTime;

out vec4 FragColor;

// Simple hash for procedural noise in shader
float hash(vec3 p) {
    p = fract(p * vec3(443.897, 441.423, 437.195));
    p += dot(p, p.yzx + 19.19);
    return fract((p.x + p.y) * p.z);
}

float noise3D(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float n = mix(
        mix(mix(hash(i), hash(i + vec3(1,0,0)), f.x),
            mix(hash(i + vec3(0,1,0)), hash(i + vec3(1,1,0)), f.x), f.y),
        mix(mix(hash(i + vec3(0,0,1)), hash(i + vec3(1,0,1)), f.x),
            mix(hash(i + vec3(0,1,1)), hash(i + vec3(1,1,1)), f.x), f.y),
        f.z);
    return n;
}

void main() {
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(uCameraPos - vWorldPos);

    // Animated surface turbulence
    vec3 noiseCoord = normal * 3.0 + uTime * 0.05;
    float turbulence = noise3D(noiseCoord) * 0.5
                     + noise3D(noiseCoord * 2.0) * 0.25
                     + noise3D(noiseCoord * 4.0) * 0.125;

    // Base color with turbulence variation
    vec3 color = uStarColor * (0.85 + turbulence * 0.3);

    // Limb brightening (stars are brighter at center)
    float fresnel = dot(normal, viewDir);
    float limbFactor = 0.4 + 0.6 * pow(fresnel, 0.5);
    color *= limbFactor;

    // Emissive glow — stars emit light, no dark side
    color *= 1.2;

    FragColor = vec4(color, 1.0);
}
