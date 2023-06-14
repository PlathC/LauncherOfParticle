#ifndef SHADERS_LOP_BRDF_FRESNEL_GLSL
#define SHADERS_LOP_BRDF_FRESNEL_GLSL

float iorToReflectance(float ior){ return pow2(ior - 1.) / pow2(ior + 1.); }

// Schlick, C. (1994). An Inexpensive BRDF Model for Physically-based Rendering. 
// In Computer Graphics Forum (Vol. 13, Issue 3, pp. 233–246). Wiley. 
vec3 fresnelSchlick(vec3 r0, float u) { return r0 + (1. - r0) * pow(1. - u, 5.); }

// Linear interpolation between Fresnel metallic and dielectric based on 
// material.metallic. 
// Found: https://schuttejoe.github.io/post/disneybsdf/
vec3 getDisneyFresnel(Material material, vec3 wo, vec3 wi, vec3 h)
{
    float luminance = getLuminance(material.baseColor);
    vec3 tint       = luminance > 0. ? material.baseColor * (1. / luminance) : vec3(1.);
    
    vec3 baseR0 = vec3(iorToReflectance(material.ior));
    vec3 r0     = mix(baseR0, tint, material.specularTint);
    r0          = mix(r0, material.baseColor, material.metallic);

    float wih = clamp(abs(dot(wi, h)), 1e-4, 1.);
    float woh = clamp(abs(dot(wo, h)), 1e-4, 1.);
    
    vec3 dielectricF = fresnelSchlick(baseR0, woh);
    vec3 metallicF   = fresnelSchlick(r0, wih);
    
    return mix(dielectricF, metallicF, material.metallic);
}

#endif // SHADERS_LOP_BRDF_FRESNEL_GLSL