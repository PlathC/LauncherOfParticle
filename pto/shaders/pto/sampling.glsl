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

// Sampling Transformations Zoo
// Peter Shirley, Samuli Laine, David Hart, Matt Pharr, Petrik Clarberg,
// Eric Haines, Matthias Raab, and David Cline
// NVIDIA
vec2 sample2D(sampler2D skySampler, uint size, vec2 u, out float pdf)
{
    const int maxMipMap = int(log2(size));

    int x = 0, y = 0;
    for (int mip = maxMipMap - 1; mip >= 0; --mip)
    {
        x <<= 1;
        y <<= 1;
        float left = texelFetch(skySampler, ivec2(x, y), mip).r + //
                     texelFetch(skySampler, ivec2(x, y + 1), int(mip)).r;
        float right = texelFetch(skySampler, ivec2(x + 1, y), mip).r + //
                      texelFetch(skySampler, ivec2(x + 1, y + 1), mip).r;
        float probLeft = left / (left + right);
        if (u.x < probLeft)
        {
            u.x /= probLeft;
            float probLower = texelFetch(skySampler, ivec2(x, y), mip).r / left;
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
            float probLower = texelFetch(skySampler, ivec2(x, y), mip).r / right;
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

    pdf = texelFetch(skySampler, ivec2(x, y), 0).r / texelFetch(skySampler, ivec2(0, 0), maxMipMap).r;

    return vec2(x, y) / float(size);
}

vec3 sampleEnvironment(sampler2D skySampler, uint size, vec2 u, out float pdf)
{
    vec2 uv = sample2D(skySampler, size, u, pdf);

    // We want X to be mapped from 0 to 2Pi since that's where the image is the largest
    const float theta = uv.y * Pi;
    const float phi   = uv.x * 2. * Pi;

    const float cosTheta = cos(theta);
    const float sinTheta = sin(theta);
    const float cosPhi   = cos(phi);
    const float sinPhi   = sin(phi);

    pdf /= max(1e-4, 2. * Pi * Pi   // Density in terms of spherical coordinates
                         * sinTheta // Mapping jacobian
    );

    return normalize(vec3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta));
}

#endif // SHADERS_PTO_SAMPLING_GLSL