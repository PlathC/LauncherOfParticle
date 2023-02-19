#ifndef SHADERS_PTO_SKY_GLSL
#define SHADERS_PTO_SKY_GLSL

#include "pto/color.glsl"
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

ivec2 getLodSize(int lod, ivec2 size)
{
    const int powLod = int(pow(2, lod));
    const int width  = max(size.x / powLod, 1);
    const int height = max(size.y / powLod, 1);
    return ivec2(width, height);
}

vec3 loadSkybox(sampler2D skySampler, vec2 uv, int lod)
{
    const ivec2 size = ivec2(4096, 2048);
    return texelFetch(skySampler, ivec2(uv * vec2(getLodSize(lod, size))), int(lod)).rgb;
}

vec3 sampleSky(sampler2D skySampler, vec3 rd)
{
    const float theta = acos(rd.z / length(rd));
    const float phi   = sign(rd.y) * acos(rd.x / length(rd.xy));

    const vec2 uv = vec2(phi / (2. * Pi) + .5, theta / Pi);
    return loadSkybox(skySampler, uv, 0);
}

// Sampling Transformations Zoo
// Peter Shirley, Samuli Laine, David Hart, Matt Pharr, Petrik Clarberg,
// Eric Haines, Matthias Raab, and David Cline
// NVIDIA
ivec2 sampleSkyLight(sampler2D skySampler, vec2 u, out float pdf)
{
    const ivec2 size = ivec2(4096, 2048);

    int x = 0, y = 0;
    for (int mip = 10; mip >= 0; --mip)
    {
        x <<= 1;
        y <<= 1;
        float left =  getLuminance(texelFetch(skySampler, ivec2(x, y),         int(mip)).rgb) +
                      getLuminance(texelFetch(skySampler, ivec2(x, y + 1),     int(mip)).rgb);
        float right = getLuminance(texelFetch(skySampler, ivec2(x + 1, y),     int(mip)).rgb) +
                      getLuminance(texelFetch(skySampler, ivec2(x + 1, y + 1), int(mip)).rgb);
        float probLeft = left / (left + right);
        if (u.x < probLeft)
        {
            u.x /= probLeft;
            float probLower = getLuminance(texelFetch(skySampler, ivec2(x, y), int(mip)).rgb) / left;
            if (u.y < probLower)
            {
                u.y /= probLower;
            }
            else
            {
                y++;
                u.y = (u.y - probLower) / (1. - probLower);
            }
        }
        else
        {
            x++;
            u.x             = (u.x - probLeft) / (1. - probLeft);
            float probLower = getLuminance(texelFetch(skySampler, ivec2(x, y), int(mip)).rgb) / right;
            if (u.y < probLower)
            {
                u.y /= probLower;
            }
            else
            {
                y++;
                u.y = (u.y - probLower) / (1. - probLower);
            }
        }
    }

    pdf = getLuminance(texelFetch(skySampler, ivec2(x, y), 0).rgb) /
          getLuminance(texelFetch(skySampler, ivec2(0, 0), 12).rgb);

    return ivec2(x, y);
}

#endif // SHADERS_PTO_SKY_GLSL