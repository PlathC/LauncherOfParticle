#ifndef SHADERS_LOP_MATH_GLSL
#define SHADERS_LOP_MATH_GLSL

const float Pi = 3.1415;

vec4 quaternion(float angle, vec3 axis)
{
    float halfAngle = angle / 2.;
    return vec4(axis.x * sin(halfAngle), axis.y * sin(halfAngle), axis.z * sin(halfAngle), cos(halfAngle));
}

// Based on GLM implementation
vec3 multiply(vec4 quat, vec3 p)
{
    vec3 quatVector = quat.xyz;
    vec3 uv         = cross(quatVector, p);
    vec3 uuv        = cross(quatVector, uv);

    return p + ((uv * quat.w) + uuv) * 2.;
}

vec4 conjugate(vec4 quat) { return vec4(-quat.x, -quat.y, -quat.z, quat.w); }

// Both n and ref must be normalized
vec4 toLocal(vec3 n, vec3 ref)
{
    if (dot(n, ref) < -1.f + 1e-4f)
        return vec4(1.f, 0.f, 0.f, 0.f);

    float angle = 1.f + dot(n, ref); // sqrt(length2(n) * length2(ref)) + dot( input, up );
    vec3  axis  = cross(n, ref);
    return normalize(vec4(axis, angle));
}

vec4 toLocalZ(vec3 n) { return toLocal(n, vec3(0., 0., 1.)); }

#endif // SHADERS_LOP_MATH_GLSL