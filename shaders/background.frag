#version 410 core

in vec3 vDirection;

out vec4 FragColor;

// Hash for star placement
float hash(vec3 p) {
    p = fract(p * vec3(443.897, 441.423, 437.195));
    p += dot(p, p.yzx + 19.19);
    return fract((p.x + p.y) * p.z);
}

void main() {
    vec3 dir = normalize(vDirection);

    // Procedural starfield
    // Divide sky into cells and place a star in some cells
    vec3 cell = floor(dir * 80.0);
    float starChance = hash(cell);

    vec3 color = vec3(0.0, 0.0, 0.01); // Very dark blue-black background

    if (starChance > 0.97) {
        // Star brightness and color variation
        float brightness = hash(cell + 1.0) * 0.8 + 0.2;
        float colorTemp = hash(cell + 2.0);

        vec3 starColor;
        if (colorTemp < 0.2) {
            starColor = vec3(1.0, 0.7, 0.5); // Warm
        } else if (colorTemp > 0.8) {
            starColor = vec3(0.7, 0.8, 1.0); // Cool blue
        } else {
            starColor = vec3(1.0);            // White
        }

        // Point-like: brighter at cell center
        vec3 cellFract = fract(dir * 80.0) - 0.5;
        float dist = length(cellFract);
        float star = smoothstep(0.15, 0.0, dist);

        color += starColor * brightness * star;
    }

    FragColor = vec4(color, 1.0);
}
