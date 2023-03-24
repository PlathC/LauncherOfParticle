#ifndef LOP_SYSTEM_MATERIAL_HPP
#define LOP_SYSTEM_MATERIAL_HPP

#include <glm/vec3.hpp>

namespace lop
{
    struct Material
    {
        glm::vec3 baseColor;
        float     roughness;
        glm::vec3 emission;

        // Plastic IOR, Reference: https://pixelandpoly.com/ior.html
        float ior = 1.460f;
    };
} // namespace lop

#endif // LOP_SYSTEM_MATERIAL_HPP
