#include <vzt/Data/Camera.hpp>
#include <vzt/Utils/Compiler.hpp>
#include <vzt/Utils/MeshLoader.hpp>
#include <vzt/Vulkan/AccelerationStructure.hpp>
#include <vzt/Vulkan/Buffer.hpp>
#include <vzt/Vulkan/Command.hpp>
#include <vzt/Vulkan/Pipeline/RaytracingPipeline.hpp>
#include <vzt/Vulkan/Surface.hpp>
#include <vzt/Vulkan/Swapchain.hpp>
#include <vzt/Window.hpp>

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
    vzt::Vec3 baseColor;
    float     roughness;

    vzt::Vec3 emission;
    float     transmission;
};

struct PtProperties
{
    vzt::Mat4 invView;
    vzt::Mat4 invProj;
    uint32_t  sampleId;
};

int main(int argc, char** argv)
{
    const std::string ApplicationName = "Particle throwing";

    auto compiler = vzt::Compiler();
    auto window   = vzt::Window{ApplicationName};
    auto instance = vzt::Instance{window};
    auto surface  = vzt::Surface{window, instance};

    auto device    = instance.getDevice(vzt::DeviceBuilder::rt(), surface);
    auto hardware  = device.getHardware();
    auto swapchain = vzt::Swapchain{device, surface, window.getExtent()};

    vzt::Mesh                mesh = vzt::readObj("samples/Dragon/dragon.obj");
    std::vector<VertexInput> vertexInputs;
    vertexInputs.reserve(mesh.vertices.size());
    for (std::size_t i = 0; i < mesh.vertices.size(); i++)
        vertexInputs.emplace_back(VertexInput{mesh.vertices[i], mesh.normals[i]});

    constexpr vzt::BufferUsage GeometryBufferUsages =               //
        vzt::BufferUsage::AccelerationStructureBuildInputReadOnly | //
        vzt::BufferUsage::ShaderDeviceAddress |                     //
        vzt::BufferUsage::StorageBuffer;
    const auto vertexBuffer = vzt::Buffer::fromData<VertexInput>( //
        device, vertexInputs, vzt::BufferUsage::VertexBuffer | GeometryBufferUsages);
    const auto indexBuffer  = vzt::Buffer::fromData<uint32_t>( //
        device, mesh.indices, vzt::BufferUsage::IndexBuffer | GeometryBufferUsages);

    vzt::GeometryAsBuilder bottomAsBuilder{vzt::AsTriangles{
        vzt::Format::R32G32B32SFloat,
        vertexBuffer,
        sizeof(VertexInput),
        vertexInputs.size(),
        indexBuffer,
    }};

    const auto bottomAs = vzt::AccelerationStructure( //
        device, bottomAsBuilder, vzt::AccelerationStructureType::BottomLevel);
    {
        auto scratchBuffer = vzt::Buffer{
            device,
            bottomAs.getScratchBufferSize(),
            vzt::BufferUsage::StorageBuffer | vzt::BufferUsage::ShaderDeviceAddress,
        };

        // "vkCmdBuildAccelerationStructuresKHR Supported Queue Types: Compute"
        const auto queue = device.getQueue(vzt::QueueType::Compute);
        queue->oneShot([&](vzt::CommandBuffer& commands) {
            vzt::AccelerationStructureBuilder builder{
                vzt::BuildAccelerationStructureFlag::PreferFastBuild,
                bottomAs,
                scratchBuffer,
            };
            commands.buildAs(builder);
        });
    }

    auto instancesData = std::vector{
        VkAccelerationStructureInstanceKHR{
            VkTransformMatrixKHR{
                1.f, 0.f, 0.f, 0.f, //
                0.f, 1.f, 0.f, 0.f, //
                0.f, 0.f, 1.f, 0.f, //
            },
            0,
            0xff,
            0,
            VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
            bottomAs.getDeviceAddress(),
        },
    };
    auto instances = vzt::Buffer::fromData<VkAccelerationStructureInstanceKHR>( //
        device, instancesData,
        vzt::BufferUsage::AccelerationStructureBuildInputReadOnly | vzt::BufferUsage::ShaderDeviceAddress);

    vzt::GeometryAsBuilder topAsBuilder{vzt::AsInstance{instances.getDeviceAddress(), 1}};
    const auto             topAs = vzt::AccelerationStructure( //
        device, topAsBuilder, vzt::AccelerationStructureType::TopLevel);
    {
        auto scratchBuffer = vzt::Buffer{
            device,
            topAs.getScratchBufferSize(),
            vzt::BufferUsage::StorageBuffer | vzt::BufferUsage::ShaderDeviceAddress,
        };

        // "vkCmdBuildAccelerationStructuresKHR Supported Queue Types: Compute"
        const auto queue = device.getQueue(vzt::QueueType::Compute);
        queue->oneShot([&](vzt::CommandBuffer& commands) {
            vzt::AccelerationStructureBuilder builder{
                vzt::BuildAccelerationStructureFlag::PreferFastBuild,
                topAs,
                scratchBuffer,
            };
            commands.buildAs(builder);
        });
    }

    vzt::ShaderGroup shaderGroup{device};
    shaderGroup.addShader(compiler.compile("shaders/base.rgen", vzt::ShaderStage::RayGen));
    shaderGroup.addShader(compiler.compile("shaders/dummy.rmiss", vzt::ShaderStage::Miss));
    shaderGroup.addShader(compiler.compile("shaders/dummy.rchit", vzt::ShaderStage::ClosestHit),
                          vzt::ShaderGroupType::TrianglesHitGroup);

    vzt::DescriptorLayout layout{device};
    layout.addBinding(0, vzt::DescriptorType::AccelerationStructure); // AS
    layout.addBinding(1, vzt::DescriptorType::StorageImage);          // Accumulation image
    layout.addBinding(2, vzt::DescriptorType::StorageImage);          // Render image
    layout.addBinding(3, vzt::DescriptorType::UniformBuffer);         // Camera
    layout.addBinding(4, vzt::DescriptorType::StorageBuffer);         // ObjectDescription
    layout.compile();

    vzt::RaytracingPipeline pipeline{device};
    pipeline.setDescriptorLayout(layout);
    pipeline.setShaderGroup(shaderGroup);
    pipeline.compile();

    vzt::DescriptorPool descriptorPool{device, layout};
    descriptorPool.allocate(swapchain.getImageNb(), layout);

    const std::size_t uboAlignment = hardware.getUniformAlignment<PtProperties>();
    vzt::Buffer       ubo{
        device, uboAlignment * swapchain.getImageNb(), vzt::BufferUsage::UniformBuffer, vzt::MemoryLocation::Device,
        true,
    };

    const ObjectDescription objectDescriptions{vertexBuffer.getDeviceAddress(), indexBuffer.getDeviceAddress()};
    vzt::Buffer             objectDescriptionBuffer =
        vzt::Buffer::fromData(device, vzt::OffsetCSpan{&objectDescriptions, 1}, vzt::BufferUsage::StorageBuffer);

    std::vector<vzt::Image>     accumulationImages;
    std::vector<vzt::ImageView> accumulationImageView;
    accumulationImages.reserve(swapchain.getImageNb());
    accumulationImageView.reserve(swapchain.getImageNb());

    std::vector<vzt::Image>     renderImages;
    std::vector<vzt::ImageView> renderImageView;
    renderImages.reserve(swapchain.getImageNb());
    renderImageView.reserve(swapchain.getImageNb());

    auto       queue               = device.getQueue(vzt::QueueType::Graphics | vzt::QueueType::Compute);
    const auto createRenderObjects = [&](uint32_t i) {
        vzt::Extent2D extent = window.getExtent();

        accumulationImages.emplace_back( //
            device, extent, vzt::ImageUsage::Storage | vzt::ImageUsage::TransferSrc, vzt::Format::R32G32B32A32SFloat);

        const auto& lastAccumulationImage = accumulationImages.back();
        queue->oneShot([&lastAccumulationImage](vzt::CommandBuffer& commands) {
            vzt::ImageBarrier barrier{};
            barrier.image     = lastAccumulationImage;
            barrier.oldLayout = vzt::ImageLayout::Undefined;
            barrier.newLayout = vzt::ImageLayout::General;
            commands.barrier(vzt::PipelineStage::TopOfPipe, vzt::PipelineStage::BottomOfPipe, barrier);
        });

        accumulationImageView.emplace_back(vzt::ImageView{device, lastAccumulationImage, vzt::ImageAspect::Color});

        renderImages.emplace_back( //
            device, extent, vzt::ImageUsage::Storage | vzt::ImageUsage::TransferSrc, vzt::Format::R32G32B32A32SFloat);

        const auto& lastImage = renderImages.back();
        queue->oneShot([&lastImage](vzt::CommandBuffer& commands) {
            vzt::ImageBarrier barrier{};
            barrier.image     = lastImage;
            barrier.oldLayout = vzt::ImageLayout::Undefined;
            barrier.newLayout = vzt::ImageLayout::General;
            commands.barrier(vzt::PipelineStage::TopOfPipe, vzt::PipelineStage::BottomOfPipe, barrier);
        });

        renderImageView.emplace_back(vzt::ImageView{device, lastImage, vzt::ImageAspect::Color});

        vzt::BufferSpan uboSpan{ubo, sizeof(PtProperties), i * uboAlignment};
        vzt::BufferSpan objectDescriptionUboSpan{objectDescriptionBuffer, 2 * sizeof(uint64_t)};

        vzt::IndexedDescriptor ubos{};
        ubos[0] = vzt::DescriptorAccelerationStructure{vzt::DescriptorType::AccelerationStructure, topAs};
        ubos[1] = vzt::DescriptorImage{
            vzt::DescriptorType::StorageImage,
            accumulationImageView.back(),
            {},
            vzt::ImageLayout::General,
        };
        ubos[2] = vzt::DescriptorImage{
            vzt::DescriptorType::StorageImage,
            renderImageView.back(),
            {},
            vzt::ImageLayout::General,
        };
        ubos[3] = vzt::DescriptorBuffer{vzt::DescriptorType::UniformBuffer, uboSpan};
        ubos[4] = vzt::DescriptorBuffer{vzt::DescriptorType::StorageBuffer, objectDescriptionUboSpan};
        descriptorPool.update(i, ubos);
    };

    for (uint32_t i = 0; i < swapchain.getImageNb(); i++)
        createRenderObjects(i);

    vzt::Buffer raygenShaderBindingTable{
        device,
        pipeline.getShaderHandleSize() + sizeof(vzt::Vec3),
        vzt::BufferUsage::ShaderBindingTable | vzt::BufferUsage::ShaderDeviceAddress,
        vzt::MemoryLocation::Device,
        true,
    };

    vzt::Buffer missShaderBindingTable{
        device,
        pipeline.getShaderHandleSize() + sizeof(vzt::Vec3),
        vzt::BufferUsage::ShaderBindingTable | vzt::BufferUsage::ShaderDeviceAddress,
        vzt::MemoryLocation::Device,
        true,
    };

    vzt::Buffer hitShaderBindingTable{
        device,
        pipeline.getShaderHandleSize() + sizeof(vzt::Vec3),
        vzt::BufferUsage::ShaderBindingTable | vzt::BufferUsage::ShaderDeviceAddress,
        vzt::MemoryLocation::Device,
        true,
    };

    const vzt::CSpan<uint8_t> shaderHandleStorage = pipeline.getShaderHandleStorage();
    const uint32_t            handleSizeAligned   = pipeline.getShaderHandleSizeAligned();
    const uint32_t            handleSize          = pipeline.getShaderHandleSize();

    uint8_t* rayGenData = raygenShaderBindingTable.map();
    std::memcpy(rayGenData, shaderHandleStorage.data, handleSize);
    raygenShaderBindingTable.unMap();

    uint8_t* missData = missShaderBindingTable.map();
    std::memcpy(missData, shaderHandleStorage.data + handleSizeAligned, handleSize);
    missShaderBindingTable.unMap();

    uint8_t* hitData = hitShaderBindingTable.map();
    std::memcpy(hitData, shaderHandleStorage.data + 2ul * handleSizeAligned, handleSize);
    hitShaderBindingTable.unMap();

    // Compute AABB to place camera in front of the model
    vzt::Vec3 minimum{std::numeric_limits<float>::max()};
    vzt::Vec3 maximum{std::numeric_limits<float>::lowest()};
    for (vzt::Vec3& vertex : mesh.vertices)
    {
        minimum = glm::min(minimum, vertex);
        maximum = glm::max(maximum, vertex);
    }

    vzt::Camera camera{};
    camera.front = vzt::Vec3(0.f, 0.f, 1.f);
    camera.up    = vzt::Vec3(0.f, 1.f, 0.f);
    camera.right = vzt::Vec3(1.f, 0.f, 0.f);

    const vzt::Vec3 target         = (minimum + maximum) * .5f;
    const float     bbRadius       = glm::compMax(glm::abs(maximum - target));
    const float     distance       = bbRadius / std::tan(camera.fov * .5f);
    const vzt::Vec3 cameraPosition = target - camera.front * 1.15f * distance;

    auto commandPool = vzt::CommandPool(device, queue, swapchain.getImageNb());

    std::size_t  i = 0;
    PtProperties properties{};
    while (window.update())
    {
        const auto& inputs = window.getInputs();
        if (inputs.windowResized)
            swapchain.setExtent(inputs.windowSize);

        auto submission = swapchain.getSubmission();
        if (!submission)
            continue;

        vzt::Extent2D extent = window.getExtent();

        // Per frame update
        vzt::Quat orientation = {1.f, 0.f, 0.f, 0.f};
        if (inputs.mouseLeftPressed || i < swapchain.getImageNb())
        {
            float           t               = inputs.mousePosition.x * vzt::Tau / static_cast<float>(window.getWith());
            const vzt::Quat rotation        = glm::angleAxis(t, camera.up);
            const vzt::Vec3 currentPosition = rotation * (cameraPosition - target) + target;

            vzt::Vec3       direction  = glm::normalize(target - currentPosition);
            const vzt::Vec3 reference  = camera.front;
            const float     projection = glm::dot(reference, direction);
            if (std::abs(projection) < 1.f - 1e-6f) // If direction and reference are not the same
                orientation = glm::rotation(reference, direction);
            else if (projection < 0.f) // If direction and reference are opposite
                orientation = glm::angleAxis(-vzt::Pi, camera.up);

            vzt::Mat4 view = camera.getViewMatrix(currentPosition, orientation);
            properties     = {glm::inverse(view), glm::inverse(camera.getProjectionMatrix()), 0};
            i++;
        }

        const auto&        image    = swapchain.getImage(submission->imageId);
        vzt::CommandBuffer commands = commandPool[submission->imageId];
        {
            commands.begin();

            uint8_t* data = ubo.map();
            std::memcpy(data + submission->imageId * uboAlignment, &properties, sizeof(PtProperties));
            ubo.unMap();

            vzt::BufferBarrier barrier{ubo, vzt::Access::TransferWrite, vzt::Access::UniformRead};
            commands.barrier(vzt::PipelineStage::Transfer, vzt::PipelineStage::RaytracingShader, barrier);

            vzt::ImageBarrier imageBarrier{};
            imageBarrier.image     = renderImages[submission->imageId];
            imageBarrier.oldLayout = vzt::ImageLayout::Undefined;
            imageBarrier.newLayout = vzt::ImageLayout::General;
            commands.barrier(vzt::PipelineStage::TopOfPipe, vzt::PipelineStage::Transfer, imageBarrier);

            commands.bind(pipeline, descriptorPool[submission->imageId]);
            commands.traceRays({raygenShaderBindingTable.getDeviceAddress(), handleSizeAligned, handleSizeAligned},
                               {missShaderBindingTable.getDeviceAddress(), handleSizeAligned,
                                vzt::align(handleSizeAligned + sizeof(vzt::Vec3),
                                           std::max(handleSizeAligned, handleSizeAligned - handleSize))},
                               {hitShaderBindingTable.getDeviceAddress(), handleSizeAligned,
                                vzt::align(handleSizeAligned + sizeof(vzt::Vec3),
                                           std::max(handleSizeAligned, handleSizeAligned - handleSize))},
                               {}, extent.width, extent.height, 1);

            imageBarrier.image     = renderImages[submission->imageId];
            imageBarrier.oldLayout = vzt::ImageLayout::General;
            imageBarrier.newLayout = vzt::ImageLayout::TransferSrcOptimal;
            commands.barrier(vzt::PipelineStage::TopOfPipe, vzt::PipelineStage::Transfer, imageBarrier);

            imageBarrier.image     = image;
            imageBarrier.oldLayout = vzt::ImageLayout::Undefined;
            imageBarrier.newLayout = vzt::ImageLayout::TransferDstOptimal;
            commands.barrier(vzt::PipelineStage::TopOfPipe, vzt::PipelineStage::Transfer, imageBarrier);

            commands.blit(renderImages[submission->imageId], vzt::ImageLayout::TransferSrcOptimal, image,
                          vzt::ImageLayout::TransferDstOptimal);

            imageBarrier.image     = image;
            imageBarrier.oldLayout = vzt::ImageLayout::TransferDstOptimal;
            imageBarrier.newLayout = vzt::ImageLayout::PresentSrcKHR;
            commands.barrier(vzt::PipelineStage::TopOfPipe, vzt::PipelineStage::Transfer, imageBarrier);

            commands.end();
        }

        properties.sampleId++;

        queue->submit(commands, *submission);
        if (!swapchain.present())
        {
            // Wait all commands execution
            device.wait();

            // Apply screen size update
            vzt::Extent2D extent = window.getExtent();
            camera.aspectRatio   = static_cast<float>(extent.width) / static_cast<float>(extent.height);

            accumulationImages.clear();
            accumulationImageView.clear();
            renderImages.clear();
            renderImageView.clear();
            for (uint32_t i = 0; i < swapchain.getImageNb(); i++)
                createRenderObjects(i);
        }
    }

    return EXIT_SUCCESS;
}
