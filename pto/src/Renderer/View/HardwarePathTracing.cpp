#include "pto/Renderer/View/HardwarePathTracing.hpp"

#include <vzt/Vulkan/Command.hpp>
#include <vzt/Vulkan/Device.hpp>

namespace pto
{
    HardwarePathTracingView::HardwarePathTracingView(vzt::View<vzt::Device> device, uint32_t imageNb,
                                                     vzt::Extent2D extent, System& system,
                                                     vzt::View<GeometryHandler> handler)
        : m_device(device), m_imageNb(imageNb), m_extent(extent), m_shaderGroup(device), m_layout(device),
          m_pipeline(device), m_descriptorPool(device, m_layout), m_system(&system), m_handler(handler)
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
        m_layout.compile();

        m_pipeline.setDescriptorLayout(m_layout);
        m_pipeline.setShaderGroup(m_shaderGroup);
        m_pipeline.compile();

        m_descriptorPool.allocate(imageNb, m_layout);

        vzt::PhysicalDevice hardware = device->getHardware();
        m_uboAlignment               = hardware.getUniformAlignment<HardwarePathTracingView::Properties>();

        m_ubo = vzt::Buffer{
            device, m_uboAlignment * imageNb, vzt::BufferUsage::UniformBuffer, vzt::MemoryLocation::Device, true,
        };

        m_accumulationImages.reserve(imageNb);
        m_accumulationImageView.reserve(imageNb);

        m_renderImages.reserve(imageNb);
        m_renderImageView.reserve(imageNb);

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

    void HardwarePathTracingView::resize(vzt::Extent2D extent)
    {
        m_extent = extent;

        m_accumulationImages.clear();
        m_accumulationImageView.clear();
        m_renderImages.clear();
        m_renderImageView.clear();

        for (uint32_t i = 0; i < m_imageNb; i++)
        {
            const auto queue = m_device->getQueue(vzt::QueueType::Graphics | vzt::QueueType::Compute);

            m_accumulationImages.emplace_back( //
                m_device, extent, vzt::ImageUsage::Storage | vzt::ImageUsage::TransferSrc,
                vzt::Format::R32G32B32A32SFloat);
            const auto& lastAccumulationImage = m_accumulationImages.back();
            queue->oneShot([&lastAccumulationImage](vzt::CommandBuffer& commands) {
                vzt::ImageBarrier barrier{};
                barrier.image     = lastAccumulationImage;
                barrier.oldLayout = vzt::ImageLayout::Undefined;
                barrier.newLayout = vzt::ImageLayout::General;
                commands.barrier(vzt::PipelineStage::TopOfPipe, vzt::PipelineStage::BottomOfPipe, barrier);
            });
            m_accumulationImageView.emplace_back(
                vzt::ImageView{m_device, lastAccumulationImage, vzt::ImageAspect::Color});

            m_renderImages.emplace_back( //
                m_device, extent, vzt::ImageUsage::Storage | vzt::ImageUsage::TransferSrc,
                vzt::Format::R32G32B32A32SFloat);
            const auto& lastImage = m_renderImages.back();
            queue->oneShot([&lastImage](vzt::CommandBuffer& commands) {
                vzt::ImageBarrier barrier{};
                barrier.image     = lastImage;
                barrier.oldLayout = vzt::ImageLayout::Undefined;
                barrier.newLayout = vzt::ImageLayout::General;
                commands.barrier(vzt::PipelineStage::TopOfPipe, vzt::PipelineStage::BottomOfPipe, barrier);
            });
            m_renderImageView.emplace_back(vzt::ImageView{m_device, lastImage, vzt::ImageAspect::Color});
        }

        update();
    }

    void HardwarePathTracingView::update()
    {
        auto holders = m_system->registry.view<GeometryHolder>();

        std::vector<ObjectDescription> descriptions{};
        holders.each([&descriptions](const auto& holder) {
            descriptions.emplace_back(ObjectDescription{
                holder.vertexBuffer.getDeviceAddress(),
                holder.indexBuffer.getDeviceAddress(),
            });
        });

        m_objectDescriptionBuffer = vzt::Buffer::fromData<ObjectDescription>( //
            m_device, descriptions, vzt::BufferUsage::StorageBuffer);

        for (uint32_t i = 0; i < m_imageNb; i++)
        {
            vzt::BufferSpan uboSpan{m_ubo, sizeof(HardwarePathTracingView::Properties), i * m_uboAlignment};
            vzt::BufferSpan objectDescriptionUboSpan{m_objectDescriptionBuffer, m_objectDescriptionBuffer.size()};

            vzt::IndexedDescriptor ubos{};
            ubos[0] = vzt::DescriptorAccelerationStructure{vzt::DescriptorType::AccelerationStructure,
                                                           m_handler->getAccelerationStructure()};
            ubos[1] = vzt::DescriptorImage{
                vzt::DescriptorType::StorageImage,
                m_accumulationImageView[i],
                {},
                vzt::ImageLayout::General,
            };
            ubos[2] = vzt::DescriptorImage{
                vzt::DescriptorType::StorageImage,
                m_renderImageView[i],
                {},
                vzt::ImageLayout::General,
            };
            ubos[3] = vzt::DescriptorBuffer{vzt::DescriptorType::UniformBuffer, uboSpan};
            ubos[4] = vzt::DescriptorBuffer{vzt::DescriptorType::StorageBuffer, objectDescriptionUboSpan};
            m_descriptorPool.update(i, ubos);
        }
    }

    void HardwarePathTracingView::record(uint32_t imageId, vzt::CommandBuffer& commands,
                                         const vzt::View<vzt::Image> outputImage, const Properties& properties)
    {
        uint8_t* data = m_ubo.map();
        std::memcpy(data + imageId * m_uboAlignment, &properties, sizeof(HardwarePathTracingView::Properties));
        m_ubo.unMap();

        vzt::BufferBarrier barrier{m_ubo, vzt::Access::TransferWrite, vzt::Access::UniformRead};
        commands.barrier(vzt::PipelineStage::Transfer, vzt::PipelineStage::RaytracingShader, barrier);

        vzt::ImageBarrier imageBarrier{};
        imageBarrier.image     = m_renderImages[imageId];
        imageBarrier.oldLayout = vzt::ImageLayout::Undefined;
        imageBarrier.newLayout = vzt::ImageLayout::General;
        commands.barrier(vzt::PipelineStage::TopOfPipe, vzt::PipelineStage::Transfer, imageBarrier);

        commands.bind(m_pipeline, m_descriptorPool[imageId]);
        commands.traceRays({m_raygenShaderBindingTable.getDeviceAddress(), m_handleSizeAligned, m_handleSizeAligned},
                           {m_missShaderBindingTable.getDeviceAddress(), m_handleSizeAligned, m_handleSizeAligned},
                           {m_hitShaderBindingTable.getDeviceAddress(), m_handleSizeAligned, m_handleSizeAligned}, {},
                           m_extent.width, m_extent.height, 1);

        imageBarrier.image     = m_renderImages[imageId];
        imageBarrier.oldLayout = vzt::ImageLayout::General;
        imageBarrier.newLayout = vzt::ImageLayout::TransferSrcOptimal;
        commands.barrier(vzt::PipelineStage::TopOfPipe, vzt::PipelineStage::Transfer, imageBarrier);

        imageBarrier.image     = outputImage;
        imageBarrier.oldLayout = vzt::ImageLayout::Undefined;
        imageBarrier.newLayout = vzt::ImageLayout::TransferDstOptimal;
        commands.barrier(vzt::PipelineStage::TopOfPipe, vzt::PipelineStage::Transfer, imageBarrier);

        commands.blit(m_renderImages[imageId], vzt::ImageLayout::TransferSrcOptimal, outputImage,
                      vzt::ImageLayout::TransferDstOptimal);

        imageBarrier.image     = outputImage;
        imageBarrier.oldLayout = vzt::ImageLayout::TransferDstOptimal;
        imageBarrier.newLayout = vzt::ImageLayout::PresentSrcKHR;
        commands.barrier(vzt::PipelineStage::TopOfPipe, vzt::PipelineStage::Transfer, imageBarrier);
    }

} // namespace pto