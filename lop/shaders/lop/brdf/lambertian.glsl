#ifndef SHADERS_LOP_BRDF_LAMBERTIAN_GLSL
#define SHADERS_LOP_BRDF_LAMBERTIAN_GLSL

#include "lop/material.glsl"
#include "lop/sampling.glsl"

vec3 evalLambertian(const Material material)
{
    return material.baseColor /* * (1. - material.metalness) */ / Pi;
}

vec3 sampleLambertian(const vec3 wo, const vec2 u)
{
    vec3 wi = sampleCosine(u);
    if (wo.z < 0.)
        wi.z *= -1.;
    return wi;
}

float getPdfLambertian(const vec3 wo, const vec3 wi, const vec3 n) { return wo.z * wi.z > 0. ? abs(wi.z) / Pi : 0.; }

#endif // SHADERS_LOP_BRDF_LAMBERTIAN_GLSL