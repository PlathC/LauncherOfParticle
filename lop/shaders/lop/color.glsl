#ifndef SHADERS_LOP_COLOR_GLSL
#define SHADERS_LOP_COLOR_GLSL

float getLuminance(vec3 v) { return 0.2126 * v.r + 0.7152 * v.g + 0.0722 * v.b; }

// Reference: https://www.shadertoy.com/view/WdjSW3
vec3 tonemapACES(vec3 x) 
{
    // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

#endif // SHADERS_LOP_COLOR_GLSL