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
    return value;
}

float fbmNorm(vec3 p, int octaves) {
    return fbm(p, octaves) * 0.5 + 0.5;
}

// Voronoi for granulation cells
vec3 voronoi(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    float f1 = 1.0;
    float f2 = 1.0;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            for (int z = -1; z <= 1; z++) {
                vec3 neighbor = vec3(float(x), float(y), float(z));
                vec3 cellId = i + neighbor;
                vec3 point = vec3(hash(cellId),
                                  hash(cellId + 31.0),
                                  hash(cellId + 57.0));
                // Animate cell points slowly
                point = 0.5 + 0.5 * sin(point * 6.28 + uTime * 0.02);
                vec3 diff = neighbor + point - f;
                float dist = dot(diff, diff);
                if (dist < f1) {
                    f2 = f1;
                    f1 = dist;
                } else if (dist < f2) {
                    f2 = dist;
                }
            }
        }
    }
    return vec3(sqrt(f1), sqrt(f2), f2 - f1);
}

// Ridged noise for magnetic field lines
float ridgedNoise(vec3 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    float prev = 1.0;
    for (int i = 0; i < octaves; i++) {
        float n = 1.0 - abs(gradientNoise(p * frequency));
        n = n * n;
        value += n * amplitude * prev;
        prev = n;
        frequency *= 2.1;
        amplitude *= 0.5;
    }
    return value;
}

void main() {
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(uCameraPos - vWorldPos);

    float slowTime = uTime * 0.025;
    float fastTime = uTime * 0.06;
    float verySlowTime = uTime * 0.008;

    // === Granulation: multi-scale convection cells ===
    // Large-scale supergranulation
    vec3 superGranCoord = normal * 4.0 + verySlowTime * 0.3;
    vec3 superGran = voronoi(superGranCoord);
    float superGranEdge = smoothstep(0.0, 0.12, superGran.z);

    // Medium granulation (primary convection cells)
    vec3 granCoord = normal * 12.0 + slowTime * 0.2;
    vec3 gran = voronoi(granCoord);
    float granulation = gran.x;
    float granEdge = smoothstep(0.0, 0.15, gran.z);

    // Fine granulation (visible at high LOD)
    vec3 fineGranCoord = normal * 30.0 + slowTime * 0.4;
    vec3 fineGran = voronoi(fineGranCoord);
    float fineGranEdge = smoothstep(0.0, 0.12, fineGran.z);

    // Combined granulation
    float combinedGran = superGranEdge * 0.3 + granEdge * 0.5 + fineGranEdge * 0.2;

    // === Multi-scale turbulence ===
    float turb1 = fbmNorm(normal * 3.0 + slowTime, 7);
    float turb2 = fbmNorm(normal * 7.0 + slowTime * 1.3, 6);
    float turb3 = fbmNorm(normal * 15.0 + slowTime * 1.8, 5);
    float turbulence = turb1 * 0.5 + turb2 * 0.3 + turb3 * 0.2;

    // === Sunspots with more structure ===
    float spotNoise = fbmNorm(normal * 2.5 + vec3(slowTime * 0.3), 6);
    float umbra = smoothstep(0.73, 0.80, spotNoise);
    float penumbra = smoothstep(0.64, 0.73, spotNoise) * (1.0 - umbra);

    // Penumbra radial filaments
    float penumbraDetail = fbmNorm(normal * 20.0 + vec3(slowTime * 0.5), 5);
    float filaments = smoothstep(0.3, 0.7, penumbraDetail);
    penumbra *= (0.7 + filaments * 0.3);

    // === Solar flares / bright plage regions ===
    float flareNoise = fbmNorm(normal * 5.0 + vec3(fastTime), 7);
    float flares = smoothstep(0.72, 0.92, flareNoise);

    // === Faculae: bright regions near sunspots ===
    float faculae = smoothstep(0.58, 0.64, spotNoise) * (1.0 - smoothstep(0.64, 0.73, spotNoise));

    // === Magnetic field-aligned bright loops ===
    float magField = ridgedNoise(normal * 3.0 + vec3(verySlowTime), 5);
    float magLoops = smoothstep(0.5, 0.7, magField) * smoothstep(0.73, 0.65, spotNoise);

    // === Base color with granulation texture ===
    vec3 hotColor = uStarColor * 1.12;
    vec3 coolColor = uStarColor * 0.82;
    vec3 color = mix(coolColor, hotColor, combinedGran);

    // Granulation cell brightness variation
    color *= (0.86 + granulation * 0.22);

    // Supergranulation subtle variation
    color *= (0.96 + superGranEdge * 0.08);

    // Turbulence variation
    color *= (0.90 + turbulence * 0.18);

    // === Sunspot umbra: very dark, reddish ===
    vec3 umbraColor = uStarColor * 0.20 * vec3(1.0, 0.45, 0.15);
    color = mix(color, umbraColor, umbra * 0.88);

    // === Sunspot penumbra: intermediate, radial structure ===
    vec3 penumbraColor = uStarColor * 0.50 * vec3(1.0, 0.65, 0.35);
    color = mix(color, penumbraColor, penumbra * 0.65);

    // === Faculae: bright regions ===
    color = mix(color, uStarColor * 1.35, faculae * 0.35);

    // === Magnetic loops: subtle bright arcs ===
    color = mix(color, uStarColor * 1.25, magLoops * 0.15);

    // === Bright flare regions ===
    vec3 flareColor = uStarColor * 1.7 + vec3(0.18, 0.10, 0.0);
    color = mix(color, flareColor, flares * 0.38);

    // === Limb darkening: wavelength-dependent ===
    float cosTheta = max(dot(normal, viewDir), 0.0);
    float limbR = 0.28 + 0.72 * pow(cosTheta, 0.45);
    float limbG = 0.22 + 0.78 * pow(cosTheta, 0.65);
    float limbB = 0.18 + 0.82 * pow(cosTheta, 0.95);
    color *= vec3(limbR, limbG, limbB);

    // === Solar prominences at the limb ===
    float edgeFactor = 1.0 - cosTheta;
    float limbRegion = smoothstep(0.7, 0.95, edgeFactor);

    // Prominence arcs: ridged noise along the limb
    float promNoise = ridgedNoise(normal * 6.0 + vec3(slowTime * 0.5), 5);
    float prominence = limbRegion * smoothstep(0.4, 0.7, promNoise);
    vec3 promColor = uStarColor * 1.5 * vec3(1.0, 0.6, 0.3);
    color += promColor * prominence * 0.25;

    // === Corona glow at the very edge ===
    float corona = pow(edgeFactor, 6.0);
    // Structured corona with streamers
    float coronaStreamer = fbmNorm(normal * 4.0 + vec3(verySlowTime * 0.3), 5);
    float coronaStructure = 0.6 + coronaStreamer * 0.4;
    vec3 coronaColor = uStarColor * 0.55 + vec3(0.12, 0.06, 0.0);
    color += coronaColor * corona * coronaStructure * 0.5;

    // === Emissive boost ===
    color *= 1.25;

    // === HDR bloom simulation ===
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    color += color * max(luminance - 0.7, 0.0) * 0.35;

    FragColor = vec4(color, 1.0);
}