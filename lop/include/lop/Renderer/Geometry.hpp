#ifndef LOP_RENDERER_GEOMETRY_HPP
#define LOP_RENDERER_GEOMETRY_HPP

#include <vzt/Core/Math.hpp>
#include <vzt/Vulkan/AccelerationStructure.hpp>
#include <vzt/Vulkan/Buffer.hpp>

namespace vzt
{
    struct Mesh;
}

namespace lop
{
    struct System;

    struct VertexInput
    {
        vzt::Vec3 position;
        vzt::Vec3 normal;
        vzt::Vec2 pad;
    };

    struct ObjectDescription
    {
        uint64_t vertexBuffer;
        uint64_t indexBuffer;
    };

    struct Material
    {
        vzt::Vec3 baseColor = {.6f, .55f, .55f};
        float     roughness = 0.5f;

        float metallic             = 0.f;
        float ior                  = 1.460f; // Plastic IOR, Reference: https://pixelandpoly.com/ior.html
        float specularTransmission = 0.f;
        float specularTint         = 0.f;

        vzt::Vec3 transmittance = {1.f, 1.f, 1.f};
        float     atDistance    = 1.f;

        vzt::Vec3 emission  = {};
        float     clearcoat = 0.f;

        float clearcoatGloss = 0.f;
        float pad1           = 0.f;
        float pad2           = 0.f;
        float pad3           = 0.f;
    };

    struct MeshHolder
    {
        MeshHolder(vzt::View<vzt::Device> device, const vzt::Mesh& mesh);
        ~MeshHolder() = default;

        inline const vzt::AccelerationStructure& getAccelerationStructure() const;

        vzt::Buffer vertexBuffer;
        vzt::Buffer indexBuffer;

        vzt::AccelerationStructure accelerationStructure;
    };

    struct MeshHandler
    {
      public:
        MeshHandler(vzt::View<vzt::Device> device, System& system);
        ~MeshHandler() = default;

        void update();

        inline const vzt::AccelerationStructure& getAccelerationStructure() const;
        inline const vzt::Buffer&                getDescriptions() const;
        inline const vzt::Buffer&                getMaterials() const;

      private:
        vzt::View<vzt::Device> m_device;
        System*                m_system;

        vzt::Buffer                m_objectDescriptionBuffer;
        vzt::Buffer                m_materials;
        vzt::Buffer                m_instances;
        vzt::AccelerationStructure m_accelerationStructure;
        uint32_t                   m_scratchBufferAlignment;
    };
} // namespace lop

#include "lop/Renderer/Geometry.inl"

#endif // LOP_RENDERER_GEOMETRY_HPP
