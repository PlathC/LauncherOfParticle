#ifndef SHADERS_LOP_RAY_GLSL
#define SHADERS_LOP_RAY_GLSL

#include "lop/material.glsl"

struct HitInfo
{
    Material material;

    vec3  position;
    float t;
    vec3  shadingNormal;
    vec3  geometricNormal;
    bool  hit;
};

// A Fast and Robust Method for Avoiding Self-Intersection, Carsten Wächter and Nikolaus Binder, NVIDIA
// Reference:
// https://github.com/Apress/ray-tracing-gems/blob/master/Ch_06_A_Fast_and_Robust_Method_for_Avoiding_Self-Intersection/offset_ray.cu
vec3 offsetRay(vec3 p, vec3 n)
{
    const float origin      = 1.0f / 32.0f;
    const float float_scale = 1.0f / 65536.0f;
    const float int_scale   = 256.0f;

    ivec3 of_i = ivec3(int_scale * n.x, int_scale * n.y, int_scale * n.z);

    vec3 p_i = vec3(intBitsToFloat(floatBitsToInt(p.x) + ((p.x < 0.) ? -of_i.x : of_i.x)),
                    intBitsToFloat(floatBitsToInt(p.y) + ((p.y < 0.) ? -of_i.y : of_i.y)),
                    intBitsToFloat(floatBitsToInt(p.z) + ((p.z < 0.) ? -of_i.z : of_i.z)));

    return vec3(abs(p.x) < origin ? p.x + float_scale * n.x : p_i.x,
                abs(p.y) < origin ? p.y + float_scale * n.y : p_i.y,
                abs(p.z) < origin ? p.z + float_scale * n.z : p_i.z);
}

#endif // SHADERS_LOP_RAY_GLSL