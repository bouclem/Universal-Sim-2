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
// High-quality noise functions (v0.6.0)
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

float hash2(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * vec3(443.897, 441.423, 437.195));
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
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

// Gradient noise (Perlin-style)
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

// Ridged multifractal — sharp mountain ridges
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

// Erosion noise — simulates hydraulic erosion carving channels
float erosionNoise(vec3 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    vec3 deriv = vec3(0.0);
    for (int i = 0; i < octaves; i++) {
        float n = gradientNoise(p * frequency);
        // Accumulate derivative to simulate sediment transport
        deriv += vec3(n) * amplitude;
        // Erosion: reduce amplitude where slope is steep
        float erosionFactor = 1.0 / (1.0 + dot(deriv, deriv));
        value += n * amplitude * erosionFactor;
        frequency *= 2.2;
        amplitude *= 0.45;
    }
    return value;
}

// Domain warping
vec3 domainWarp(vec3 p, float strength) {
    float wx = fbm(p + vec3(0.0, 0.0, 0.0), 5);
    float wy = fbm(p + vec3(5.2, 1.3, 2.8), 5);
    float wz = fbm(p + vec3(1.7, 9.2, 3.4), 5);
    return p + vec3(wx, wy, wz) * strength;
}

vec3 domainWarp2(vec3 p, float s1, float s2) {
    vec3 q = domainWarp(p, s1);
    return domainWarp(q + vec3(3.7, 8.2, 1.1), s2);
}

// Triple domain warp for extreme organic distortion
vec3 domainWarp3(vec3 p, float s1, float s2, float s3) {
    vec3 q = domainWarp2(p, s1, s2);
    return domainWarp(q + vec3(7.1, 2.9, 5.3), s3);
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

// Crackle noise — Voronoi F2-F1 for tectonic plate boundaries
float crackle(vec3 p) {
    vec3 v = voronoi(p);
    return v.y - v.x;
}

// Noise derivatives for bump mapping
vec3 noiseDerivatives(vec3 p) {
    float eps = 0.001;
    float nx = gradientNoise(p + vec3(eps, 0, 0)) - gradientNoise(p - vec3(eps, 0, 0));
    float ny = gradientNoise(p + vec3(0, eps, 0)) - gradientNoise(p - vec3(0, eps, 0));
    float nz = gradientNoise(p + vec3(0, 0, eps)) - gradientNoise(p - vec3(0, 0, eps));
    return vec3(nx, ny, nz) / (2.0 * eps);
}

// Multi-scale noise derivatives for detailed bump mapping
vec3 multiScaleDerivatives(vec3 p) {
    vec3 d = noiseDerivatives(p * 4.0);
    d += noiseDerivatives(p * 8.0) * 0.5;
    d += noiseDerivatives(p * 16.0) * 0.25;
    return d;
}


// ============================================================
// Rocky planet (v0.6.0) — tectonic plates, erosion, volcanic
// regions, cloud layer, expanded biomes, river-like features
// ============================================================

vec3 rockyColor(vec3 pos, out float heightOut, out bool isOcean) {
    vec3 p = pos * uNoiseScale + uNoiseSeed;

    // Tectonic plate boundaries (Voronoi crackle)
    float plates = crackle(p * 0.6);
    float plateBoundary = smoothstep(0.0, 0.08, plates);

    // Continental shelf: large-scale domain-warped terrain
    vec3 warped = domainWarp3(p * 0.7, 0.5, 0.3, 0.15);
    float continental = fbmNorm(warped, 8);

    // Tectonic ridges along plate boundaries (Swiss turbulence)
    float tectonics = swissNoise(p * 1.2 + 7.0, 7);

    // Mountain ranges with ridged noise
    float mountains = ridgedNoise(p * 2.0 + 3.0, 8);

    // Erosion-carved terrain detail
    float eroded = erosionNoise(p * 4.0 + 11.0, 6) * 0.5 + 0.5;

    // Fine terrain detail
    float detail = fbmNorm(p * 10.0, 5) * 0.12;

    // Micro detail visible at close range
    float microTerrain = fbmNorm(p * 25.0, 4) * 0.05;

    // Combine into elevation map
    float elevation = continental * 0.40
                    + mountains * 0.22
                    + tectonics * 0.15
                    + eroded * 0.10
                    + detail
                    + microTerrain;

    // Rift valleys along plate boundaries
    float riftDepth = (1.0 - plateBoundary) * 0.08;
    elevation -= riftDepth;

    // Volcanic hotspots near plate boundaries
    float volcanoNoise = fbmNorm(p * 3.0 + 50.0, 5);
    float volcanoSpot = (1.0 - plateBoundary) * smoothstep(0.6, 0.8, volcanoNoise);

    // Volcanoes raise terrain
    elevation += volcanoSpot * 0.12;

    // Sea level
    float seaLevel = 0.42;
    isOcean = (elevation < seaLevel);
    heightOut = elevation;

    if (isOcean) {
        float depth = (seaLevel - elevation) / seaLevel;
        vec3 shallowWater = uColorAccent * vec3(0.35, 0.65, 1.0);
        vec3 deepWater = uColorAccent * vec3(0.03, 0.10, 0.35);
        vec3 abyssWater = uColorAccent * vec3(0.01, 0.04, 0.15);
        vec3 color = mix(shallowWater, deepWater, smoothstep(0.0, 0.35, depth));
        color = mix(color, abyssWater, smoothstep(0.35, 0.8, depth));

        // Continental shelf coloring (near coast)
        float shelf = smoothstep(0.0, 0.08, depth);
        vec3 shelfColor = uColorAccent * vec3(0.25, 0.55, 0.7);
        color = mix(shelfColor, color, shelf);

        // Caustic patterns in shallow water
        float caustic = voronoi(p * 15.0).x;
        color += vec3(0.025) * smoothstep(0.2, 0.0, depth) * caustic;

        // Subtle wave patterns
        float waves = sin(p.x * 40.0 + p.z * 30.0 + fbm(p * 5.0, 3) * 3.0) * 0.5 + 0.5;
        color += vec3(0.01) * waves * smoothstep(0.15, 0.0, depth);

        return color;
    }

    // Land biomes based on latitude, elevation, moisture, and temperature
    float latitude = abs(pos.y);
    float moisture = fbmNorm(p * 2.0 + vec3(100.0), 6);
    float temperature = 1.0 - latitude * 0.9 - (elevation - seaLevel) * 0.5;
    temperature += fbm(p * 1.5 + vec3(200.0), 4) * 0.15;

    // Biome palette
    vec3 desert = uColorPrimary * vec3(1.1, 0.9, 0.6);
    vec3 savanna = uColorPrimary * vec3(0.7, 0.65, 0.3);
    vec3 grassland = uColorPrimary * vec3(0.30, 0.50, 0.18);
    vec3 forest = uColorPrimary * vec3(0.12, 0.32, 0.08);
    vec3 rainforest = uColorPrimary * vec3(0.08, 0.28, 0.05);
    vec3 wetland = uColorPrimary * vec3(0.20, 0.35, 0.15);
    vec3 steppe = uColorPrimary * vec3(0.55, 0.50, 0.30);
    vec3 tundra = uColorSecondary * vec3(0.55, 0.60, 0.50);
    vec3 rock = uColorSecondary * vec3(0.45, 0.40, 0.35);
    vec3 snow = vec3(0.92, 0.94, 0.97);
    vec3 sand = uColorPrimary * vec3(0.90, 0.82, 0.58);
    vec3 volcanic = vec3(0.25, 0.15, 0.10);
    vec3 lava = vec3(1.0, 0.3, 0.05);

    float landElev = (elevation - seaLevel) / (1.0 - seaLevel);

    // Coastal zone
    vec3 color;
    float coastalBand = smoothstep(0.0, 0.03, landElev);

    if (landElev < 0.03) {
        // Beach / coastal sand
        color = mix(sand, grassland, coastalBand);
    } else if (temperature < 0.15) {
        // Polar: tundra to snow
        float snowLine = 0.12 + fbm(p * 3.0, 4) * 0.08;
        float snowFactor = smoothstep(snowLine, snowLine + 0.1, 0.15 - temperature);
        color = mix(tundra, snow, snowFactor);
    } else if (temperature < 0.35) {
        // Boreal: tundra / steppe
        float borealMix = smoothstep(0.3, 0.5, moisture);
        color = mix(steppe, tundra, borealMix);
        // Sparse forest patches
        float treePatch = fbmNorm(p * 6.0, 4);
        color = mix(color, forest * 1.2, smoothstep(0.55, 0.7, treePatch) * borealMix * 0.5);
    } else if (temperature > 0.75 && moisture < 0.35) {
        // Hot dry: desert
        color = desert;
        // Sand dune patterns
        float dunes = sin(p.x * 15.0 + fbm(p * 2.0, 3) * 4.0) * 0.5 + 0.5;
        color = mix(color, sand * 1.1, dunes * 0.2);
    } else if (temperature > 0.65 && moisture < 0.5) {
        // Warm dry: savanna
        color = savanna;
        // Scattered tree clumps
        float trees = fbmNorm(p * 8.0, 4);
        color = mix(color, forest * 0.9, smoothstep(0.65, 0.8, trees) * 0.4);
    } else if (temperature > 0.6 && moisture > 0.6) {
        // Tropical wet: rainforest
        color = rainforest;
        // Canopy variation
        float canopy = fbmNorm(p * 12.0, 5);
        color = mix(color, forest * vec3(0.9, 1.1, 0.8), canopy * 0.3);
    } else if (moisture > 0.65) {
        // Wet: wetland / marsh
        color = mix(wetland, forest, smoothstep(0.65, 0.8, moisture));
    } else {
        // Temperate: grassland / forest
        color = mix(grassland, forest, smoothstep(0.35, 0.6, moisture));
        // Blend steppe in drier areas
        float steppeFactor = smoothstep(0.35, 0.2, moisture);
        color = mix(color, steppe, steppeFactor * 0.4);
    }

    // Mountain rock and snow at high elevation
    float rockLine = 0.45 + fbm(p * 4.0, 4) * 0.1;
    float snowMountain = 0.65 + fbm(p * 3.0, 4) * 0.1;
    if (landElev > rockLine) {
        float rockFactor = smoothstep(rockLine, rockLine + 0.12, landElev);
        // Layered rock strata
        float strata = sin(elevation * 80.0 + fbm(p * 6.0, 3) * 2.0) * 0.5 + 0.5;
        vec3 strataColor = mix(rock * 0.85, rock * 1.15, strata);
        color = mix(color, strataColor, rockFactor);
    }
    if (landElev > snowMountain) {
        float snowFactor = smoothstep(snowMountain, snowMountain + 0.08, landElev);
        // Patchy snow with wind exposure
        float snowPatch = fbmNorm(p * 10.0, 4);
        color = mix(color, snow, snowFactor * smoothstep(0.3, 0.6, snowPatch));
    }

    // Volcanic regions near plate boundaries
    if (volcanoSpot > 0.01) {
        color = mix(color, volcanic, volcanoSpot * 0.7);
        // Lava glow in active volcanic areas
        float lavaFlow = fbmNorm(p * 20.0 + 77.0, 4);
        float lavaActive = volcanoSpot * smoothstep(0.6, 0.8, lavaFlow);
        color = mix(color, lava, lavaActive * 0.6);
    }

    // Rift valley coloring
    if (plateBoundary < 0.3) {
        float riftFactor = smoothstep(0.3, 0.0, plateBoundary);
        color = mix(color, rock * 0.6, riftFactor * 0.5);
    }

    // River-like erosion channels in valleys
    float riverNoise = crackle(p * 8.0 + 33.0);
    float riverChannel = smoothstep(0.0, 0.03, riverNoise);
    if (landElev < 0.3 && moisture > 0.4) {
        float riverFactor = (1.0 - riverChannel) * smoothstep(0.0, 0.3, landElev);
        vec3 riverColor = uColorAccent * vec3(0.2, 0.4, 0.6);
        color = mix(color, riverColor, riverFactor * 0.4);
    }

    // Voronoi craters
    vec3 craterInfo = voronoi(p * 5.0);
    float craterF1 = craterInfo.x;
    float craterRim = smoothstep(0.08, 0.12, craterF1) - smoothstep(0.12, 0.18, craterF1);
    float craterFloor = 1.0 - smoothstep(0.0, 0.08, craterF1);
    color = mix(color, rock * 0.65, craterFloor * 0.35);
    color = mix(color, rock * 1.2, craterRim * 0.25);

    // Small impact craters (higher frequency)
    vec3 smallCrater = voronoi(p * 15.0);
    float smallRim = smoothstep(0.06, 0.09, smallCrater.x) - smoothstep(0.09, 0.13, smallCrater.x);
    float smallFloor = 1.0 - smoothstep(0.0, 0.06, smallCrater.x);
    color = mix(color, rock * 0.7, smallFloor * 0.15);
    color = mix(color, rock * 1.1, smallRim * 0.1);

    // Fine surface detail and micro-texture
    float microDetail = gradientNoise(p * 35.0) * 0.025;
    float fineGrain = gradientNoise(p * 60.0) * 0.012;
    color += microDetail + fineGrain;

    return color;
}

// ============================================================
// Gas giant (v0.6.0) — jet streams, lightning, ammonia clouds,
// deeper domain warping, more turbulent flow
// ============================================================

vec3 gasGiantColor(vec3 pos) {
    vec3 p = pos * uNoiseScale + uNoiseSeed;
    float lat = pos.y;

    // Primary banding with deep domain warping
    float bandWarp1 = fbm(p * 0.5, 6) * 1.5;
    float bandWarp2 = fbm(p * 1.0 + 5.0, 5) * 0.8;
    float bandWarp3 = fbm(p * 2.5 + 12.0, 4) * 0.3;
    float bands = sin(lat * 22.0 + bandWarp1 + bandWarp2 + bandWarp3) * 0.5 + 0.5;

    // Secondary finer bands
    float fineBandWarp = fbm(p * 2.0 + 10.0, 5) * 1.0;
    float fineBands = sin(lat * 55.0 + fineBandWarp) * 0.5 + 0.5;

    // Tertiary micro-bands
    float microBands = sin(lat * 130.0 + fbm(p * 3.5, 4) * 2.0) * 0.5 + 0.5;

    // Quaternary ultra-fine bands (visible at high LOD)
    float ultraBands = sin(lat * 300.0 + fbm(p * 6.0, 3) * 1.5) * 0.5 + 0.5;

    float bandMix = bands * 0.48 + fineBands * 0.28 + microBands * 0.16 + ultraBands * 0.08;

    // Band color: zones (light) and belts (dark)
    vec3 zoneColor = uColorPrimary * 1.2;
    vec3 beltColor = uColorSecondary * 0.8;
    vec3 color = mix(beltColor, zoneColor, bandMix);

    // Jet streams at band boundaries (high-velocity shear)
    float bandEdge = abs(fract(lat * 11.0 + bandWarp1 * 0.1) - 0.5) * 2.0;
    float jetStream = smoothstep(0.85, 1.0, bandEdge);
    vec3 jetWarp = domainWarp2(p * 4.0 + vec3(lat * 5.0, 0.0, 0.0), 0.6, 0.3);
    float jetTurb = fbmNorm(jetWarp, 6);
    color = mix(color, mix(zoneColor, beltColor, jetTurb), jetStream * 0.3);

    // Chevron patterns between bands
    float chevronLat = sin(lat * 22.0) * 0.5 + 0.5;
    float chevronEdge = smoothstep(0.42, 0.58, chevronLat);
    float chevron = fbm(vec3(p.x * 3.0 + lat * 10.0, p.y * 0.5, p.z * 3.0 + lat * 10.0), 5);
    color = mix(color, mix(zoneColor, beltColor, 0.5), chevron * chevronEdge * 0.18);

    // Ammonia crystal cloud tops (bright white patches in zones)
    float ammoniaNoise = fbmNorm(domainWarp(p * 3.0 + 40.0, 0.4), 6);
    float ammonia = smoothstep(0.65, 0.82, ammoniaNoise) * smoothstep(0.4, 0.6, bandMix);
    vec3 ammoniaColor = vec3(0.95, 0.93, 0.88);
    color = mix(color, ammoniaColor, ammonia * 0.25);

    // Great Red Spot analog
    vec3 spotCenter = normalize(vec3(0.3 + uNoiseSeed * 0.001, -0.2, 0.5));
    float spotDist = length(pos - spotCenter);
    float spot = smoothstep(0.32, 0.10, spotDist);

    if (spot > 0.0) {
        float angle = atan(pos.z - spotCenter.z, pos.x - spotCenter.x);
        // Multi-layer swirling
        float swirl1 = fbm(vec3(spotDist * 8.0 + angle * 2.5, p.y, uNoiseSeed), 6);
        float swirl2 = fbm(vec3(spotDist * 15.0 - angle * 4.0, p.y + 5.0, uNoiseSeed), 5);
        float swirl = swirl1 * 0.6 + swirl2 * 0.4;
        vec3 spotColor = mix(uColorAccent, uColorPrimary * 1.4, swirl * 0.5 + 0.5);
        // Darker eye with turbulent interior
        float eye = smoothstep(0.05, 0.015, spotDist);
        spotColor = mix(spotColor, uColorAccent * 0.6, eye);
        // Bright ring around the spot
        float ring = smoothstep(0.10, 0.12, spotDist) * smoothstep(0.15, 0.12, spotDist);
        spotColor = mix(spotColor, zoneColor * 1.2, ring * 0.3);
        color = mix(color, spotColor, spot * 0.85);
    }

    // Smaller storm vortices (more of them, varied sizes)
    for (int s = 0; s < 5; s++) {
        vec3 sc = normalize(vec3(
            hash(vec3(uNoiseSeed + float(s) * 7.0)),
            (hash(vec3(uNoiseSeed + float(s) * 13.0)) - 0.5) * 1.6,
            hash(vec3(uNoiseSeed + float(s) * 19.0))
        ));
        float stormSize = 0.06 + hash(vec3(uNoiseSeed + float(s) * 23.0)) * 0.10;
        float sd = length(pos - sc);
        float ss = smoothstep(stormSize + 0.04, stormSize * 0.3, sd);
        if (ss > 0.0) {
            float angle = atan(pos.z - sc.z, pos.x - sc.x);
            float swirlSmall = fbm(vec3(sd * 14.0 + angle * 2.0, p.y + float(s), p.z), 5);
            vec3 stormCol = mix(zoneColor, uColorAccent, swirlSmall * 0.5 + 0.5);
            color = mix(color, stormCol, ss * 0.5);
        }
    }

    // General turbulence in belt regions
    float turbulence = fbm(domainWarp(p * 3.5, 0.35), 6);
    float beltRegion = smoothstep(0.3, 0.5, 1.0 - bandMix);
    color = mix(color, uColorAccent * 0.85, turbulence * beltRegion * 0.18);

    // Lightning flash spots in deep storm regions
    float lightningNoise = hash(p * 50.0 + uNoiseSeed * 3.0);
    float stormDepth = smoothstep(0.55, 0.75, 1.0 - bandMix) * smoothstep(0.6, 0.8, turbulence);
    float lightning = step(0.997, lightningNoise) * stormDepth;
    color += vec3(0.6, 0.7, 1.0) * lightning * 0.4;

    // Longitudinal streaks
    float streaks = gradientNoise(vec3(lat * 45.0, p.y * 0.3, p.z * 0.3)) * 0.035;
    color += streaks;

    // Fine atmospheric grain
    float grain = gradientNoise(p * 40.0) * 0.015;
    color += grain;

    return color;
}

// ============================================================
// Ice giant (v0.6.0) — wind shear, atmospheric depth layers,
// brighter methane absorption, more detail
// ============================================================

vec3 iceGiantColor(vec3 pos) {
    vec3 p = pos * uNoiseScale + uNoiseSeed;
    float lat = pos.y;

    // Smooth wide bands with gentle warping
    float bandWarp = fbm(p * 0.35, 5) * 1.0;
    float bands = sin(lat * 14.0 + bandWarp) * 0.5 + 0.5;

    // Secondary subtle bands
    float fineBands = sin(lat * 35.0 + fbm(p * 1.0, 4) * 1.2) * 0.5 + 0.5;

    // Tertiary micro-bands
    float microBands = sin(lat * 80.0 + fbm(p * 2.5, 3) * 1.5) * 0.5 + 0.5;

    float bandMix = bands * 0.55 + fineBands * 0.30 + microBands * 0.15;
    vec3 color = mix(uColorPrimary, uColorSecondary, bandMix * 0.45 + 0.28);

    // Wind shear patterns at different altitudes
    vec3 shearCoord = p * 2.0 + vec3(lat * 3.0, 0.0, 0.0);
    vec3 sheared = domainWarp2(shearCoord, 0.8, 0.4);
    float windShear = fbmNorm(sheared, 7);
    float shearPattern = smoothstep(0.3, 0.7, windShear);
    color = mix(color, uColorPrimary * 0.9, shearPattern * 0.12);

    // High-altitude methane haze layers (multiple)
    vec3 hazeCoord1 = domainWarp2(p * 1.8, 0.6, 0.3);
    vec3 hazeCoord2 = domainWarp2(p * 3.0 + 15.0, 0.4, 0.2);
    float haze1 = fbmNorm(hazeCoord1, 7);
    float haze2 = fbmNorm(hazeCoord2, 6);
    float hazeLayer = smoothstep(0.35, 0.65, haze1) * 0.6 + smoothstep(0.4, 0.7, haze2) * 0.4;
    vec3 hazeColor = uColorAccent * vec3(0.88, 0.94, 1.0);
    color = mix(color, hazeColor, hazeLayer * 0.22);

    // Wispy cloud streaks with more detail
    float cloudWarp = fbm(p * 1.5 + 20.0, 6) * 1.8;
    float clouds = sin(lat * 28.0 + cloudWarp + p.x * 3.5) * 0.5 + 0.5;
    float wisps = smoothstep(0.52, 0.72, clouds);
    // Thin cirrus-like streaks
    float cirrus = fbmNorm(vec3(p.x * 8.0 + lat * 12.0, p.y * 0.5, p.z * 8.0), 5);
    float cirrusStreak = smoothstep(0.6, 0.8, cirrus) * smoothstep(0.3, 0.5, abs(lat));
    color = mix(color, hazeColor * 1.1, wisps * 0.18);
    color = mix(color, hazeColor * 1.15, cirrusStreak * 0.12);

    // Dark spot (Neptune-like)
    vec3 darkSpotCenter = normalize(vec3(
        0.5 + uNoiseSeed * 0.002, -0.15, 0.4));
    float darkSpotDist = length(pos - darkSpotCenter);
    float darkSpot = smoothstep(0.22, 0.07, darkSpotDist);
    if (darkSpot > 0.0) {
        float angle = atan(pos.z - darkSpotCenter.z, pos.x - darkSpotCenter.x);
        float swirl = fbm(vec3(darkSpotDist * 12.0 + angle * 1.5, p.y, p.z), 5);
        vec3 spotColor = uColorPrimary * 0.55;
        color = mix(color, spotColor, darkSpot * 0.55);
        // Bright companion cloud
        float companion = smoothstep(0.24, 0.19, darkSpotDist) *
                          smoothstep(0.12, 0.16, darkSpotDist);
        color = mix(color, hazeColor * 1.4, companion * 0.45);
    }

    // Secondary smaller dark spot
    vec3 spot2Center = normalize(vec3(
        -0.3 + uNoiseSeed * 0.003, 0.3, -0.5));
    float spot2Dist = length(pos - spot2Center);
    float spot2 = smoothstep(0.12, 0.04, spot2Dist);
    if (spot2 > 0.0) {
        vec3 spot2Color = uColorPrimary * 0.65;
        color = mix(color, spot2Color, spot2 * 0.35);
    }

    // Polar brightening with aurora-like glow
    float polarBright = smoothstep(0.55, 0.92, abs(lat));
    vec3 polarColor = uColorAccent * 0.85 + vec3(0.06, 0.10, 0.14);
    // Aurora bands
    float aurora = sin(abs(lat) * 40.0 + fbm(p * 5.0, 3) * 3.0) * 0.5 + 0.5;
    aurora = smoothstep(0.6, 0.9, aurora) * polarBright;
    color = mix(color, polarColor, polarBright * 0.35);
    color += vec3(0.05, 0.15, 0.10) * aurora * 0.3;

    // Atmospheric depth: slight darkening at limb
    float viewAngle = abs(dot(normalize(pos), normalize(pos)));
    float microDetail = gradientNoise(p * 25.0) * 0.02;
    float fineGrain = gradientNoise(p * 50.0) * 0.008;
    color += microDetail + fineGrain;

    return color;
}


// ============================================================
// Bump mapping (v0.6.0) — multi-scale for close-up detail
// ============================================================

vec3 computeBumpNormal(vec3 normal, vec3 pos, float bumpStrength) {
    vec3 p = pos * uNoiseScale + uNoiseSeed;
    vec3 deriv = multiScaleDerivatives(p);

    // Add tectonic plate boundary bumps
    float eps = 0.002;
    float c0 = crackle(p * 0.6);
    float cx = crackle((p + vec3(eps, 0, 0)) * 0.6);
    float cz = crackle((p + vec3(0, 0, eps)) * 0.6);
    deriv += vec3(cx - c0, 0.0, cz - c0) / eps * 0.3;

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
// Main (v0.6.0)
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

    // Bump mapping for rocky planets (land only), gas/ice get subtle bumps too
    vec3 shadingNormal = normal;
    if (uPlanetType == 0 && !isOcean) {
        shadingNormal = computeBumpNormal(normal, vLocalPos, 0.4);
    } else if (uPlanetType == 1) {
        // Subtle cloud-top bumps for gas giants
        vec3 p = vLocalPos * uNoiseScale + uNoiseSeed;
        vec3 deriv = noiseDerivatives(p * 3.0) * 0.08;
        vec3 tangent = normalize(cross(normal, vec3(0.0, 1.0, 0.0)));
        if (length(tangent) < 0.001) tangent = normalize(cross(normal, vec3(1.0, 0.0, 0.0)));
        vec3 bitangent = normalize(cross(normal, tangent));
        shadingNormal = normalize(normal + (tangent * deriv.x + bitangent * deriv.z));
    }

    // Lighting
    float NdotL = dot(shadingNormal, lightDir);
    float NdotLRaw = dot(normal, lightDir);

    // Soft terminator with subsurface scattering
    float terminator = smoothstep(-0.08, 0.2, NdotL);
    float subsurface = smoothstep(-0.3, 0.0, NdotLRaw) * 0.10;

    // Ambient: hemisphere lighting
    vec3 skyAmbient = vec3(0.05, 0.07, 0.11) * uStarColor;
    vec3 groundAmbient = surfaceColor * 0.025;
    float ambientMix = dot(normal, vec3(0, 1, 0)) * 0.5 + 0.5;
    vec3 ambient = mix(groundAmbient, skyAmbient, ambientMix) * surfaceColor;

    // Diffuse
    vec3 diffuse = surfaceColor * terminator * uStarColor;

    // Subsurface scattering tint
    vec3 sss = surfaceColor * subsurface * uStarColor * vec3(1.0, 0.7, 0.5);

    // Specular: Blinn-Phong
    vec3 halfDir = normalize(lightDir + viewDir);
    float NdotH = max(dot(shadingNormal, halfDir), 0.0);

    float specPower, specStrength;
    if (uPlanetType == 0) {
        if (isOcean) {
            specPower = 256.0;
            specStrength = 0.65;
        } else {
            specPower = 32.0;
            specStrength = 0.05;
        }
    } else if (uPlanetType == 1) {
        specPower = 16.0;
        specStrength = 0.08;
    } else {
        specPower = 24.0;
        specStrength = 0.10;
    }

    float spec = pow(NdotH, specPower);
    vec3 specular = uStarColor * spec * specStrength * terminator;

    // Fresnel for ocean
    if (uPlanetType == 0 && isOcean) {
        float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 5.0);
        specular *= (0.04 + 0.96 * fresnel);
    }

    // Rim light
    float rim = 1.0 - max(dot(normal, viewDir), 0.0);
    rim = pow(rim, 4.0);
    vec3 rimLight = uStarColor * rim * 0.06 * max(-NdotLRaw, 0.0);

    // Night side: city lights for rocky planets
    vec3 nightGlow = vec3(0.0);
    if (uPlanetType == 0 && !isOcean && NdotLRaw < -0.05) {
        float nightFactor = smoothstep(-0.05, -0.25, NdotLRaw);
        float lights = noise3D(vLocalPos * uNoiseScale * 15.0 + uNoiseSeed);
        lights = smoothstep(0.62, 0.72, lights);
        // Cluster lights near coasts and in temperate zones
        float latitude = abs(vLocalPos.y);
        float coastProx = smoothstep(0.1, 0.0, height - 0.42);
        float tempZone = smoothstep(0.2, 0.5, latitude) * smoothstep(0.8, 0.6, latitude);
        float lightDensity = 0.3 + coastProx * 0.4 + tempZone * 0.3;
        nightGlow = vec3(1.0, 0.85, 0.5) * lights * nightFactor * 0.02 * lightDensity;
    }

    // Volcanic glow on night side
    if (uPlanetType == 0 && NdotLRaw < 0.0) {
        vec3 p = vLocalPos * uNoiseScale + uNoiseSeed;
        float plates = crackle(p * 0.6);
        float plateBoundary = smoothstep(0.0, 0.08, plates);
        float volcanoNoise = fbmNorm(p * 3.0 + 50.0, 5);
        float volcanoSpot = (1.0 - plateBoundary) * smoothstep(0.6, 0.8, volcanoNoise);
        float nightFactor = smoothstep(0.0, -0.15, NdotLRaw);
        vec3 lavaGlow = vec3(1.0, 0.25, 0.02) * volcanoSpot * nightFactor * 0.04;
        nightGlow += lavaGlow;
    }

    vec3 finalColor = ambient + diffuse + sss + specular + rimLight + nightGlow;

    // Tone mapping (Reinhard, prevents blowout)
    finalColor = finalColor / (finalColor + vec3(1.0)) * 1.15;

    FragColor = vec4(finalColor, 1.0);
}