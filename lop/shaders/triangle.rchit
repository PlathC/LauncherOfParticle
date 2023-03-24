#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "lop/math.glsl"

#include "lop/color.glsl"
#include "lop/ray.glsl"
#include "lop/object.glsl"
#include "lop/vertex.glsl"

layout(location = 0) rayPayloadInEXT HitInfo prd;

// https://stackoverflow.com/a/70931803
layout(buffer_reference, std430, buffer_reference_align = 4) readonly buffer Indices
{
    uint Index;
};
layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; }; 
layout(binding = 4, set = 0) buffer Objects { Object data[]; } objects;
layout(binding = 5, set = 0) buffer Materials { Material data[]; } materials;

hitAttributeEXT vec2 attribs;

void main()
{
    Object   object   = objects.data[gl_InstanceCustomIndexEXT];
    Vertices vertices = Vertices(object.vertexBuffer);
    
    Indices indices = Indices(object.indexBuffer);    
    uvec3   ind     = uvec3(indices[gl_PrimitiveID * 3].Index, indices[gl_PrimitiveID * 3 + 1].Index, indices[gl_PrimitiveID * 3 + 2].Index);

    // Vertex of the triangle
    Vertex v0 = vertices.v[ind.x];
    Vertex v1 = vertices.v[ind.y];
    Vertex v2 = vertices.v[ind.z];
    
    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    
    // Computing the coordinates of the hit position
    const vec3 position = v0.position * barycentrics.x + v1.position * barycentrics.y + v2.position * barycentrics.z;
    prd.position        = vec3(gl_ObjectToWorldEXT * vec4(position, 1.0));  
    
    // Computing the normal at hit position
    const vec3 normal = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
    prd.normal        = normalize(vec3(normal)); 
    
    prd.material = materials.data[gl_InstanceCustomIndexEXT];
    prd.t        = gl_HitTEXT;
    prd.hit      = true;
}
