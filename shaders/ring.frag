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

void main() {
    // Procedural ring bands based on radial position
    float bands = sin(vRadialT * 60.0 + uNoiseSeed) * 0.5 + 0.5;
    float detail = sin(vRadialT * 200.0 + uNoiseSeed * 1.7) * 0.5 + 0.5;

    // Gaps in the rings (like Cassini division)
    float gap1 = smoothstep(0.48, 0.50, vRadialT) * (1.0 - smoothstep(0.52, 0.54, vRadialT));
    float gap2 = smoothstep(0.73, 0.75, vRadialT) * (1.0 - smoothstep(0.76, 0.78, vRadialT));
    float gapFactor = 1.0 - gap1 * 0.8 - gap2 * 0.6;

    // Color variation across the ring
    vec3 color = uRingColor * (0.7 + bands * 0.3) * (0.9 + detail * 0.1);

    // Simple lighting from star
    vec3 lightDir = normalize(uStarPos - vWorldPos);
    // Rings are flat, so use a fixed normal (up)
    float diff = abs(lightDir.y) * 0.5 + 0.5;
    color *= diff * uStarColor;

    // Shadow from planet: if the ring fragment is behind the planet relative to star
    vec3 toStar = normalize(uStarPos - vWorldPos);
    vec3 toPlanet = uPlanetPos - vWorldPos;
    float projDist = dot(toPlanet, toStar);
    if (projDist > 0.0) {
        vec3 closestPoint = vWorldPos + toStar * projDist;
        float distFromAxis = length(closestPoint - uPlanetPos);
        if (distFromAxis < uPlanetRadius) {
            color *= 0.3; // In shadow
        }
    }

    // Fade at edges
    float edgeFade = smoothstep(0.0, 0.05, vRadialT) *
                     smoothstep(1.0, 0.95, vRadialT);

    float alpha = uOpacity * gapFactor * edgeFade;

    FragColor = vec4(color, alpha);
}
