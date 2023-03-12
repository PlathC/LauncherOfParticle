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

vec3 evalLambertian(const Material material)
{
    return material.baseColor * (1. - material.metalness) / Pi;
}
vec3 sampleLambertian(const vec3 wo, const vec2 u)
{
    vec3 wi = sampleCosine(u);
    if (wo.z < 0.)
        wi.z *= -1.;
    return wi;
}
float getPdfLambertian(const vec3 n, const vec3 wo, const vec3 wi) { return wo.z * wi.z > 0. ? abs(wi.z) / Pi : 0.; }

#endif // SHADERS_PTO_MATERIAL_GLSL