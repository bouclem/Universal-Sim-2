#version 410 core

in vec3 vNormal;
in vec3 vWorldPos;
in vec3 vLocalPos;

uniform vec3 uStarPos;
uniform vec3 uStarColor;
uniform vec3 uCameraPos;
uniform vec3 uColorPrimary;
uniform vec3 uColorSecondary;
uniform vec3 uColorAccent;
uniform float uNoiseScale;
uniform float uNoiseSeed;
uniform int uPlanetType; // 0=rocky, 1=gas, 2=ice

out vec4 FragColor;

// --- Improved noise functions ---

float hash(vec3 p) {
    p = fract(p * vec3(443.897, 441.423, 437.195));
    p += dot(p, p.yzx + 19.19);
    return fract((p.x + p.y) * p.z);
}

float hash2(vec2 p) {
    p = fract(p * vec2(443.897, 441.423));
    p += dot(p, p.yx + 19.19);
    return fract(p.x * p.y);
}

float noise3D(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    // Quintic interpolation for smoother results
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
    float frequency = 1.0;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise3D(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// Ridged noise: abs(noise) inverted, gives sharp ridges like mountain ranges
float ridgedNoise(vec3 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    float prev = 1.0;
    for (int i = 0; i < octaves; i++) {
        float n = 1.0 - abs(noise3D(p * frequency) * 2.0 - 1.0);
        n = n * n;
        value += n * amplitude * prev;
        prev = n;
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// Domain warping: distort coordinates using noise for organic shapes
vec3 domainWarp(vec3 p, float strength) {
    float wx = fbm(p + vec3(0.0, 0.0, 0.0), 4);
    float wy = fbm(p + vec3(5.2, 1.3, 2.8), 4);
    float wz = fbm(p + vec3(1.7, 9.2, 3.4), 4);
    return p + vec3(wx, wy, wz) * strength;
}

// Voronoi noise for crater-like features
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
                float dist = dot(diff, diff);
                minDist = min(minDist, dist);
            }
        }
    }
    return sqrt(minDist);
}

// --- Planet surface functions ---

vec3 rockyColor(vec3 pos) {
    vec3 p = pos * uNoiseScale + uNoiseSeed;

    // Domain-warped base terrain
    vec3 warped = domainWarp(p, 0.4);
    float terrain = fbm(warped, 8);

    // Ridged mountains
    float mountains = ridgedNoise(p * 1.5 + 3.0, 6);

    // Voronoi craters
    float craters = voronoi(p * 4.0);
    float craterDepth = smoothstep(0.05, 0.2, craters);

    // Elevation combines terrain + mountains
    float elevation = terrain * 0.6 + mountains * 0.4;

    // Color mapping based on elevation
    vec3 lowColor = uColorPrimary * 0.7;       // Lowlands / valleys
    vec3 midColor = uColorPrimary;              // Mid terrain
    vec3 highColor = uColorSecondary;           // Highlands
    vec3 peakColor = uColorSecondary * 1.3;     // Mountain peaks

    vec3 color;
    if (elevation < 0.3) {
        color = mix(lowColor, midColor, elevation / 0.3);
    } else if (elevation < 0.6) {
        color = mix(midColor, highColor, (elevation - 0.3) / 0.3);
    } else {
        color = mix(highColor, peakColor, (elevation - 0.6) / 0.4);
    }

    // Crater darkening
    color *= mix(0.6, 1.0, craterDepth);

    // Fine detail noise
    float detail = noise3D(p * 20.0) * 0.08;
    color += detail;

    // Polar ice caps with noisy edge
    float latitude = abs(pos.y);
    float iceNoise = fbm(p * 3.0 + 10.0, 4) * 0.15;
    float iceThreshold = 0.7 + iceNoise;
    if (latitude > iceThreshold) {
        float iceFactor = smoothstep(iceThreshold, iceThreshold + 0.12, latitude);
        vec3 iceColor = uColorAccent * (0.9 + noise3D(p * 8.0) * 0.2);
        color = mix(color, iceColor, iceFactor);
    }

    return color;
}

vec3 gasGiantColor(vec3 pos) {
    vec3 p = pos * uNoiseScale + uNoiseSeed;

    // Latitude-based banding with turbulent distortion
    float lat = pos.y;
    float bandDistortion = fbm(p * 0.8, 4) * 0.8;
    float bands = sin(lat * 18.0 + bandDistortion) * 0.5 + 0.5;

    // Secondary finer bands
    float fineBands = sin(lat * 45.0 + fbm(p * 1.5, 3) * 1.5) * 0.5 + 0.5;

    // Combine band layers
    float bandMix = bands * 0.7 + fineBands * 0.3;
    vec3 color = mix(uColorPrimary, uColorSecondary, bandMix);

    // Domain-warped storm systems
    vec3 stormCoord = domainWarp(p * 2.0, 0.6);
    float storm = fbm(stormCoord, 6);

    // Great spot: large persistent storm feature
    vec3 spotCenter = vec3(0.3 + uNoiseSeed * 0.001, -0.2, 0.5);
    float spotDist = length(pos - spotCenter);
    float spot = smoothstep(0.25, 0.15, spotDist);
    float spotSwirl = fbm(p * 5.0 + vec3(storm * 2.0), 4);

    if (spot > 0.0) {
        vec3 spotColor = mix(uColorAccent, uColorPrimary * 1.2, spotSwirl);
        color = mix(color, spotColor, spot * 0.7);
    }

    // General storm turbulence
    if (storm > 0.6) {
        float stormFactor = smoothstep(0.6, 0.85, storm);
        color = mix(color, uColorAccent, stormFactor * 0.4);
    }

    // Subtle longitudinal streaks
    float streaks = noise3D(vec3(lat * 30.0, p.y * 0.5, p.z * 0.5)) * 0.06;
    color += streaks;

    return color;
}

vec3 iceGiantColor(vec3 pos) {
    vec3 p = pos * uNoiseScale + uNoiseSeed;

    // Smooth wide bands
    float lat = pos.y;
    float bandWarp = fbm(p * 0.5, 3) * 0.6;
    float bands = sin(lat * 10.0 + bandWarp) * 0.5 + 0.5;

    vec3 color = mix(uColorPrimary, uColorSecondary, bands * 0.5 + 0.25);

    // Wispy high-altitude cloud features with domain warping
    vec3 cloudCoord = domainWarp(p * 2.5, 0.5);
    float clouds = fbm(cloudCoord, 6);
    float wisps = smoothstep(0.4, 0.7, clouds);
    color = mix(color, uColorAccent, wisps * 0.25);

    // Subtle polar brightening
    float polarBright = smoothstep(0.6, 0.95, abs(lat));
    color = mix(color, uColorAccent * 0.8 + vec3(0.1), polarBright * 0.3);

    // Very fine detail
    float detail = noise3D(p * 15.0) * 0.04;
    color += detail;

    return color;
}

void main() {
    vec3 normal = normalize(vNormal);

    // Surface color from procedural noise
    vec3 surfaceColor;
    if (uPlanetType == 0) {
        surfaceColor = rockyColor(vLocalPos);
    } else if (uPlanetType == 1) {
        surfaceColor = gasGiantColor(vLocalPos);
    } else {
        surfaceColor = iceGiantColor(vLocalPos);
    }

    // Lighting from star
    vec3 lightDir = normalize(uStarPos - vWorldPos);
    float NdotL = dot(normal, lightDir);
    float diff = max(NdotL, 0.0);

    // Soft terminator: smooth transition at the day/night boundary
    float terminator = smoothstep(-0.1, 0.15, NdotL);

    // Ambient: slightly tinted by star color
    vec3 ambient = surfaceColor * 0.06 * (vec3(0.5) + uStarColor * 0.5);

    // Diffuse with soft terminator
    vec3 diffuse = surfaceColor * terminator * uStarColor;

    // Specular: Blinn-Phong
    vec3 viewDir = normalize(uCameraPos - vWorldPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 64.0);
    // Rocky planets: low specular. Gas/ice: slightly more
    float specStrength = (uPlanetType == 0) ? 0.08 : 0.15;
    vec3 specular = uStarColor * spec * specStrength * terminator;

    // Rim light: subtle backlight at edges
    float rim = 1.0 - max(dot(normal, viewDir), 0.0);
    rim = pow(rim, 4.0);
    vec3 rimLight = uStarColor * rim * 0.04 * max(-NdotL, 0.0);

    vec3 finalColor = ambient + diffuse + specular + rimLight;

    FragColor = vec4(finalColor, 1.0);
}
