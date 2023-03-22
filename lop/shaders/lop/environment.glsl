#ifndef SHADERS_LOP_ENVIRONMENT_GLSL
#define SHADERS_LOP_ENVIRONMENT_GLSL

#include "lop/color.glsl"
#include "lop/math.glsl"

vec3 getEnvironment(sampler2D skySampler, vec3 v)
{
    const float theta = acos(clamp(v.z, -1., 1.));

    float phi = sign(v.y) * acos(v.x / length(v.xy));
    phi       = phi < 0. ? phi + 2. * Pi : phi;

    const vec2 uv = vec2(phi / (2. * Pi), theta / Pi);
    return texture(skySampler, uv).rgb;
}

#endif // SHADERS_LOP_ENVIRONMENT_GLSL