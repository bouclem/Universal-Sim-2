#version 410 core

in vec3 vWorldPos;
in vec3 vLocalPos;

uniform vec3 uGalaxyCenter;
uniform float uGalaxyRadius;

out vec4 FragColor;

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
    for (int i = 0; i < octaves; i++) {
        value += amplitude * gradientNoise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value * 0.5 + 0.5;
}

void main() {
    vec3 relPos = vWorldPos - uGalaxyCenter;
    float r = length(relPos.xz) / uGalaxyRadius;
    float y = abs(relPos.y) / (uGalaxyRadius * 0.05);

    // Disk falloff: thin disk shape
    float diskFalloff = exp(-y * y * 8.0);
    // Radial falloff
    float radialFalloff = smoothstep(1.0, 0.2, r);

    // Spiral arm structure
    float angle = atan(relPos.z, relPos.x);
    float spiralPhase = angle * 2.0 + r * 12.0;
    float arms = sin(spiralPhase) * 0.5 + 0.5;
    arms = pow(arms, 2.0);

    // Dust/nebula noise
    vec3 noiseCoord = relPos * 0.002;
    float dust = fbm(noiseCoord, 5);
    float dustDetail = fbm(noiseCoord * 3.0 + 10.0, 4);

    // Color: warm core, blue arms, dark dust lanes
    vec3 coreColor = vec3(1.0, 0.85, 0.6);
    vec3 armColor = vec3(0.4, 0.5, 0.8);
    vec3 dustColor = vec3(0.15, 0.08, 0.05);

    float coreFactor = smoothstep(0.4, 0.0, r);
    vec3 color = mix(armColor, coreColor, coreFactor);

    // Dust lanes darken parts of the arms
    float dustLane = smoothstep(0.4, 0.6, dustDetail) * arms;
    color = mix(color, dustColor, dustLane * 0.5);

    // Emission nebula patches (reddish)
    float emission = smoothstep(0.65, 0.8, dust) * arms;
    color = mix(color, vec3(0.8, 0.2, 0.3), emission * 0.3);

    // Overall density
    float density = diskFalloff * radialFalloff * (0.3 + arms * 0.7) * (0.5 + dust * 0.5);

    float alpha = density * 0.4;
    alpha = clamp(alpha, 0.0, 0.6);

    FragColor = vec4(color * density, alpha);
}
