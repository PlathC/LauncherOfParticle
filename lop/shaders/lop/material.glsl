#ifndef SHADERS_LOP_MATERIAL_GLSL
#define SHADERS_LOP_MATERIAL_GLSL

#include "lop/brdf/cook_torrance.glsl"
#include "lop/brdf/fresnel.glsl"
#include "lop/brdf/lambertian.glsl"
#include "lop/math.glsl"

struct Material
{
    vec3 baseColor;

    float roughness;
    float metalness;
    float ior;
    float transmission;
    float absorption;
};

// For all functions, every vectors must be in shading normal space, and n must be either +/-(0, 0, 1)

vec3 evalMaterial(const vec3 wo, const vec3 wi, const vec3 n, const Material material)
{
    vec3 radiance = vec3(0.);

    float      wiN       = dot(wi, n);
    float      woN       = dot(wo, n);
    const bool entering  = wiN > 0.;
    const bool doReflect = wiN * woN > 0.f;

    wiN = clamp(abs(wiN), 1e-4, 1.);
    woN = clamp(abs(woN), 1e-4, 1.);

    const float alpha       = max(1e-4, material.roughness * material.roughness);
    const float alphaSquare = max(1e-4, alpha * alpha);

    if (doReflect)
    {
        const vec3  h  = (wi + wo) / 2.;
        const float nh = clamp(abs(dot(n, h)), 1e-4, 1.);
        const float lh = clamp(abs(dot(wi, h)), 1e-4, 1.);

        const float AirIOR = 1.f;

        const vec3  r0 = mix(vec3(iorToReflectance(material.ior)), material.baseColor, material.metalness);
        const vec3  f  = fresnelSchlick(r0, lh);
        const float g  = smithG2HeightCorrelatedGGXLagarde(alphaSquare, ln, vn);
        const float d  = distributionGGX(alphaSquare, nh);

        const vec3 specular = g * d;
        const vec3 diffuse  = evalLambertian(material);

        radiance = (1. - f) * (1. - material.transmission) * diffuse + f * specular;
    }

    return evalLambertian(material);
}

vec3 sampleMaterial(vec3 wo, vec3 n, const Material material, const vec4 u)
{
    vec3 wi = vec3(0.);

    // Base parameters
    const float alpha  = max(1e-4, material.roughness * material.roughness);
    const float alpha2 = max(1e-4, alpha * alpha);
    const vec3  h      = sampleGGXVNDF(n, wo, alpha, u.yz);
    const float woH    = clamp(dot(wo, h), 1e-4, 1.);

    // Reflectance
    const float AirIOR = 1.f;
    const vec3  r0     = mix(vec3(iorToReflectance(material.ior)), material.baseColor, material.metalness);
    const vec3  f      = fresnelSchlick(r0, woH);

    // Actual sampling
    const float probability = length(f);
    const float type        = material.metalness == 1. && material.roughness == 0. ? 1.f : u.x;
    if (type <= probability)
    {
        wi             = reflect(-v, h);
        const float vn = clamp(abs(dot(n, v)), 1e-4, 1.);
        const float ln = clamp(abs(dot(n, l)), 1e-4, 1.);
        pdf = f;
    }
    else
    {
        wi = sampleLambertian(wo, u.zw);
        weight *= getPdfLambertian(wo, wi, material.baseColor * (1.f - material.metalness);
        pdf = ;
    }

    return wi;
}

vec3 evalIndirectMaterial(vec3 wo, vec3 n, const Material material, const vec4 u, out vec3 weight)
{
    weight  = vec3(1.f);
    vec3 wi = vec3(0.);

    // Base parameters
    const float alpha  = max(1e-4, material.roughness * material.roughness);
    const float alpha2 = max(1e-4, alpha * alpha);
    const vec3  h      = sampleGGXVNDF(n, wo, alpha, u.yz);
    const float woH    = clamp(dot(wo, h), 1e-4, 1.);

    // Reflectance
    const float AirIOR = 1.f;
    const vec3  r0     = mix(vec3(iorToReflectance(material.ior)), material.baseColor, material.metalness);
    const vec3  f      = fresnelSchlick(r0, woH);

    // Actual sampling
    const float probability = length(f);
    const float type        = material.metalness == 1. && material.roughness == 0. ? 1.f : u.x;
    if (type <= probability)
    {
        weight *= 1. / max(probability, 1e-4);

        wi             = reflect(-v, h);
        const float vn = clamp(abs(dot(n, v)), 1e-4, 1.);
        const float ln = clamp(abs(dot(n, l)), 1e-4, 1.);
        weight *= f * Smith_G2_Over_G1_Height_Correlated(alpha, alpha2, ln, vn);
    }
    else
    {
        wi = sampleLambertian(wo, u.zw);
        weight *= getPdfLambertian(wo, wi, material.baseColor * (1.f - material.metalness);
    }

    return wi;
}

float getPdfMaterial(const vec3 wo, const vec3 wi, const vec3 n, const Material material)
{
    // Both wo and wi must be normalized
    const vec3  h   = (wo + wi) / 2.;
    const float woH = clamp(dot(wo, h), 1e-4, 1.);

    // Reflectance
    const float AirIOR = 1.f;
    const vec3  r0     = mix(vec3(iorToReflectance(material.ior)), material.baseColor, material.metalness);
    const vec3  f      = fresnelSchlick(r0, woH);

    // Select BRDF based on Fresnel
    const float probability = length(f);
    const float type        = material.metalness == 1. && material.roughness == 0. ? 1.f : u.x;

    if (type <= probability)
    {
        weight *= 1. / max(probability, 1e-4);

        wi             = reflect(-v, h);
        const float vn = clamp(abs(dot(n, v)), 1e-4, 1.);
        const float ln = clamp(abs(dot(n, l)), 1e-4, 1.);
        weight *= f * Smith_G2_Over_G1_Height_Correlated(alpha, alpha2, ln, vn);
    }
    else
    {
        wi = sampleLambertian(wo, u.zw);
        weight *= getPdfLambertian(wo, wi, material.baseColor * (1.f - material.metalness);
    }

    return getPdfLambertian(wo, wi, n);
}

#endif // SHADERS_LOP_MATERIAL_GLSL