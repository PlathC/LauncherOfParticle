#ifndef SHADERS_LOP_RANDOM_GLSL
#define SHADERS_LOP_RANDOM_GLSL

// Reference: https://www.shadertoy.com/view/XlGcRh
// Hash Functions for GPU Rendering. Mark Jarzynski, & Marc Olano (2020).
// Journal of Computer Graphics Techniques (JCGT), 9(3), 20–38.
uvec4 pcg4d(uvec4 v)
{
    v = v * 1664525u + 1013904223u;

    v.x += v.y * v.w;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v.w += v.y * v.z;

    v ^= v >> 16u;

    v.x += v.y * v.w;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v.w += v.y * v.z;

    return v;
}

// https://github.com/boksajak/referencePT/blob/master/shaders/PathTracer.hlsl#L145
// Converts unsigned integer into float int range <0; 1) by using 23 most significant bits for mantissa
vec4 uintToFloat(uvec4 x) { return uintBitsToFloat(0x3f800000 | (x >> 9)) - 1.0f; }

vec4 prng(inout uvec4 p)
{
    p.w++;
    return uintToFloat(pcg4d(p));
}

uint lcg(inout uint prev)
{
    const uint LCG_A = 1664525u;
    const uint LCG_C = 1013904223u;
    prev             = (LCG_A * prev + LCG_C);
    return (prev & uint(0x00ffffff));
}

#endif // SHADERS_LOP_RANDOM_GLSL