#ifndef SHADERS_PTO_SKY_GLSL
#define SHADERS_PTO_SKY_GLSL

#include "pto/math.glsl"

vec3 faded(vec3 rd)
{
    const vec3[2] palette = vec3[](vec3(0.557, 0.725, 0.984), vec3(0.957, 0.373, 0.145));
    const float angle     = acos(dot(rd, vec3(0., 0., 1.)));

    vec3 color = pow(mix(palette[0], palette[1], abs(angle) / (Pi)), vec3(1.5f));
    if (angle < 0.3)
        color += smoothstep(0.f, 0.3f, 0.3f - angle) * 5.;

    return color;
}

vec3 sampleSky(sampler2D skySampler, vec3 rd)
{
    const float theta = acos(rd.z / length(rd));
    const float phi   = sign(rd.y) * acos(rd.x / length(rd.xy));

    return texture(skySampler, vec2(phi / (2. * Pi), theta / Pi)).rgb;
}

#endif // SHADERS_PTO_SKY_GLSL