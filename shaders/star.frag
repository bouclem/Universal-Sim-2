#version 410 core

in vec3 vNormal;
in vec3 vWorldPos;

uniform vec3 uStarColor;
uniform vec3 uCameraPos;
uniform float uTime;

out vec4 FragColor;

float hash(vec3 p) {
    p = fract(p * vec3(443.897, 441.423, 437.195));
    p += dot(p, p.yzx + 19.19);
    return fract((p.x + p.y) * p.z);
}

float noise3D(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

    float n = mix(
        mix(mix(hash(i), hash(i + vec3(1,0,0)), f.x),
            mix(hash(i + vec3(0,1,0)), hash(i + vec3(1,1,0)), f.x), f.y),
        mix(mix(hash(i + vec3(0,0,1)), hash(i + vec3(1,0,1)), f.x),
            mix(hash(i + vec3(0,1,1)), hash(i + vec3(1,1,1)), f.x), f.y),
        f.z);
    return n;
}

float fbm(vec3 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise3D(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// Voronoi for granulation / convection cells
float voronoi(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    float minDist = 1.0;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            for (int z = -1; z <= 1; z++) {
                vec3 neighbor = vec3(float(x), float(y), float(z));
                vec3 point = vec3(hash(i + neighbor),
                                  hash(i + neighbor + 31.0),
                                  hash(i + neighbor + 57.0));
                vec3 diff = neighbor + point - f;
                minDist = min(minDist, dot(diff, diff));
            }
        }
    }
    return sqrt(minDist);
}

void main() {
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(uCameraPos - vWorldPos);

    // Animated coordinates
    float slowTime = uTime * 0.03;
    float fastTime = uTime * 0.08;

    // Granulation: convection cells on the surface
    float granulation = voronoi(normal * 8.0 + slowTime);
    granulation = smoothstep(0.0, 0.5, granulation);

    // Large-scale turbulence
    vec3 turbCoord = normal * 3.0 + slowTime;
    float turbulence = fbm(turbCoord, 6);

    // Sunspots: dark cooler regions
    float spotNoise = fbm(normal * 2.0 + vec3(slowTime * 0.5), 4);
    float spots = smoothstep(0.68, 0.75, spotNoise);

    // Solar flare / bright regions
    float flareNoise = fbm(normal * 4.0 + vec3(fastTime), 5);
    float flares = smoothstep(0.7, 0.9, flareNoise);

    // Base color with granulation texture
    vec3 color = uStarColor * (0.8 + granulation * 0.25);

    // Turbulence variation
    color *= (0.9 + turbulence * 0.2);

    // Sunspots: darken and shift color toward red
    vec3 spotColor = uStarColor * 0.4 * vec3(1.0, 0.6, 0.3);
    color = mix(color, spotColor, spots * 0.7);

    // Bright flare regions
    vec3 flareColor = uStarColor * 1.5 + vec3(0.1, 0.05, 0.0);
    color = mix(color, flareColor, flares * 0.3);

    // Limb darkening (more physically accurate than v0.2)
    float cosTheta = max(dot(normal, viewDir), 0.0);
    // Quadratic limb darkening law
    float limbDarkening = 0.3 + 0.7 * cosTheta;
    color *= limbDarkening;

    // Emissive boost
    color *= 1.15;

    FragColor = vec4(color, 1.0);
}
