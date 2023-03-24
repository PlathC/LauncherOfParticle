#include "lop/Renderer/Geometry.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <vzt/Data/Mesh.hpp>
#include <vzt/Vulkan/Command.hpp>
#include <vzt/Vulkan/Device.hpp>

#include "lop/System/System.hpp"
#include "lop/System/Transform.hpp"

namespace lop
{
    MeshHolder::MeshHolder(vzt::View<vzt::Device> device, const vzt::Mesh& mesh)
    {
        std::vector<VertexInput> vertexInputs;
        vertexInputs.reserve(mesh.vertices.size());
        for (std::size_t i = 0; i < mesh.vertices.size(); i++)
            vertexInputs.emplace_back(VertexInput{mesh.vertices[i], mesh.normals[i]});

        constexpr vzt::BufferUsage GeometryBufferUsages =               //
            vzt::BufferUsage::AccelerationStructureBuildInputReadOnly | //
            vzt::BufferUsage::ShaderDeviceAddress |                     //
            vzt::BufferUsage::StorageBuffer;

        vertexBuffer = vzt::Buffer::fromData<VertexInput>( //
            device, vertexInputs, vzt::BufferUsage::VertexBuffer | GeometryBufferUsages);
        indexBuffer  = vzt::Buffer::fromData<uint32_t>( //
            device, mesh.indices, vzt::BufferUsage::IndexBuffer | GeometryBufferUsages);

        vzt::GeometryAsBuilder bottomAsBuilder{vzt::AsTriangles{
            vzt::Format::R32G32B32SFloat,
            vertexBuffer,
            sizeof(VertexInput),
            vertexInputs.size(),
            indexBuffer,
        }};

        accelerationStructure = vzt::AccelerationStructure( //
            device, bottomAsBuilder, vzt::AccelerationStructureType::BottomLevel);
        {
            auto scratchBuffer = vzt::Buffer{
                device,
                accelerationStructure.getScratchBufferSize(),
                vzt::BufferUsage::StorageBuffer | vzt::BufferUsage::ShaderDeviceAddress,
            };

            // "vkCmdBuildAccelerationStructuresKHR Supported Queue Types: Compute"
            const auto queue = device->getQueue(vzt::QueueType::Compute);

            queue->oneShot([&](vzt::CommandBuffer& commands) {
                vzt::AccelerationStructureBuilder builder{
                    vzt::BuildAccelerationStructureFlag::PreferFastBuild,
                    accelerationStructure,
                    scratchBuffer,
                };
                commands.buildAs(builder);
            });
        }
    }

    MeshHandler::MeshHandler(vzt::View<vzt::Device> device, System& system) : m_device(device), m_system(&system)
    {
        update();
        m_system->registry.on_construct<MeshHolder>().connect<&MeshHandler::update>(*this);

        VkPhysicalDeviceAccelerationStructurePropertiesKHR asProperties = {};
        asProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
        asProperties.pNext = NULL;

        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayPipelineProperties = {};
        rayPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
        rayPipelineProperties.pNext = &asProperties;

        VkPhysicalDeviceProperties2 deviceProperties = {};
        deviceProperties.sType                       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProperties.pNext                       = &rayPipelineProperties;

        const vzt::PhysicalDevice hardware = m_device->getHardware();

        vkGetPhysicalDeviceProperties2(hardware.getHandle(), &deviceProperties);
        m_scratchBufferAlignment = asProperties.minAccelerationStructureScratchOffsetAlignment;
    }

    void MeshHandler::update()
    {
        const auto holders = m_system->registry.view<MeshHolder, Transform, Material>();

        std::vector<VkAccelerationStructureInstanceKHR> instancesData{};
        instancesData.reserve(holders.size_hint());

        std::vector<ObjectDescription> descriptions{};
        descriptions.reserve(holders.size_hint());

        std::vector<Material> materials{};
        materials.reserve(holders.size_hint());

        for (entt::entity entity : holders)
        {
            const auto& [holder, transform, material] = m_system->registry.get<MeshHolder, Transform, Material>(entity);

            // VkTransformMatrixKHR is a 3x4 row-major affine transformation matrix while glm is column major.
            const glm::mat4      transformMatrix = glm::transpose(transform.get());
            VkTransformMatrixKHR vkMatrix{};
            std::memcpy(reinterpret_cast<float*>(&vkMatrix), glm::value_ptr(transformMatrix), sizeof(float) * 12);

            instancesData.emplace_back( //
                VkAccelerationStructureInstanceKHR{
                    vkMatrix,
                    uint32_t(instancesData.size()),
                    0xff,
                    0,
                    0,
                    vzt::align(holder.getAccelerationStructure().getDeviceAddress(), m_scratchBufferAlignment),
                });

            descriptions.emplace_back(ObjectDescription{
                holder.vertexBuffer.getDeviceAddress(),
                holder.indexBuffer.getDeviceAddress(),
            });

            materials.emplace_back(material);
        }

        if (descriptions.empty())
        {
            // Dummy instance to still allow tracing
            instancesData.emplace_back( //
                VkAccelerationStructureInstanceKHR{
                    VkTransformMatrixKHR{
                        1.f, 0.f, 0.f, 0.f, //
                        0.f, 1.f, 0.f, 0.f, //
                        0.f, 0.f, 1.f, 0.f, //
                    },
                    uint32_t(instancesData.size()),
                    0xff,
                    0,
                    0,
                    0,
                });
            descriptions.emplace_back();
            materials.emplace_back();
        }

        m_objectDescriptionBuffer = vzt::Buffer::fromData<ObjectDescription>( //
            m_device, descriptions, vzt::BufferUsage::StorageBuffer);

        m_instances = vzt::Buffer::fromData<VkAccelerationStructureInstanceKHR>( //
            m_device, instancesData,
            vzt::BufferUsage::AccelerationStructureBuildInputReadOnly | vzt::BufferUsage::ShaderDeviceAddress);
        vzt::GeometryAsBuilder topAsBuilder{
            vzt::AsInstance{m_instances.getDeviceAddress(), uint32_t(descriptions.size())}};
        m_accelerationStructure = vzt::AccelerationStructure( //
            m_device, topAsBuilder, vzt::AccelerationStructureType::TopLevel);
        {
            const auto scratchBuffer = vzt::Buffer{
                m_device,
                m_accelerationStructure.getScratchBufferSize(),
                vzt::BufferUsage::StorageBuffer | vzt::BufferUsage::ShaderDeviceAddress,
            };

            // "vkCmdBuildAccelerationStructuresKHR Supported Queue Types: Compute"
            const auto queue = m_device->getQueue(vzt::QueueType::Compute);
            queue->oneShot([&](vzt::CommandBuffer& commands) {
                vzt::AccelerationStructureBuilder builder{
                    vzt::BuildAccelerationStructureFlag::PreferFastBuild,
                    m_accelerationStructure,
                    scratchBuffer,
                };
                commands.buildAs(builder);
            });
        }

        m_materials = vzt::Buffer::fromData<Material>(m_device, materials, vzt::BufferUsage::StorageBuffer);
    }
} // namespace lop
