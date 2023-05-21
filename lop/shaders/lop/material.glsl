#ifndef SHADERS_LOP_MATERIAL_GLSL
#define SHADERS_LOP_MATERIAL_GLSL

struct Material
{
    vec3 baseColor;

    float roughness;
    float metallic;
    float ior;
    float specularTransmission;
    float absorption;
};

#include "lop/math.glsl"
#include "lop/random.glsl"
#include "lop/sampling.glsl"

//
#include "lop/brdf/diffuse.glsl"
#include "lop/brdf/fresnel.glsl"
#include "lop/brdf/specular.glsl"

// For all functions, every vectors must be in shading normal space, and n must be (0, 0, 1)

vec3 evalMaterial(Material material, vec3 wo, vec3 wi)
{
    float win       = clamp(abs(wi.z),       1e-4, 1.);
    float won       = clamp(abs(wo.z),       1e-4, 1.);
    bool  entering  = win > 0.;
    bool  doReflect = win * won > 0.f;
    
    const float AirIOR = 1.f;
    float etaI = entering ? material.ior : AirIOR;
    float etaT = entering ? AirIOR : material.ior;
  
    if( doReflect )
    {
        vec3  h   = normalize(wi + wo);
        float wih = clamp(abs(dot(wi, h)), 1e-4, 1.);
        vec3 r0   = mix(vec3(iorToReflectance(material.ior)), material.baseColor, material.metallic);
        vec3 f    = fresnelSchlick(r0, wih);

        float roughness = max(1e-4, material.roughness);
        float alpha     = max(1e-4, roughness * roughness);
        float alpha2    = max(1e-4, alpha * alpha);

        float nh  = clamp(abs(h.z),        1e-4, 1.);
        float lh  = clamp(abs(dot(wi, h)), 1e-4, 1.);

        float diffuseWeight = 1. - material.specularTransmission;
        vec3  diffuse       = diffuseWeight * evalDisneyDiffuse(material, wo, wi);
        float specular      = evalSpecularReflection(material, wo, wi);
        return (1. - f) * diffuse + f * specular;
    }
    else 
    {
        vec3 h    = normalize(-(etaI * wi + etaT * wo));
        float wih = clamp(abs(dot(wi, h)), 1e-4, 1.);
        vec3 r0   = mix(vec3(iorToReflectance(material.ior)), material.baseColor, material.metallic);
        vec3 f    = fresnelSchlick(r0, wih);

        float transmissionWeight   = material.specularTransmission;
        float specularTransmission = transmissionWeight * evalSpecularTransmission(material, wo, wi);
        return (1. - f) * specularTransmission;
    }
}

vec3 sampleMaterial(Material material, vec3 wo, inout uvec4 seed, out vec3 weight, out float pdf)
{
    float roughness = max(1e-4, material.roughness);
    float alpha     = max(1e-4, roughness * roughness);
    float alpha2    = max(1e-4, alpha * alpha);

    float inside   = sign(wo.z);
    bool  isInside = inside < 0.;

    vec3 h = vec3(0., 0., 1.);
    if (roughness > 0.f)
    {
        vec2 u = prng(seed).xy;
        h      = sampleGGXVNDF(wo, alpha, alpha, u.x, u.y);
    }

    float woh = clamp(abs(dot(wo, h)), 1e-4, 1.);
    vec3  r0  = mix(vec3(iorToReflectance(material.ior)), material.baseColor, material.metallic);
    vec3  f   = fresnelSchlick(r0, woh);

    float specularWeight = length(f);
    bool  fullSpecular   = roughness == 0. && material.metallic == 1.;
    float type           = fullSpecular ? 0. : prng(seed).x;

    if (type < specularWeight)
    {
        vec3 wi = reflect(-wo, h);

        weight = f * evalSpecularReflection(material, wo, wi);
        pdf    = getPdfSpecularReflection(material, wo, wi);
        pdf *= fullSpecular ? 1. : specularWeight;
        return wi;
    }

    float transmissionType           = type - specularWeight;
    float specularTransmissionWeight = (1. - specularWeight) * material.specularTransmission;
    if (transmissionType < specularTransmissionWeight)
    {
        const float AirIOR = 1.f;
        float       etaI   = isInside ? material.ior : AirIOR;
        float       etaT   = isInside ? AirIOR : material.ior;
        vec3        wi     = refract(-wo, h, etaI / etaT);

        weight = material.specularTransmission * vec3(1. - f) * evalSpecularTransmission(material, wo, wi);
        pdf    = material.specularTransmission * getPdfSpecularTransmission(material, wo, wi);

        return wi;
    }

    vec3 wi = sampleDisneyDiffuse(material, wo, prng(seed).xy);
    weight  = (1. - material.specularTransmission) * (1. - f) * evalDisneyDiffuse(material, wo, wi);
    pdf     = (1. - material.specularTransmission) * getPdfDisneyDiffuse(material, wo, wi);

    return wi;
}

float getPdfMaterial(Material material, vec3 wo, vec3 wi, inout uvec4 seed)
{
    float roughness = max(1e-4, material.roughness);
    vec3  r0        = mix(vec3(iorToReflectance(material.ior)), material.baseColor, material.metallic);

    vec3 h = normalize(wi + wo);
    vec3 f = fresnelSchlick(r0, abs(dot(wo, h)));

    float specularWeight = length(f);
    bool  fullSpecular   = roughness == 0. && material.metallic == 1.;
    float type           = fullSpecular ? 0. : prng(seed).x;
    if (type < specularWeight)
        return getPdfSpecularReflection(material, wo, wi) * (fullSpecular ? 1. : specularWeight);

    float transmissionType           = type - specularWeight;
    float specularTransmissionWeight = (1. - specularWeight) * material.specularTransmission;
    if (transmissionType < specularTransmissionWeight)
        return material.specularTransmission * (1. - specularWeight) * getPdfSpecularTransmission(material, wo, wi);

    return getPdfDisneyDiffuse(material, wo, wi) * (1. - material.specularTransmission) * (1. - specularWeight);
}

#endif // SHADERS_LOP_MATERIAL_GLSL