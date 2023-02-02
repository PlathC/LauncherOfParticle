#ifndef SHADERS_PTO_SAMPLING_GLSL
#define SHADERS_PTO_SAMPLING_GLSL

#include "pto/math.glsl"

// Sampling Transformations Zoo
// Peter Shirley, Samuli Laine, David Hart, Matt Pharr, Petrik Clarberg,
// Eric Haines, Matthias Raab, and David Cline
// NVIDIA
vec3 sampleCosine(vec2 u)
{
    // 16.6.1 COSINE-WEIGHTED HEMISPHERE ORIENTED TO THE Z-AXIS
    float a = sqrt(u.x);
    float b = 2. * Pi * u.y;

    return vec3(a * cos(b), a * sin(b), sqrt(1.0f - u.x));
}

#endif // SHADERS_PTO_SAMPLING_GLSL