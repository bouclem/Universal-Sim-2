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

void main() {
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(uCameraPos - vWorldPos);

    float slowTime = uTime * 0.025;
    float fastTime = uTime * 0.06;

    // Granulation: convection cells (animated Voronoi)
    vec3 granCoord = normal * 10.0;
    vec3 gran = voronoi(granCoord);
    float granulation = gran.x;
    float granEdge = smoothstep(0.0, 0.15, gran.z); // Cell boundaries

    // Multi-scale turbulence
    float turb1 = fbmNorm(normal * 3.0 + slowTime, 6);
    float turb2 = fbmNorm(normal * 6.0 + slowTime * 1.3, 5);
    float turbulence = turb1 * 0.6 + turb2 * 0.4;

    // Sunspots: dark cooler regions with penumbra
    float spotNoise = fbmNorm(normal * 2.5 + vec3(slowTime * 0.4), 5);
    float umbra = smoothstep(0.72, 0.78, spotNoise);
    float penumbra = smoothstep(0.65, 0.72, spotNoise) * (1.0 - umbra);

    // Solar flares / bright plage regions
    float flareNoise = fbmNorm(normal * 5.0 + vec3(fastTime), 6);
    float flares = smoothstep(0.72, 0.92, flareNoise);

    // Faculae: bright regions near sunspots
    float faculae = smoothstep(0.60, 0.65, spotNoise) * (1.0 - smoothstep(0.65, 0.72, spotNoise));

    // Base color with granulation texture
    vec3 hotColor = uStarColor * 1.1;
    vec3 coolColor = uStarColor * 0.85;
    vec3 color = mix(coolColor, hotColor, granEdge);

    // Granulation cell brightness variation
    color *= (0.88 + granulation * 0.2);

    // Turbulence variation
    color *= (0.92 + turbulence * 0.16);

    // Sunspot umbra: very dark, reddish
    vec3 umbraColor = uStarColor * 0.25 * vec3(1.0, 0.5, 0.2);
    color = mix(color, umbraColor, umbra * 0.85);

    // Sunspot penumbra: intermediate, radial structure
    vec3 penumbraColor = uStarColor * 0.55 * vec3(1.0, 0.7, 0.4);
    color = mix(color, penumbraColor, penumbra * 0.6);

    // Faculae: bright regions
    color = mix(color, uStarColor * 1.3, faculae * 0.3);

    // Bright flare regions
    vec3 flareColor = uStarColor * 1.6 + vec3(0.15, 0.08, 0.0);
    color = mix(color, flareColor, flares * 0.35);

    // Limb darkening: wavelength-dependent (more darkening in blue)
    float cosTheta = max(dot(normal, viewDir), 0.0);
    float limbR = 0.3 + 0.7 * pow(cosTheta, 0.5);
    float limbG = 0.25 + 0.75 * pow(cosTheta, 0.7);
    float limbB = 0.2 + 0.8 * pow(cosTheta, 1.0);
    color *= vec3(limbR, limbG, limbB);

    // Corona glow at the very edge
    float edgeFactor = 1.0 - cosTheta;
    float corona = pow(edgeFactor, 8.0);
    vec3 coronaColor = uStarColor * 0.5 + vec3(0.1, 0.05, 0.0);
    color += coronaColor * corona * 0.4;

    // Emissive boost
    color *= 1.2;

    // Subtle HDR bloom simulation
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    color += color * max(luminance - 0.8, 0.0) * 0.3;

    FragColor = vec4(color, 1.0);
}
