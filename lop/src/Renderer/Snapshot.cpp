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

        const vzt::SubresourceLayout subresourceLayout = targetImage.getSubresourceLayout(vzt::ImageAspect::Color);
        const uint8_t*               mappedData        = targetImage.map<uint8_t>();
        mappedData += subresourceLayout.offset;

        std::vector<uint8_t> cpuData = std::vector<uint8_t>(extent.width * extent.height * 4);
        for (uint32_t y = 0; y < extent.height; y++)
        {
            const uint32_t dstStride = y * extent.width * sizeof(uint32_t);
            const uint32_t srcStride = y * subresourceLayout.rowPitch;
            std::memcpy(cpuData.data() + dstStride, mappedData + srcStride, extent.width * sizeof(uint32_t));

            for (uint32_t x = 0; x < extent.width; x++)
            {
                const uint32_t pixelId = (y * extent.width + x) * 4;
                std::swap(cpuData[pixelId + 0], cpuData[pixelId + 2]);
            }
        }
        targetImage.unmap();

        const std::string str = outputPath.string();
        if (!stbi_write_png(str.c_str(), extent.width, extent.height, 4, cpuData.data(), 0))
            vzt::logger::error("Failed to save snapshot at {}", str);
    }
} // namespace lop
