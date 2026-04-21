#version 410 core

in vec3 vColor;
in float vLuminosity;

out vec4 FragColor;

void main() {
    // Circular point with soft edge
    vec2 coord = gl_PointCoord * 2.0 - 1.0;
    float dist = length(coord);

    // Soft glow falloff
    float alpha = smoothstep(1.0, 0.0, dist);
    // Brighter core
    float core = smoothstep(0.5, 0.0, dist);

    vec3 color = vColor * (0.6 + core * 0.6);

    // Boost bright stars
    color *= (0.5 + sqrt(vLuminosity) * 0.5);

    FragColor = vec4(color, alpha * 0.9);
}
