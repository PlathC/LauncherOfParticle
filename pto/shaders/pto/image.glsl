#ifndef SHADERS_PTO_IMAGE_GLSL
#define SHADERS_PTO_IMAGE_GLSL

ivec2 getLodSize(int lod, ivec2 size)
{
    const int powLod = int(pow(2, lod));
    const int width  = max(size.x / powLod, 1);
    const int height = max(size.y / powLod, 1);
    return ivec2(width, height);
}

#endif // SHADERS_PTO_IMAGE_GLSL