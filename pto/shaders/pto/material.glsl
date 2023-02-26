#ifndef SHADERS_PTO_MATERIAL_GLSL
#define SHADERS_PTO_MATERIAL_GLSL

#include "pto/sampling.glsl"

struct Material
{
    vec3  baseColor;
    float roughness;

    vec3  emission;
    float transmission;

    float ior;
    float metalness;

    float pad1;
    float pad2;
};

vec3 evalLambertian(const Material material) { return material.baseColor * (1. - material.metalness) / Pi; }
vec3 sampleLambertian(const Material material, const vec3 n, const vec2 u, out vec3 l)
{
    const vec4 transformation = toLocalZ(n);
    
    l = normalize(multiply(conjugate(transformation), sampleCosine(u)));
    return material.baseColor * (1. - material.metalness); // Pre divided pdf
}

#endif // SHADERS_PTO_MATERIAL_GLSL