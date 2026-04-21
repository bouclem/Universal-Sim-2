#version 410 core

in vec3 vWorldPos;
in float vRadialT;

uniform vec3 uRingColor;
uniform float uOpacity;
uniform float uNoiseSeed;
uniform vec3 uStarPos;
uniform vec3 uStarColor;
uniform vec3 uPlanetPos;
uniform float uPlanetRadius;

out vec4 FragColor;

float hash(float p) {
    p = fract(p * 443.897);
    p += p * (p + 19.19);
    return fract(p);
}

float noise1D(float p) {
    float i = floor(p);
    float f = fract(p);
    f = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
    return mix(hash(i), hash(i + 1.0), f);
}

void main() {
    // Multi-scale procedural ring bands
    float t = vRadialT;

    // Large-scale density variation
    float density1 = sin(t * 40.0 + uNoiseSeed) * 0.5 + 0.5;
    float density2 = sin(t * 80.0 + uNoiseSeed * 1.3) * 0.5 + 0.5;
    float density3 = sin(t * 160.0 + uNoiseSeed * 2.1) * 0.5 + 0.5;
    float density4 = sin(t * 320.0 + uNoiseSeed * 3.7) * 0.5 + 0.5;

    // Combine density layers
    float density = density1 * 0.4 + density2 * 0.3 + density3 * 0.2 + density4 * 0.1;

    // Noise-based density variation (particle clumping)
    float noiseT = t * 500.0 + uNoiseSeed;
    float particleNoise = noise1D(noiseT) * 0.3 + noise1D(noiseT * 3.0) * 0.15;
    density += particleNoise;

    // Named gaps (Cassini-like divisions)
    float gap1 = smoothstep(0.47, 0.49, t) * (1.0 - smoothstep(0.53, 0.55, t));
    float gap2 = smoothstep(0.72, 0.74, t) * (1.0 - smoothstep(0.77, 0.79, t));
    float gap3 = smoothstep(0.30, 0.31, t) * (1.0 - smoothstep(0.32, 0.33, t));
    float gap4 = smoothstep(0.88, 0.89, t) * (1.0 - smoothstep(0.90, 0.91, t));
    float gapFactor = 1.0 - gap1 * 0.9 - gap2 * 0.7 - gap3 * 0.5 - gap4 * 0.4;

    // Color variation across the ring (inner = darker/warmer, outer = lighter/cooler)
    vec3 innerColor = uRingColor * vec3(0.85, 0.75, 0.65);
    vec3 outerColor = uRingColor * vec3(1.0, 1.0, 1.05);
    vec3 color = mix(innerColor, outerColor, t);

    // Density-based brightness variation
    color *= (0.6 + density * 0.5);

    // Subtle color bands (compositional variation)
    float colorBand = sin(t * 100.0 + uNoiseSeed * 5.0) * 0.5 + 0.5;
    vec3 bandTint = mix(vec3(0.95, 0.9, 0.85), vec3(1.0, 0.95, 1.0), colorBand);
    color *= bandTint;

    // Lighting from star
    vec3 lightDir = normalize(uStarPos - vWorldPos);
    // Rings are flat: use both sides
    float diff = abs(lightDir.y) * 0.4 + 0.6;
    color *= diff * uStarColor;

    // Forward scattering: rings brighten when backlit
    vec3 viewDir = normalize(vWorldPos); // Approximate
    float forwardScatter = max(dot(normalize(vWorldPos - uPlanetPos), -lightDir), 0.0);
    forwardScatter = pow(forwardScatter, 4.0) * 0.3;
    color += uStarColor * forwardScatter * (1.0 - density * 0.5);

    // Shadow from planet
    vec3 toStar = normalize(uStarPos - vWorldPos);
    vec3 toPlanet = uPlanetPos - vWorldPos;
    float projDist = dot(toPlanet, toStar);
    if (projDist > 0.0) {
        vec3 closestPoint = vWorldPos + toStar * projDist;
        float distFromAxis = length(closestPoint - uPlanetPos);
        if (distFromAxis < uPlanetRadius) {
            // Soft shadow edge (penumbra)
            float shadowSoft = smoothstep(uPlanetRadius * 0.95, uPlanetRadius, distFromAxis);
            color *= mix(0.15, 1.0, shadowSoft);
        }
    }

    // Fade at edges
    float edgeFade = smoothstep(0.0, 0.03, t) * smoothstep(1.0, 0.97, t);

    // Translucency: thin parts are more transparent
    float alpha = uOpacity * gapFactor * edgeFade * (0.5 + density * 0.5);

    FragColor = vec4(color, alpha);
}
