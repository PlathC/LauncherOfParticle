#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "pto/ray.glsl"

layout(location = 0) rayPayloadInEXT HitInfo prd;

void main()
{
    // Reference: https://github.com/SaschaWillems/Vulkan/blob/master/examples
    prd.hit = false;
}
