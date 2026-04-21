#version 410 core

in vec3 vDirection;

out vec4 FragColor;

float hash(vec3 p) {
    p = fract(p * vec3(443.897, 441.423, 437.195));
    p += dot(p, p.yzx + 19.19);
    return fract((p.x + p.y) * p.z);
}

float hash1(float p) {
    p = fract(p * 443.897);
    p += p * (p + 19.19);
    return fract(p);
}

vec3 hashGrad(vec3 p) {
    p = vec3(dot(p, vec3(127.1, 311.7, 74.7)),
             dot(p, vec3(269.5, 183.3, 246.1)),
             dot(p, vec3(113.5, 271.9, 124.6)));
    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

float gradientNoise(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    vec3 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

    return mix(mix(mix(dot(hashGrad(i + vec3(0,0,0)), f - vec3(0,0,0)),
                       dot(hashGrad(i + vec3(1,0,0)), f - vec3(1,0,0)), u.x),
                   mix(dot(hashGrad(i + vec3(0,1,0)), f - vec3(0,1,0)),
                       dot(hashGrad(i + vec3(1,1,0)), f - vec3(1,1,0)), u.x), u.y),
               mix(mix(dot(hashGrad(i + vec3(0,0,1)), f - vec3(0,0,1)),
                       dot(hashGrad(i + vec3(1,0,1)), f - vec3(1,0,1)), u.x),
                   mix(dot(hashGrad(i + vec3(0,1,1)), f - vec3(0,1,1)),
                       dot(hashGrad(i + vec3(1,1,1)), f - vec3(1,1,1)), u.x), u.y), u.z);
}

float fbm(vec3 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * gradientNoise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value * 0.5 + 0.5;
}

void main() {
    vec3 dir = normalize(vDirection);

    // Deep space background color
    vec3 color = vec3(0.002, 0.003, 0.012);

    // --- Milky Way band ---
    // A bright band across the sky tilted at an angle
    vec3 milkyAxis = normalize(vec3(0.3, 0.1, 1.0));
    float milkyDist = abs(dot(dir, milkyAxis));
    float milkyBand = smoothstep(0.35, 0.0, milkyDist);

    // Nebula-like structure within the Milky Way
    float nebula1 = fbm(dir * 4.0 + vec3(42.0), 6);
    float nebula2 = fbm(dir * 8.0 + vec3(17.0), 5);
    float nebulaDetail = fbm(dir * 16.0 + vec3(73.0), 4);

    // Milky Way color: warm white with dust lanes
    vec3 milkyColor = vec3(0.08, 0.07, 0.09) * (nebula1 * 0.6 + 0.4);
    // Dark dust lanes
    float dustLane = smoothstep(0.35, 0.55, nebula2);
    milkyColor *= mix(1.0, 0.2, dustLane * milkyBand);
    // Add warm tint
    milkyColor += vec3(0.02, 0.015, 0.01) * nebulaDetail * milkyBand;

    color += milkyColor * milkyBand;

    // --- Distant nebula patches ---
    // A few colored nebula regions scattered in the sky
    float neb1 = fbm(dir * 2.0 + vec3(100.0), 6);
    float neb1Mask = smoothstep(0.6, 0.8, neb1);
    vec3 neb1Color = vec3(0.15, 0.03, 0.05) * neb1Mask; // Reddish emission nebula

    float neb2 = fbm(dir * 2.5 + vec3(200.0), 6);
    float neb2Mask = smoothstep(0.65, 0.85, neb2);
    vec3 neb2Color = vec3(0.02, 0.04, 0.12) * neb2Mask; // Blue reflection nebula

    float neb3 = fbm(dir * 3.0 + vec3(300.0), 5);
    float neb3Mask = smoothstep(0.7, 0.9, neb3);
    vec3 neb3Color = vec3(0.06, 0.02, 0.08) * neb3Mask; // Purple nebula

    color += neb1Color + neb2Color + neb3Color;

    // --- Starfield ---
    // Multiple layers of stars at different densities

    // Layer 1: Dense faint stars
    vec3 cell1 = floor(dir * 120.0);
    float star1Chance = hash(cell1);
    if (star1Chance > 0.96) {
        vec3 cellFract = fract(dir * 120.0) - 0.5;
        float dist = length(cellFract);
        float brightness = hash(cell1 + 1.0) * 0.4 + 0.1;
        float star = smoothstep(0.12, 0.0, dist);
        float colorTemp = hash(cell1 + 2.0);
        vec3 starColor = vec3(0.8, 0.85, 1.0); // Default bluish
        if (colorTemp < 0.15) starColor = vec3(1.0, 0.7, 0.4); // Orange
        else if (colorTemp < 0.3) starColor = vec3(1.0, 0.9, 0.7); // Yellow
        else if (colorTemp > 0.85) starColor = vec3(0.6, 0.7, 1.0); // Blue
        color += starColor * brightness * star;
    }

    // Layer 2: Medium stars
    vec3 cell2 = floor(dir * 60.0);
    float star2Chance = hash(cell2 + 50.0);
    if (star2Chance > 0.975) {
        vec3 cellFract = fract(dir * 60.0) - 0.5;
        float dist = length(cellFract);
        float brightness = hash(cell2 + 51.0) * 0.6 + 0.3;
        float star = smoothstep(0.1, 0.0, dist);
        // Airy disk diffraction ring
        float ring = smoothstep(0.14, 0.12, dist) * smoothstep(0.1, 0.12, dist);
        float colorTemp = hash(cell2 + 52.0);
        vec3 starColor = vec3(1.0);
        if (colorTemp < 0.2) starColor = vec3(1.0, 0.65, 0.4);
        else if (colorTemp > 0.8) starColor = vec3(0.65, 0.75, 1.0);
        color += starColor * brightness * (star + ring * 0.15);
    }

    // Layer 3: Bright prominent stars (rare)
    vec3 cell3 = floor(dir * 25.0);
    float star3Chance = hash(cell3 + 100.0);
    if (star3Chance > 0.992) {
        vec3 cellFract = fract(dir * 25.0) - 0.5;
        float dist = length(cellFract);
        float brightness = hash(cell3 + 101.0) * 0.5 + 0.5;
        float star = smoothstep(0.08, 0.0, dist);
        // Diffraction spikes
        float spike1 = smoothstep(0.02, 0.0, abs(cellFract.x)) * smoothstep(0.3, 0.0, abs(cellFract.y));
        float spike2 = smoothstep(0.02, 0.0, abs(cellFract.y)) * smoothstep(0.3, 0.0, abs(cellFract.x));
        float spikes = (spike1 + spike2) * 0.2;
        // Glow halo
        float glow = smoothstep(0.25, 0.0, dist) * 0.15;
        float colorTemp = hash(cell3 + 102.0);
        vec3 starColor = vec3(1.0);
        if (colorTemp < 0.25) starColor = vec3(1.0, 0.6, 0.3);
        else if (colorTemp > 0.75) starColor = vec3(0.5, 0.7, 1.0);
        color += starColor * brightness * (star + spikes + glow);
    }

    // --- Unresolved star clusters (faint glow patches) ---
    float cluster1 = fbm(dir * 6.0 + vec3(500.0), 4);
    float clusterMask = smoothstep(0.65, 0.85, cluster1);
    color += vec3(0.015, 0.012, 0.02) * clusterMask;

    FragColor = vec4(color, 1.0);
}
