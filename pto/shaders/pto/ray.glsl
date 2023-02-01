struct Material
{
    vec3  baseColor;
    float roughness;

    vec3  emission;
    float transmission;

    float ior;
    float metalness;

    float pad1;
    float pad2;
};

struct HitInfo {
    Material material;
    
    vec3  position;
    float t;
    vec3  normal;
    bool  hit;
};