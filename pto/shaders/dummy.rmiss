#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "pto/ray.glsl"

layout(location = 0) rayPayloadInEXT HitInfo prd;

void main()
{
    prd.hit = false;
}
