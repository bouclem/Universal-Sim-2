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

// ============================================================
// High-quality noise functions
// ============================================================

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

// Quintic-interpolated 3D value noise
float noise3D(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

    return mix(
        mix(mix(hash(i), hash(i + vec3(1,0,0)), f.x),
            mix(hash(i + vec3(0,1,0)), hash(i + vec3(1,1,0)), f.x), f.y),
        mix(mix(hash(i + vec3(0,0,1)), hash(i + vec3(1,0,1)), f.x),
            mix(hash(i + vec3(0,1,1)), hash(i + vec3(1,1,1)), f.x), f.y),
        f.z);
}

// Gradient noise for smoother results (Perlin-style)
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

// FBM with gradient noise
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

// FBM returning value in [0,1] range
float fbmNorm(vec3 p, int octaves) {
    return fbm(p, octaves) * 0.5 + 0.5;
}

// Ridged multifractal noise — sharp mountain ridges
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

// Swiss turbulence — ridged noise with lateral displacement
float swissNoise(vec3 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    vec3 dx = vec3(0.0);
    for (int i = 0; i < octaves; i++) {
        float n = gradientNoise(p * frequency + dx);
        float ridge = 1.0 - abs(n);
        ridge = ridge * ridge;
        dx += vec3(n) * amplitude * 0.5;
        value += ridge * amplitude;
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// Multi-level domain warping for organic shapes
vec3 domainWarp(vec3 p, float strength) {
    float wx = fbm(p + vec3(0.0, 0.0, 0.0), 5);
    float wy = fbm(p + vec3(5.2, 1.3, 2.8), 5);
    float wz = fbm(p + vec3(1.7, 9.2, 3.4), 5);
    return p + vec3(wx, wy, wz) * strength;
}

// Double domain warp for extra organic feel
vec3 domainWarp2(vec3 p, float s1, float s2) {
    vec3 q = domainWarp(p, s1);
    return domainWarp(q + vec3(3.7, 8.2, 1.1), s2);
}

// Voronoi noise — returns (F1 distance, F2 distance, cell ID)
vec3 voronoi(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    float f1 = 1.0;
    float f2 = 1.0;
    float id = 0.0;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            for (int z = -1; z <= 1; z++) {
                vec3 neighbor = vec3(float(x), float(y), float(z));
                vec3 cellId = i + neighbor;
                vec3 point = vec3(hash(cellId),
                                  hash(cellId + 31.0),
                                  hash(cellId + 57.0));
                vec3 diff = neighbor + point - f;
                float dist = dot(diff, diff);
                if (dist < f1) {
                    f2 = f1;
                    f1 = dist;
                    id = hash(cellId + 113.0);
                } else if (dist < f2) {
                    f2 = dist;
                }
            }
        }
    }
    return vec3(sqrt(f1), sqrt(f2), id);
}

// Analytical noise derivatives for bump mapping
vec3 noiseDerivatives(vec3 p) {
    float eps = 0.001;
    float nx = gradientNoise(p + vec3(eps, 0, 0)) - gradientNoise(p - vec3(eps, 0, 0));
    float ny = gradientNoise(p + vec3(0, eps, 0)) - gradientNoise(p - vec3(0, eps, 0));
    float nz = gradientNoise(p + vec3(0, 0, eps)) - gradientNoise(p - vec3(0, 0, eps));
    return vec3(nx, ny, nz) / (2.0 * eps);
}

// ============================================================
// Rocky planet — continents, oceans, biomes, craters
// ============================================================

vec3 rockyColor(vec3 pos, out float heightOut, out bool isOcean) {
    vec3 p = pos * uNoiseScale + uNoiseSeed;

    // Continental shelf: large-scale domain-warped terrain
    vec3 warped = domainWarp2(p * 0.8, 0.5, 0.3);
    float continental = fbmNorm(warped, 8);

    // Tectonic ridges along continental boundaries
    float tectonics = swissNoise(p * 1.2 + 7.0, 6);

    // Mountain ranges with ridged noise
    float mountains = ridgedNoise(p * 2.0 + 3.0, 7);

    // Fine terrain detail
    float detail = fbmNorm(p * 8.0, 5) * 0.15;

    // Combine into elevation map
    float elevation = continental * 0.5 + mountains * 0.25 + tectonics * 0.15 + detail;

    // Sea level — creates oceans
    float seaLevel = 0.42;
    isOcean = (elevation < seaLevel);
    heightOut = elevation;

    if (isOcean) {
        // Ocean coloring: depth-based
        float depth = (seaLevel - elevation) / seaLevel;
        vec3 shallowWater = uColorAccent * vec3(0.4, 0.7, 1.0);
        vec3 deepWater = uColorAccent * vec3(0.05, 0.15, 0.4);
        vec3 color = mix(shallowWater, deepWater, smoothstep(0.0, 0.5, depth));

        // Subtle caustic-like patterns in shallow water
        float caustic = voronoi(p * 12.0).x;
        color += vec3(0.02) * smoothstep(0.3, 0.0, depth) * caustic;

        return color;
    }

    // Land biomes based on latitude and elevation
    float latitude = abs(pos.y);
    float moisture = fbmNorm(p * 2.0 + vec3(100.0), 5);

    // Biome colors
    vec3 desert = uColorPrimary * vec3(1.1, 0.9, 0.6);
    vec3 grassland = uColorPrimary * vec3(0.35, 0.55, 0.2);
    vec3 forest = uColorPrimary * vec3(0.15, 0.35, 0.1);
    vec3 tundra = uColorSecondary * vec3(0.6, 0.65, 0.55);
    vec3 rock = uColorSecondary * vec3(0.5, 0.45, 0.4);
    vec3 snow = vec3(0.9, 0.92, 0.95);

    // Biome selection
    vec3 color;
    float landElev = (elevation - seaLevel) / (1.0 - seaLevel); // 0-1 above sea

    // Coastal sand
    float coastalBand = smoothstep(0.0, 0.04, landElev);
    vec3 sand = uColorPrimary * vec3(0.9, 0.85, 0.6);

    if (landElev < 0.04) {
        color = mix(sand, grassland, coastalBand);
    } else if (latitude > 0.75) {
        // Polar: tundra to snow
        float snowLine = 0.75 + fbm(p * 3.0, 4) * 0.1;
        float snowFactor = smoothstep(snowLine, snowLine + 0.15, latitude);
        color = mix(tundra, snow, snowFactor);
    } else if (latitude > 0.55) {
        // Temperate: forest/grassland based on moisture
        color = mix(grassland, forest, smoothstep(0.3, 0.6, moisture));
        // Transition to tundra
        float tundraFactor = smoothstep(0.55, 0.75, latitude);
        color = mix(color, tundra, tundraFactor);
    } else if (latitude < 0.25) {
        // Tropical: desert or jungle based on moisture
        color = mix(desert, forest * vec3(0.8, 1.2, 0.8), smoothstep(0.3, 0.6, moisture));
    } else {
        // Mid-latitudes: grassland/forest
        color = mix(grassland, forest, smoothstep(0.35, 0.65, moisture));
        // Blend desert in dry areas
        float desertFactor = smoothstep(0.35, 0.15, moisture) * smoothstep(0.25, 0.4, latitude);
        color = mix(color, desert, desertFactor * 0.5);
    }

    // Mountain rock and snow at high elevation
    float rockLine = 0.5 + fbm(p * 4.0, 3) * 0.1;
    float snowMountain = 0.7 + fbm(p * 3.0, 3) * 0.1;
    if (landElev > rockLine) {
        float rockFactor = smoothstep(rockLine, rockLine + 0.15, landElev);
        color = mix(color, rock, rockFactor);
    }
    if (landElev > snowMountain) {
        float snowFactor = smoothstep(snowMountain, snowMountain + 0.1, landElev);
        color = mix(color, snow, snowFactor * 0.8);
    }

    // Voronoi craters (more on airless bodies, less with atmosphere)
    vec3 craterInfo = voronoi(p * 5.0);
    float craterF1 = craterInfo.x;
    float craterRim = smoothstep(0.08, 0.12, craterF1) - smoothstep(0.12, 0.18, craterF1);
    float craterFloor = 1.0 - smoothstep(0.0, 0.08, craterF1);
    color = mix(color, rock * 0.7, craterFloor * 0.4);
    color = mix(color, rock * 1.2, craterRim * 0.3);

    // Fine surface detail
    float microDetail = gradientNoise(p * 30.0) * 0.03;
    color += microDetail;

    return color;
}

// ============================================================
// Gas giant — turbulent bands, storms, chevrons
// ============================================================

vec3 gasGiantColor(vec3 pos) {
    vec3 p = pos * uNoiseScale + uNoiseSeed;
    float lat = pos.y;

    // Primary banding: latitude-based with turbulent distortion
    float bandWarp1 = fbm(p * 0.6, 5) * 1.2;
    float bandWarp2 = fbm(p * 1.2 + 5.0, 4) * 0.6;
    float bands = sin(lat * 20.0 + bandWarp1 + bandWarp2) * 0.5 + 0.5;

    // Secondary finer bands
    float fineBandWarp = fbm(p * 2.0 + 10.0, 4) * 0.8;
    float fineBands = sin(lat * 50.0 + fineBandWarp) * 0.5 + 0.5;

    // Tertiary micro-bands
    float microBands = sin(lat * 120.0 + fbm(p * 3.0, 3) * 1.5) * 0.5 + 0.5;

    // Combine band layers
    float bandMix = bands * 0.55 + fineBands * 0.3 + microBands * 0.15;

    // Band color: zones (light) and belts (dark)
    vec3 zoneColor = uColorPrimary * 1.15;
    vec3 beltColor = uColorSecondary * 0.85;
    vec3 color = mix(beltColor, zoneColor, bandMix);

    // Chevron patterns between bands (V-shaped turbulence)
    float chevronLat = sin(lat * 20.0) * 0.5 + 0.5;
    float chevronEdge = smoothstep(0.45, 0.55, chevronLat);
    float chevron = fbm(vec3(p.x * 3.0 + lat * 8.0, p.y * 0.5, p.z * 3.0 + lat * 8.0), 4);
    color = mix(color, mix(zoneColor, beltColor, 0.5), chevron * chevronEdge * 0.15);

    // Large storm systems with domain warping
    vec3 stormCoord = domainWarp2(p * 1.5, 0.8, 0.4);
    float storm = fbmNorm(stormCoord, 7);

    // Great Red Spot analog
    vec3 spotCenter = normalize(vec3(0.3 + uNoiseSeed * 0.001, -0.2, 0.5));
    float spotDist = length(pos - spotCenter);
    float spot = smoothstep(0.3, 0.12, spotDist);

    if (spot > 0.0) {
        // Swirling interior
        float angle = atan(pos.z - spotCenter.z, pos.x - spotCenter.x);
        float swirl = fbm(vec3(spotDist * 8.0 + angle * 2.0, p.y, uNoiseSeed), 5);
        vec3 spotColor = mix(uColorAccent, uColorPrimary * 1.3, swirl * 0.5 + 0.5);
        // Darker eye
        float eye = smoothstep(0.06, 0.02, spotDist);
        spotColor = mix(spotColor, uColorAccent * 0.7, eye);
        color = mix(color, spotColor, spot * 0.8);
    }

    // Smaller storm vortices
    for (int s = 0; s < 3; s++) {
        vec3 sc = normalize(vec3(
            hash(vec3(uNoiseSeed + float(s) * 7.0)),
            (hash(vec3(uNoiseSeed + float(s) * 13.0)) - 0.5) * 1.5,
            hash(vec3(uNoiseSeed + float(s) * 19.0))
        ));
        float sd = length(pos - sc);
        float ss = smoothstep(0.15, 0.05, sd);
        if (ss > 0.0) {
            float swirlSmall = fbm(vec3(sd * 12.0, p.y + float(s), p.z), 4);
            vec3 stormCol = mix(zoneColor, uColorAccent, swirlSmall * 0.5 + 0.5);
            color = mix(color, stormCol, ss * 0.5);
        }
    }

    // General turbulence in belt regions
    float turbulence = fbm(domainWarp(p * 3.0, 0.3), 5);
    float beltRegion = smoothstep(0.3, 0.5, 1.0 - bandMix);
    color = mix(color, uColorAccent * 0.9, turbulence * beltRegion * 0.15);

    // Longitudinal streaks
    float streaks = gradientNoise(vec3(lat * 40.0, p.y * 0.3, p.z * 0.3)) * 0.04;
    color += streaks;

    return color;
}

// ============================================================
// Ice giant — methane blue, subtle bands, high-altitude haze
// ============================================================

vec3 iceGiantColor(vec3 pos) {
    vec3 p = pos * uNoiseScale + uNoiseSeed;
    float lat = pos.y;

    // Smooth wide bands with gentle warping
    float bandWarp = fbm(p * 0.4, 4) * 0.8;
    float bands = sin(lat * 12.0 + bandWarp) * 0.5 + 0.5;

    // Secondary subtle bands
    float fineBands = sin(lat * 30.0 + fbm(p * 1.0, 3) * 1.0) * 0.5 + 0.5;

    float bandMix = bands * 0.65 + fineBands * 0.35;
    vec3 color = mix(uColorPrimary, uColorSecondary, bandMix * 0.4 + 0.3);

    // High-altitude methane haze layers
    vec3 hazeCoord = domainWarp2(p * 2.0, 0.6, 0.3);
    float haze = fbmNorm(hazeCoord, 7);
    float hazeLayer = smoothstep(0.35, 0.65, haze);
    vec3 hazeColor = uColorAccent * vec3(0.9, 0.95, 1.0);
    color = mix(color, hazeColor, hazeLayer * 0.2);

    // Wispy cloud streaks
    float cloudWarp = fbm(p * 1.5 + 20.0, 5) * 1.5;
    float clouds = sin(lat * 25.0 + cloudWarp + p.x * 3.0) * 0.5 + 0.5;
    float wisps = smoothstep(0.55, 0.75, clouds);
    color = mix(color, hazeColor * 1.1, wisps * 0.15);

    // Subtle dark spot (like Neptune's Great Dark Spot)
    vec3 darkSpotCenter = normalize(vec3(
        0.5 + uNoiseSeed * 0.002, -0.15, 0.4));
    float darkSpotDist = length(pos - darkSpotCenter);
    float darkSpot = smoothstep(0.2, 0.08, darkSpotDist);
    if (darkSpot > 0.0) {
        float swirl = fbm(vec3(darkSpotDist * 10.0, p.y, p.z), 4);
        vec3 spotColor = uColorPrimary * 0.6;
        color = mix(color, spotColor, darkSpot * 0.5);
        // Bright companion cloud
        float companion = smoothstep(0.22, 0.18, darkSpotDist) *
                          smoothstep(0.12, 0.15, darkSpotDist);
        color = mix(color, hazeColor * 1.3, companion * 0.4);
    }

    // Polar brightening with aurora-like glow
    float polarBright = smoothstep(0.6, 0.95, abs(lat));
    vec3 polarColor = uColorAccent * 0.9 + vec3(0.08, 0.12, 0.15);
    color = mix(color, polarColor, polarBright * 0.35);

    // Very fine atmospheric detail
    float microDetail = gradientNoise(p * 20.0) * 0.025;
    color += microDetail;

    return color;
}

// ============================================================
// Bump mapping from noise derivatives
// ============================================================

vec3 computeBumpNormal(vec3 normal, vec3 pos, float bumpStrength) {
    vec3 p = pos * uNoiseScale + uNoiseSeed;
    vec3 deriv = noiseDerivatives(p * 4.0);

    // Project derivative onto tangent plane
    vec3 tangent = normalize(cross(normal, vec3(0.0, 1.0, 0.0)));
    if (length(tangent) < 0.001) {
        tangent = normalize(cross(normal, vec3(1.0, 0.0, 0.0)));
    }
    vec3 bitangent = normalize(cross(normal, tangent));

    vec3 bumpedNormal = normalize(normal +
        (tangent * deriv.x + bitangent * deriv.z) * bumpStrength);
    return bumpedNormal;
}

// ============================================================
// Main
// ============================================================

void main() {
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(uCameraPos - vWorldPos);
    vec3 lightDir = normalize(uStarPos - vWorldPos);

    // Surface color from procedural noise
    vec3 surfaceColor;
    float height = 0.0;
    bool isOcean = false;

    if (uPlanetType == 0) {
        surfaceColor = rockyColor(vLocalPos, height, isOcean);
    } else if (uPlanetType == 1) {
        surfaceColor = gasGiantColor(vLocalPos);
    } else {
        surfaceColor = iceGiantColor(vLocalPos);
    }

    // Bump mapping for rocky planets (land only)
    vec3 shadingNormal = normal;
    if (uPlanetType == 0 && !isOcean) {
        shadingNormal = computeBumpNormal(normal, vLocalPos, 0.3);
    }

    // Lighting
    float NdotL = dot(shadingNormal, lightDir);
    float NdotLRaw = dot(normal, lightDir);

    // Soft terminator with subsurface scattering approximation
    float terminator = smoothstep(-0.08, 0.2, NdotL);
    float subsurface = smoothstep(-0.3, 0.0, NdotLRaw) * 0.08;

    // Ambient: hemisphere lighting (sky + ground bounce)
    vec3 skyAmbient = vec3(0.06, 0.08, 0.12) * uStarColor;
    vec3 groundAmbient = surfaceColor * 0.03;
    float ambientMix = dot(normal, vec3(0, 1, 0)) * 0.5 + 0.5;
    vec3 ambient = mix(groundAmbient, skyAmbient, ambientMix) * surfaceColor;

    // Diffuse with soft terminator
    vec3 diffuse = surfaceColor * terminator * uStarColor;

    // Subsurface scattering tint on terminator
    vec3 sss = surfaceColor * subsurface * uStarColor * vec3(1.0, 0.7, 0.5);

    // Specular: Blinn-Phong with roughness variation
    vec3 halfDir = normalize(lightDir + viewDir);
    float NdotH = max(dot(shadingNormal, halfDir), 0.0);

    float specPower, specStrength;
    if (uPlanetType == 0) {
        if (isOcean) {
            // Water: high specular, sharp highlight
            specPower = 256.0;
            specStrength = 0.6;
        } else {
            // Land: rough, low specular
            specPower = 32.0;
            specStrength = 0.05;
        }
    } else if (uPlanetType == 1) {
        specPower = 16.0;
        specStrength = 0.08;
    } else {
        specPower = 24.0;
        specStrength = 0.1;
    }

    float spec = pow(NdotH, specPower);
    vec3 specular = uStarColor * spec * specStrength * terminator;

    // Fresnel for ocean
    if (uPlanetType == 0 && isOcean) {
        float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 5.0);
        specular *= (0.04 + 0.96 * fresnel);
    }

    // Rim light: subtle backlight at edges
    float rim = 1.0 - max(dot(normal, viewDir), 0.0);
    rim = pow(rim, 4.0);
    vec3 rimLight = uStarColor * rim * 0.06 * max(-NdotLRaw, 0.0);

    // Night side: faint city lights for rocky planets (optional subtle glow)
    vec3 nightGlow = vec3(0.0);
    if (uPlanetType == 0 && !isOcean && NdotLRaw < -0.05) {
        float nightFactor = smoothstep(-0.05, -0.2, NdotLRaw);
        float lights = noise3D(vLocalPos * uNoiseScale * 15.0 + uNoiseSeed);
        lights = smoothstep(0.65, 0.75, lights);
        nightGlow = vec3(1.0, 0.85, 0.5) * lights * nightFactor * 0.015;
    }

    vec3 finalColor = ambient + diffuse + sss + specular + rimLight + nightGlow;

    // Tone mapping (subtle, prevents blowout)
    finalColor = finalColor / (finalColor + vec3(1.0)) * 1.1;

    FragColor = vec4(finalColor, 1.0);
}
