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

// --- Noise functions (in-shader) ---
float hash(vec3 p) {
    p = fract(p * vec3(443.897, 441.423, 437.195));
    p += dot(p, p.yzx + 19.19);
    return fract((p.x + p.y) * p.z);
}

float noise3D(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

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

vec3 rockyColor(vec3 pos) {
    vec3 p = pos * uNoiseScale + uNoiseSeed;
    float n = fbm(p, 6);
    float n2 = fbm(p * 2.0 + 5.0, 4);

    // Terrain: mix primary and secondary based on noise
    vec3 color = mix(uColorPrimary, uColorSecondary, n);

    // Polar ice caps
    float latitude = abs(pos.y);
    if (latitude > 0.75) {
        float iceFactor = smoothstep(0.75, 0.9, latitude);
        color = mix(color, uColorAccent, iceFactor);
    }

    // Craters / variation
    float crater = smoothstep(0.55, 0.6, n2);
    color = mix(color, uColorPrimary * 0.7, crater * 0.3);

    return color;
}

vec3 gasGiantColor(vec3 pos) {
    vec3 p = pos * uNoiseScale + uNoiseSeed;

    // Bands based on latitude
    float bands = sin(pos.y * 12.0 + fbm(p * 0.5, 3) * 2.0) * 0.5 + 0.5;
    vec3 color = mix(uColorPrimary, uColorSecondary, bands);

    // Storm spots
    float storm = fbm(p * 3.0, 4);
    if (storm > 0.65) {
        float stormFactor = smoothstep(0.65, 0.8, storm);
        color = mix(color, uColorAccent, stormFactor * 0.6);
    }

    // Subtle turbulence
    float turb = fbm(p * 6.0, 3) * 0.1;
    color += turb;

    return color;
}

vec3 iceGiantColor(vec3 pos) {
    vec3 p = pos * uNoiseScale + uNoiseSeed;

    // Smooth bands with less contrast
    float bands = sin(pos.y * 8.0 + fbm(p * 0.3, 3) * 1.5) * 0.5 + 0.5;
    vec3 color = mix(uColorPrimary, uColorSecondary, bands * 0.6 + 0.2);

    // Wispy cloud features
    float clouds = fbm(p * 4.0, 5);
    color = mix(color, uColorAccent, clouds * 0.2);

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
    float diff = max(dot(normal, lightDir), 0.0);

    // Ambient so the dark side isn't pure black
    vec3 ambient = surfaceColor * 0.08;
    vec3 diffuse = surfaceColor * diff * uStarColor;

    // Simple specular highlight
    vec3 viewDir = normalize(uCameraPos - vWorldPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    vec3 specular = uStarColor * spec * 0.15;

    vec3 finalColor = ambient + diffuse + specular;

    FragColor = vec4(finalColor, 1.0);
}
