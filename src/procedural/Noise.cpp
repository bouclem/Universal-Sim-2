#include "procedural/Noise.h"
#include <cmath>
#include <array>

namespace usim {
namespace noise {

// Simplex noise implementation based on Stefan Gustavson's work.
// Adapted for C++ / GLM.

namespace {

const std::array<glm::vec3, 12> GRAD3 = {{
    {1,1,0}, {-1,1,0}, {1,-1,0}, {-1,-1,0},
    {1,0,1}, {-1,0,1}, {1,0,-1}, {-1,0,-1},
    {0,1,1}, {0,-1,1}, {0,1,-1}, {0,-1,-1}
}};

// Permutation table (doubled to avoid wrapping)
const std::array<int, 512> PERM = []() {
    std::array<int, 256> base = {{
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
        140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,
        247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
        57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
        74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
        60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
        65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,
        200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,
        52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
        207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
        119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
        129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
        218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
        81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
        184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,
        222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
    }};
    std::array<int, 512> result{};
    for (int i = 0; i < 512; ++i) {
        result[i] = base[i & 255];
    }
    return result;
}();

inline float dot(const glm::vec3& g, float x, float y, float z) {
    return g.x * x + g.y * y + g.z * z;
}

inline int fastFloor(float x) {
    int xi = static_cast<int>(x);
    return (x < static_cast<float>(xi)) ? xi - 1 : xi;
}

} // anonymous namespace

float simplex3D(const glm::vec3& v) {
    const float F3 = 1.0f / 3.0f;
    const float G3 = 1.0f / 6.0f;

    float s = (v.x + v.y + v.z) * F3;
    int i = fastFloor(v.x + s);
    int j = fastFloor(v.y + s);
    int k = fastFloor(v.z + s);

    float t = static_cast<float>(i + j + k) * G3;
    float x0 = v.x - (static_cast<float>(i) - t);
    float y0 = v.y - (static_cast<float>(j) - t);
    float z0 = v.z - (static_cast<float>(k) - t);

    int i1, j1, k1, i2, j2, k2;
    if (x0 >= y0) {
        if (y0 >= z0)      { i1=1; j1=0; k1=0; i2=1; j2=1; k2=0; }
        else if (x0 >= z0) { i1=1; j1=0; k1=0; i2=1; j2=0; k2=1; }
        else               { i1=0; j1=0; k1=1; i2=1; j2=0; k2=1; }
    } else {
        if (y0 < z0)       { i1=0; j1=0; k1=1; i2=0; j2=1; k2=1; }
        else if (x0 < z0)  { i1=0; j1=1; k1=0; i2=0; j2=1; k2=1; }
        else               { i1=0; j1=1; k1=0; i2=1; j2=1; k2=0; }
    }

    float x1 = x0 - static_cast<float>(i1) + G3;
    float y1 = y0 - static_cast<float>(j1) + G3;
    float z1 = z0 - static_cast<float>(k1) + G3;
    float x2 = x0 - static_cast<float>(i2) + 2.0f * G3;
    float y2 = y0 - static_cast<float>(j2) + 2.0f * G3;
    float z2 = z0 - static_cast<float>(k2) + 2.0f * G3;
    float x3 = x0 - 1.0f + 3.0f * G3;
    float y3 = y0 - 1.0f + 3.0f * G3;
    float z3 = z0 - 1.0f + 3.0f * G3;

    int ii = i & 255;
    int jj = j & 255;
    int kk = k & 255;
    int gi0 = PERM[ii + PERM[jj + PERM[kk]]] % 12;
    int gi1 = PERM[ii + i1 + PERM[jj + j1 + PERM[kk + k1]]] % 12;
    int gi2 = PERM[ii + i2 + PERM[jj + j2 + PERM[kk + k2]]] % 12;
    int gi3 = PERM[ii + 1 + PERM[jj + 1 + PERM[kk + 1]]] % 12;

    float n0 = 0.0f, n1 = 0.0f, n2 = 0.0f, n3 = 0.0f;

    float t0 = 0.6f - x0*x0 - y0*y0 - z0*z0;
    if (t0 >= 0.0f) { t0 *= t0; n0 = t0 * t0 * dot(GRAD3[gi0], x0, y0, z0); }

    float t1 = 0.6f - x1*x1 - y1*y1 - z1*z1;
    if (t1 >= 0.0f) { t1 *= t1; n1 = t1 * t1 * dot(GRAD3[gi1], x1, y1, z1); }

    float t2 = 0.6f - x2*x2 - y2*y2 - z2*z2;
    if (t2 >= 0.0f) { t2 *= t2; n2 = t2 * t2 * dot(GRAD3[gi2], x2, y2, z2); }

    float t3 = 0.6f - x3*x3 - y3*y3 - z3*z3;
    if (t3 >= 0.0f) { t3 *= t3; n3 = t3 * t3 * dot(GRAD3[gi3], x3, y3, z3); }

    return 32.0f * (n0 + n1 + n2 + n3);
}

float fbm(const glm::vec3& v, int octaves, float persistence, float lacunarity) {
    float total = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; ++i) {
        total += simplex3D(v * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return total / maxValue;
}

} // namespace noise
} // namespace usim
