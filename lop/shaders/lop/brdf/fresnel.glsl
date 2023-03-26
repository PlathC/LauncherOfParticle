#ifndef SHADERS_LOP_BRDF_FRESNEL_GLSL
#define SHADERS_LOP_BRDF_FRESNEL_GLSL

float iorToReflectance(float ior){ return pow2(ior - 1.) / pow2(ior + 1.); }

// Schlick, C. (1994). An Inexpensive BRDF Model for Physically-based Rendering. 
// In Computer Graphics Forum (Vol. 13, Issue 3, pp. 233–246). Wiley. 
vec3 fresnelSchlick(vec3 r0, float u) { return r0 + (1. - r0) * pow(1. - u, 5.); }

#endif // SHADERS_LOP_BRDF_FRESNEL_GLSL