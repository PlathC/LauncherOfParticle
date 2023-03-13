#include "pto/Renderer/View/Snapshot.hpp"

#include <vzt/Core/Logger.hpp>
#include <vzt/Vulkan/Device.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace pto
{
    void snapshot(vzt::View<vzt::Device> device, vzt::View<vzt::DeviceImage> outputImage, vzt::Extent2D extent,
                  const vzt::Path& outputPath)
    {
        vzt::ImageBuilder imageBuilder{};
        imageBuilder.size     = extent;
        imageBuilder.usage    = vzt::ImageUsage::TransferDst;
        imageBuilder.format   = vzt::Format::R8G8B8A8UNorm;
        imageBuilder.tiling   = vzt::ImageTiling::Linear;
        imageBuilder.mappable = true;
        auto targetImage      = vzt::DeviceImage(device, imageBuilder);

        const auto queue = device->getQueue(vzt::QueueType::Graphics | vzt::QueueType::Compute);
        queue->oneShot([&](vzt::CommandBuffer& commands) {
            vzt::ImageBarrier transition{};
            transition.image     = outputImage;
            transition.oldLayout = vzt::ImageLayout::PresentSrcKHR;
            transition.newLayout = vzt::ImageLayout::TransferSrcOptimal;
            commands.barrier(vzt::PipelineStage::Transfer, vzt::PipelineStage::Transfer, transition);

            transition.image     = targetImage;
            transition.oldLayout = vzt::ImageLayout::Undefined;
            transition.newLayout = vzt::ImageLayout::TransferDstOptimal;
            commands.barrier(vzt::PipelineStage::Transfer, vzt::PipelineStage::Transfer, transition);

            commands.copy(outputImage, targetImage, extent.width, extent.height);

            transition.image     = outputImage;
            transition.oldLayout = vzt::ImageLayout::TransferSrcOptimal;
            transition.newLayout = vzt::ImageLayout::PresentSrcKHR;
            commands.barrier(vzt::PipelineStage::Transfer, vzt::PipelineStage::Transfer, transition);

            transition.image     = targetImage;
            transition.oldLayout = vzt::ImageLayout::TransferDstOptimal;
            transition.newLayout = vzt::ImageLayout::General;
            commands.barrier(vzt::PipelineStage::Transfer, vzt::PipelineStage::Transfer, transition);
        });

        const uint32_t* mappedData = targetImage.map<uint32_t>();

        std::vector<uint8_t> image{};
        image.resize(extent.width * extent.height * 4);
        for (uint32_t y = 0; y < extent.height; y++)
        {
            for (uint32_t x = 0; x < extent.width; x++)
            {
                image[(y * extent.width + x) * 4 + 0] = static_cast<uint8_t>(*(mappedData) >> 16 & 0xff);
                image[(y * extent.width + x) * 4 + 1] = static_cast<uint8_t>(*(mappedData) >> 8 & 0xff);
                image[(y * extent.width + x) * 4 + 2] = static_cast<uint8_t>(*(mappedData) >> 0 & 0xff);
                image[(y * extent.width + x) * 4 + 3] = static_cast<uint8_t>(*(mappedData) >> 24 & 0xff);

                mappedData++;
            }
        }

        targetImage.unmap();

        stbi_write_png_compression_level = 0;

        const std::string str = outputPath.string();
        if (!stbi_write_png(str.c_str(), extent.width, extent.height, 4, image.data(), 0))
            vzt::logger::error("Failed to save snapshot at {}", str);
    }
} // namespace pto
