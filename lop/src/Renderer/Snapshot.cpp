#include "lop/Renderer/Snapshot.hpp"

#include <vzt/Core/Logger.hpp>
#include <vzt/Vulkan/Device.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace lop
{
    void snapshot(vzt::View<vzt::Device> device, vzt::View<vzt::DeviceImage> outputImage, const vzt::Path& outputPath)
    {
        const vzt::Extent3D extent = outputImage->getSize();

        vzt::ImageBuilder imageBuilder{};
        imageBuilder.size     = extent;
        imageBuilder.usage    = vzt::ImageUsage::TransferDst;
        imageBuilder.format   = vzt::Format::R8G8B8A8SRGB;
        imageBuilder.tiling   = vzt::ImageTiling::Linear;
        imageBuilder.mappable = true;
        auto targetImage      = vzt::DeviceImage(device, imageBuilder);

        const auto queue = device->getQueue(vzt::QueueType::Graphics | vzt::QueueType::Compute);
        queue->oneShot([&](vzt::CommandBuffer& commands) {
            vzt::ImageBarrier transition{};
            transition.image     = outputImage;
            transition.oldLayout = vzt::ImageLayout::TransferSrcOptimal;
            transition.newLayout = vzt::ImageLayout::TransferSrcOptimal;
            commands.barrier(vzt::PipelineStage::Transfer, vzt::PipelineStage::Transfer, transition);

            transition.image     = targetImage;
            transition.oldLayout = vzt::ImageLayout::Undefined;
            transition.newLayout = vzt::ImageLayout::TransferDstOptimal;
            commands.barrier(vzt::PipelineStage::Transfer, vzt::PipelineStage::Transfer, transition);

            commands.copy(outputImage, targetImage, extent.width, extent.height);

            transition.image     = targetImage;
            transition.oldLayout = vzt::ImageLayout::TransferDstOptimal;
            transition.newLayout = vzt::ImageLayout::General;
            commands.barrier(vzt::PipelineStage::Transfer, vzt::PipelineStage::Transfer, transition);
        });

        const uint32_t*      mappedData = targetImage.map<uint32_t>();
        std::vector<uint8_t> cpuData    = std::vector<uint8_t>(extent.width * extent.height * 4);
        std::memcpy(cpuData.data(), mappedData, cpuData.size());
        targetImage.unmap();

        for (uint32_t y = 0; y < extent.height; y++)
        {
            for (uint32_t x = 0; x < extent.width; x++)
            {
                const uint32_t pixelId = (y * extent.width + x) * 4;
                std::swap(cpuData[pixelId + 0], cpuData[pixelId + 2]);
            }
        }

        const std::string str = outputPath.string();
        if (!stbi_write_png(str.c_str(), extent.width, extent.height, 4, cpuData.data(), 0))
            vzt::logger::error("Failed to save snapshot at {}", str);
    }
} // namespace lop
