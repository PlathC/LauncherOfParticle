#include "lop/Renderer/Geometry.hpp"

#include <vzt/Data/Mesh.hpp>
#include <vzt/Vulkan/Command.hpp>
#include <vzt/Vulkan/Device.hpp>

#include "lop/System/System.hpp"

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
    }

    void MeshHandler::update()
    {
        const auto holders = m_system->registry.view<MeshHolder>();

        std::vector<VkAccelerationStructureInstanceKHR> instancesData{};
        instancesData.reserve(holders.size_hint());

        std::vector<ObjectDescription> descriptions{};
        descriptions.reserve(holders.size_hint());

        holders.each([&instancesData, &descriptions](const auto& holder) {
            instancesData.emplace_back( //
                VkAccelerationStructureInstanceKHR{
                    VkTransformMatrixKHR{
                        1.f, 0.f, 0.f, 0.f, //
                        0.f, 1.f, 0.f, 0.f, //
                        0.f, 0.f, 1.f, 0.f, //
                    },
                    0,
                    0xff,
                    0,
                    0,
                    holder.getAccelerationStructure().getDeviceAddress(),
                });

            descriptions.emplace_back(ObjectDescription{
                holder.vertexBuffer.getDeviceAddress(),
                holder.indexBuffer.getDeviceAddress(),
            });
        });

        m_objectDescriptionBuffer = vzt::Buffer::fromData<ObjectDescription>( //
            m_device, descriptions, vzt::BufferUsage::StorageBuffer);

        m_instances = vzt::Buffer::fromData<VkAccelerationStructureInstanceKHR>( //
            m_device, instancesData,
            vzt::BufferUsage::AccelerationStructureBuildInputReadOnly | vzt::BufferUsage::ShaderDeviceAddress);

        vzt::GeometryAsBuilder topAsBuilder{vzt::AsInstance{m_instances.getDeviceAddress(), 1}};
        m_accelerationStructure = vzt::AccelerationStructure( //
            m_device, topAsBuilder, vzt::AccelerationStructureType::TopLevel);
        {
            auto scratchBuffer = vzt::Buffer{
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
    }
} // namespace lop
