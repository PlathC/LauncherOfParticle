#include "pto/Renderer/Pass/HardwarePathTracing.hpp"

#include <vzt/Vulkan/Command.hpp>
#include <vzt/Vulkan/Device.hpp>

namespace pto
{
    HardwarePathTracingPass::HardwarePathTracingPass(vzt::View<vzt::Device> device, uint32_t imageNb,
                                                     vzt::Extent2D extent, vzt::View<MeshHandler> handler,
                                                     Environment environment)
        : m_device(device), m_imageNb(imageNb), m_extent(extent), m_shaderGroup(device), m_layout(device),
          m_pipeline(device), m_descriptorPool(device, m_layout), m_handler(handler),
          m_environment(std::move(environment))
    {
        m_shaderGroup.addShader(m_compiler.compile("shaders/base.rgen", vzt::ShaderStage::RayGen));
        m_shaderGroup.addShader(m_compiler.compile("shaders/dummy.rmiss", vzt::ShaderStage::Miss));
        m_shaderGroup.addShader(m_compiler.compile("shaders/triangle.rchit", vzt::ShaderStage::ClosestHit),
                                vzt::ShaderGroupType::TrianglesHitGroup);

        m_layout.addBinding(0, vzt::DescriptorType::AccelerationStructure); // AS
        m_layout.addBinding(1, vzt::DescriptorType::StorageImage);          // Accumulation image
        m_layout.addBinding(2, vzt::DescriptorType::StorageImage);          // Render image
        m_layout.addBinding(3, vzt::DescriptorType::UniformBuffer);         // Camera
        m_layout.addBinding(4, vzt::DescriptorType::StorageBuffer);         // ObjectDescription
        m_layout.addBinding(5, vzt::DescriptorType::CombinedSampler);       // Skybox
        m_layout.addBinding(6, vzt::DescriptorType::CombinedSampler);       // Skybox sampling
        m_layout.compile();

        m_pipeline.setDescriptorLayout(m_layout);
        m_pipeline.setShaderGroup(m_shaderGroup);
        m_pipeline.compile();

        m_descriptorPool.allocate(imageNb, m_layout);

        vzt::PhysicalDevice hardware = device->getHardware();
        m_uboAlignment               = hardware.getUniformAlignment<HardwarePathTracingPass::Properties>();

        m_ubo = vzt::Buffer{
            device, m_uboAlignment * imageNb, vzt::BufferUsage::UniformBuffer, vzt::MemoryLocation::Device, true,
        };

        m_raygenShaderBindingTable = vzt::Buffer{
            device,
            m_pipeline.getShaderHandleSize() + sizeof(vzt::Vec3),
            vzt::BufferUsage::ShaderBindingTable | vzt::BufferUsage::ShaderDeviceAddress,
            vzt::MemoryLocation::Device,
            true,
        };

        m_missShaderBindingTable = vzt::Buffer{
            device,
            m_pipeline.getShaderHandleSize() + sizeof(vzt::Vec3),
            vzt::BufferUsage::ShaderBindingTable | vzt::BufferUsage::ShaderDeviceAddress,
            vzt::MemoryLocation::Device,
            true,
        };

        m_hitShaderBindingTable = vzt::Buffer{
            device,
            m_pipeline.getShaderHandleSize() + sizeof(vzt::Vec3),
            vzt::BufferUsage::ShaderBindingTable | vzt::BufferUsage::ShaderDeviceAddress,
            vzt::MemoryLocation::Device,
            true,
        };

        const vzt::CSpan<uint8_t> shaderHandleStorage = m_pipeline.getShaderHandleStorage();
        m_handleSizeAligned                           = m_pipeline.getShaderHandleSizeAligned();
        m_handleSize                                  = m_pipeline.getShaderHandleSize();

        uint8_t* rayGenData = m_raygenShaderBindingTable.map();
        std::memcpy(rayGenData, shaderHandleStorage.data, m_handleSize);
        m_raygenShaderBindingTable.unMap();

        uint8_t* missData = m_missShaderBindingTable.map();
        std::memcpy(missData, shaderHandleStorage.data + m_handleSizeAligned, m_handleSize);
        m_missShaderBindingTable.unMap();

        uint8_t* hitData = m_hitShaderBindingTable.map();
        std::memcpy(hitData, shaderHandleStorage.data + 2ul * m_handleSizeAligned, m_handleSize);
        m_hitShaderBindingTable.unMap();

        resize(extent);
    }

    void HardwarePathTracingPass::resize(vzt::Extent2D extent)
    {
        m_extent = extent;

        const auto queue = m_device->getQueue(vzt::QueueType::Graphics | vzt::QueueType::Compute);

        m_accumulationImage = vzt::DeviceImage(
            m_device, extent, vzt::ImageUsage::Storage | vzt::ImageUsage::TransferSrc, vzt::Format::R32G32B32A32SFloat);
        m_renderImage = vzt::DeviceImage(m_device, extent, vzt::ImageUsage::Storage | vzt::ImageUsage::TransferSrc,
                                         vzt::Format::R32G32B32A32SFloat);

        queue->oneShot([this](vzt::CommandBuffer& commands) {
            vzt::ImageBarrier barrier{};
            barrier.image     = m_accumulationImage;
            barrier.oldLayout = vzt::ImageLayout::Undefined;
            barrier.newLayout = vzt::ImageLayout::General;
            commands.barrier(vzt::PipelineStage::TopOfPipe, vzt::PipelineStage::BottomOfPipe, barrier);

            barrier.image     = m_renderImage;
            barrier.oldLayout = vzt::ImageLayout::Undefined;
            barrier.newLayout = vzt::ImageLayout::General;
            commands.barrier(vzt::PipelineStage::TopOfPipe, vzt::PipelineStage::BottomOfPipe, barrier);
        });

        m_accumulationImageView = vzt::ImageView{m_device, m_accumulationImage, vzt::ImageAspect::Color};
        m_renderImageView       = vzt::ImageView{m_device, m_renderImage, vzt::ImageAspect::Color};

        update();
    }

    void HardwarePathTracingPass::update()
    {
        for (uint32_t i = 0; i < m_imageNb; i++)
        {
            vzt::BufferSpan uboSpan{&m_ubo, sizeof(HardwarePathTracingPass::Properties), i * m_uboAlignment};

            const vzt::Buffer& descriptions = m_handler->getDescriptions();
            vzt::BufferCSpan   objectDescriptionUboSpan{descriptions, descriptions.size()};

            vzt::IndexedDescriptor ubos{};
            ubos[0] = vzt::DescriptorAccelerationStructure{vzt::DescriptorType::AccelerationStructure,
                                                           m_handler->getAccelerationStructure()};
            ubos[1] = vzt::DescriptorImage{
                vzt::DescriptorType::StorageImage,
                m_accumulationImageView,
                {},
                vzt::ImageLayout::General,
            };
            ubos[2] = vzt::DescriptorImage{
                vzt::DescriptorType::StorageImage,
                m_renderImageView,
                {},
                vzt::ImageLayout::General,
            };
            ubos[3] = vzt::DescriptorBuffer{vzt::DescriptorType::UniformBuffer, uboSpan};
            ubos[4] = vzt::DescriptorBuffer{vzt::DescriptorType::StorageBuffer, objectDescriptionUboSpan};
            ubos[5] = vzt::DescriptorImage{
                vzt::DescriptorType::CombinedSampler,
                m_environment.view,
                m_environment.sampler,
            };
            ubos[6] = vzt::DescriptorImage{
                vzt::DescriptorType::CombinedSampler,
                m_environment.samplingView,
                m_environment.sampler,
            };
            m_descriptorPool.update(i, ubos);
        }
    }

    void HardwarePathTracingPass::record(uint32_t imageId, vzt::CommandBuffer& commands,
                                         const vzt::View<vzt::DeviceImage> outputImage, Properties properties)
    {
        uint8_t* data = m_ubo.map();
        std::memcpy(data + imageId * m_uboAlignment, &properties, sizeof(HardwarePathTracingPass::Properties));
        m_ubo.unMap();

        vzt::BufferBarrier barrier{m_ubo, vzt::Access::TransferWrite, vzt::Access::UniformRead};
        commands.barrier(vzt::PipelineStage::Transfer, vzt::PipelineStage::RaytracingShader, barrier);

        vzt::ImageBarrier imageBarrier{};
        imageBarrier.image     = m_renderImage;
        imageBarrier.oldLayout = vzt::ImageLayout::Undefined;
        imageBarrier.newLayout = vzt::ImageLayout::General;
        commands.barrier(vzt::PipelineStage::TopOfPipe, vzt::PipelineStage::Transfer, imageBarrier);

        commands.bind(m_pipeline, m_descriptorPool[imageId]);
        commands.traceRays({m_raygenShaderBindingTable.getDeviceAddress(), m_handleSizeAligned, m_handleSizeAligned},
                           {m_missShaderBindingTable.getDeviceAddress(), m_handleSizeAligned, m_handleSizeAligned},
                           {m_hitShaderBindingTable.getDeviceAddress(), m_handleSizeAligned, m_handleSizeAligned}, {},
                           m_extent.width, m_extent.height, 1);

        imageBarrier.image     = m_renderImage;
        imageBarrier.oldLayout = vzt::ImageLayout::General;
        imageBarrier.newLayout = vzt::ImageLayout::TransferSrcOptimal;
        commands.barrier(vzt::PipelineStage::TopOfPipe, vzt::PipelineStage::Transfer, imageBarrier);

        imageBarrier.image     = outputImage;
        imageBarrier.oldLayout = vzt::ImageLayout::Undefined;
        imageBarrier.newLayout = vzt::ImageLayout::TransferDstOptimal;
        commands.barrier(vzt::PipelineStage::TopOfPipe, vzt::PipelineStage::Transfer, imageBarrier);

        commands.blit(m_renderImage, vzt::ImageLayout::TransferSrcOptimal, outputImage,
                      vzt::ImageLayout::TransferDstOptimal);

        imageBarrier.image     = outputImage;
        imageBarrier.oldLayout = vzt::ImageLayout::TransferDstOptimal;
        imageBarrier.newLayout = vzt::ImageLayout::PresentSrcKHR;
        commands.barrier(vzt::PipelineStage::TopOfPipe, vzt::PipelineStage::Transfer, imageBarrier);
    }
} // namespace pto
