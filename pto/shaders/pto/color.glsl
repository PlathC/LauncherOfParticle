#ifndef SHADERS_PTO_COLOR_GLSL
#define SHADERS_PTO_COLOR_GLSL

vec3 ACESFilm(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), vec3(0.f), vec3(1.f));
}

#endif // SHADERS_PTO_COLOR_GLSL