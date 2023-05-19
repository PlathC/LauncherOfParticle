#ifndef SHADERS_LOP_BRDF_DIFFUSE_GLSL
#define SHADERS_LOP_BRDF_DIFFUSE_GLSL

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

vec3 evalDisneyDiffuse(Material material, vec3 wo, vec3 wi)
{
    float alpha = max(1e-4, material.roughness * material.roughness);
    
    vec3  h   = normalize(wo + wi);
    float wih = clamp(dot(wi, h), 0., 1.);
    float won = clamp(abs(wo.z), 1e-4, 1.);
    float win = clamp(abs(wi.z), 1e-4, 1.);
    
    float fd90 = 0.5 + 2. * alpha * wih * wih;
    float f1   = 1. + (fd90 - 1.) * pow(1. - win, 5.);
    float f2   = 1. + (fd90 - 1.) * pow(1. - won, 5.);
    return material.baseColor * OneOverPi * (1. - material.metallic) * f1 * f2;
}

vec3 sampleDisneyDiffuse(Material material, vec3 wo, const vec2 u) 
{
    vec3 wi = sampleCosine(u);
    if (wo.z < 0.)
        wi.z *= -1.;
    return wi;
}

float getPdfDisneyDiffuse(Material material, vec3 wo, vec3 wi) 
{
    return wo.z * wi.z > 0. ? abs(wi.z) * OneOverPi : 0.;
}

#endif // SHADERS_LOP_BRDF_DIFFUSE_GLSL