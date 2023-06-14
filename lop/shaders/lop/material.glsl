#ifndef SHADERS_LOP_MATERIAL_GLSL
#define SHADERS_LOP_MATERIAL_GLSL

struct Material
{
    vec3 baseColor;
    float roughness;

    float metallic;
    float ior;
    float specularTransmission;
    float specularTint;

    vec3  transmittance;
    float atDistance;

    vec3  emission;
    float clearcoat;

    float clearcoatGloss;
    float pad1          ;
    float pad2          ;
    float pad3          ;
};

#include "lop/color.glsl"
#include "lop/math.glsl"
#include "lop/random.glsl"
#include "lop/sampling.glsl"

//
#include "lop/brdf/diffuse.glsl"
#include "lop/brdf/fresnel.glsl"
#include "lop/brdf/specular.glsl"

// For all functions, every vectors must be in shading normal space, and n must be (0, 0, 1)

vec3 evalMaterial(Material material, vec3 wo, vec3 wi, float t)
{
    float win       = clamp(abs(wi.z),       1e-4, 1.);
    float won       = clamp(abs(wo.z),       1e-4, 1.);
    bool  entering  = wi.z > 0.;
    bool  doReflect = wi.z * wo.z > 0.f;
    
    vec3 weight = vec3(1.);
    if (!entering && material.specularTransmission > 0.f && material.atDistance > 0.)
        weight *= exp(log(material.transmittance) * abs(t) / material.atDistance);

    const float AirIOR = 1.f;
    float etaI = entering ? material.ior : AirIOR;
    float etaT = entering ? AirIOR : material.ior;
  
    if( doReflect )
    {
        vec3  h = normalize(wi + wo);
        vec3 f  = getDisneyFresnel(material, wi, wo, h);

        float roughness = max(1e-4, material.roughness);
        float alpha     = max(1e-4, roughness * roughness);
        float alpha2    = max(1e-4, alpha * alpha);

        float nh  = clamp(abs(h.z),        1e-4, 1.);
        float lh  = clamp(abs(dot(wi, h)), 1e-4, 1.);

        float diffuseWeight = 1. - material.specularTransmission;
        vec3  diffuse       = diffuseWeight * evalDisneyDiffuse(material, wo, wi);
        float specular      = evalSpecularReflection(material, wo, wi);

        float woh = clamp(abs(dot(wo, h)), 1e-4, 1.);
        float ccf = fresnelSchlick(vec3(iorToReflectance(1.5)), woh).x;

        return weight * ((1. - f) * diffuse + f * specular + ccf * evalClearCoat(material, wo, wi));
    }
    else 
    {
        vec3 h = normalize(-(etaI * wi + etaT * wo));
        vec3 f = getDisneyFresnel(material, wi, wo, h);

        float transmissionWeight   = material.specularTransmission;
        float specularTransmission = transmissionWeight * evalSpecularTransmission(material, wo, wi);
        return weight * (sqrt(material.baseColor) * (1. - f) * specularTransmission);
    }
}

vec3 sampleMaterial(Material material, vec3 wo, float t, inout uvec4 seed, out vec3 weight, out float pdf)
{
    float roughness = max(1e-4, material.roughness);
    float alpha     = max(1e-4, roughness * roughness);
    float alpha2    = max(1e-4, alpha * alpha);

    float inside   = sign(wo.z);
    bool  isInside = inside < 0.;

    pdf    = 1.;
    weight = vec3(1.);
    if (isInside && material.specularTransmission > 0.f && material.atDistance > 0.)
        weight *= exp(log(material.transmittance) * abs(t) / material.atDistance);

    vec3 h    = vec3(0., 0., 1.);
    vec4 alea = prng(seed);
    if(material.clearcoat > 0.) 
    {
        float ccRoughness = getClearCoatRoughness(material);
        float ccAlpha     = max(1e-4, ccRoughness * ccRoughness);
        float ccAlpha2    = max(1e-4, ccAlpha * ccAlpha);
    
        vec3 ccH = h;
        if(ccRoughness > .0)
            ccH = sampleGGXVNDF(wo, ccAlpha, ccAlpha, alea.z, alea.w);
        
        float woh = clamp(abs(dot(wo, ccH)), 1e-4, 1.);
        float ccf = fresnelSchlick(vec3(iorToReflectance(1.5)), woh).x;
        if(alea.y < material.clearcoat * ccf) 
        {
            vec3  wi = reflect(-wo,ccH);
    
            float hn  = clamp(abs(ccH.z),        1e-4, 1.);
            float woh = clamp(abs(dot(wo, ccH)), 1e-4, 1.);
            float wih = clamp(abs(dot(wi, ccH)), 1e-4, 1.);
    
            float g1 = getSmithG1GGX(woh, ccAlpha2);
            float g2 = getSmithG1GGX(wih, ccAlpha2) * g1;
            weight  *= g2 / max(1e-4, g1);
            pdf     *= 1.;
            return wi;
        }
    }

    if (roughness > 0.f)
        h = sampleGGXVNDF(wo, alpha, alpha, alea.z, alea.w);
    
    vec3 f = getDisneyFresnel(material, wo, wo, h);
    
    float specularWeight = length(f);
    bool  fullSpecular   = roughness == 0. && material.metallic == 1.;
    float type           = fullSpecular ? 0. : alea.x;

    if (type < specularWeight)
    {
        vec3 wi = reflect(-wo, h);

        weight *= f * evalSpecularReflection(material, wo, wi);
        pdf    *= getPdfSpecularReflection(material, wo, wi);
        pdf    *= fullSpecular ? 1. : specularWeight;
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

        weight *= sqrt(material.baseColor) * (1. - f) * evalSpecularTransmission(material, wo, wi);
        pdf    *= getPdfSpecularTransmission(material, wo, wi);

        return wi;
    }

    vec3 wi = sampleDisneyDiffuse(material, wo, prng(seed).xy);
    weight  = (1. - f) * evalDisneyDiffuse(material, wo, wi);
    pdf     = getPdfDisneyDiffuse(material, wo, wi);

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