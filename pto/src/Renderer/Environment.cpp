#include "pto/Renderer/Environment.hpp"

#include <vzt/Core/Math.hpp>
#include <vzt/Utils/IOHDR.hpp>
#include <vzt/Vulkan/Command.hpp>
#include <vzt/Vulkan/Device.hpp>

#include "pto/Math/Sampling.hpp"

namespace pto
{
    Environment Environment::fromFile(vzt::View<vzt::Device> device, const vzt::Path& path)
    {
        Image<float> pixels = vzt::readEXR(path);
        return Environment(device, pixels);
    }

    Environment Environment::fromFunction(vzt::View<vzt::Device> device, const ProceduralEnvironmentFunction& function,
                                          uint32_t width, uint32_t height)
    {
        std::vector<float> pixelsData{};
        pixelsData.resize(width * height * 4u);

        for (uint32_t x = 0; x < width; x++)
        {
            for (uint32_t y = 0; y < height; y++)
            {
                const vzt::Vec2 uv = vzt::Pi * (vzt::Vec2(x, 2.f * y) / vzt::Vec2(width, height));

                // Map uv to sphere to sample the procedural function for each requested pixels
                const vzt::Vec3 direction = {
                    std::sin(uv.x) * std::cos(uv.y),
                    std::sin(uv.x) * std::sin(uv.y),
                    std::cos(uv.x),
                };

                const vzt::Vec4 color = function(glm::normalize(direction));

                pixelsData[(x * height + y) * 4u + 0u] = color.r;
                pixelsData[(x * height + y) * 4u + 1u] = color.g;
                pixelsData[(x * height + y) * 4u + 2u] = color.b;
                pixelsData[(x * height + y) * 4u + 3u] = color.a;
            }
        }

        return Environment(device, Image<float>{width, height, 4u, pixelsData});
    }

    Environment::Environment(const vzt::View<vzt::Device> device, const Image<float>& pixels)
        : image(vzt::DeviceImage::fromData(
              device, vzt::ImageUsage::TransferSrc | vzt::ImageUsage::TransferDst | vzt::ImageUsage::Sampled,
              vzt::Format::R32G32B32A32SFloat, pixels)),
          view(device, image, vzt::ImageAspect::Color), sampler(device),
          samplingSize(std::min(pixels.width, pixels.height))
    {
        assert(pixels.width % samplingSize == 0 && pixels.height % samplingSize == 0);

        std::vector<float> samplingData{};
        samplingData.resize(samplingSize * samplingSize * 4);

        const uint32_t xStepSize = pixels.width / samplingSize;
        const uint32_t yStepSize = pixels.height / samplingSize;
        for (uint32_t y = 0; y < pixels.height; y += yStepSize)
        {
            // Weighting term to avoid too much sampling on the pole
            const float sinTheta =
                std::sin(vzt::Pi * (static_cast<float>(y) + .5f) / static_cast<float>(pixels.height));
            for (uint32_t x = 0; x < pixels.width; x += xStepSize)
            {
                const float r = pixels.data[(y * pixels.width + x) * 4u + 0u];
                const float g = pixels.data[(y * pixels.width + x) * 4u + 1u];
                const float b = pixels.data[(y * pixels.width + x) * 4u + 2u];

                const uint32_t xx = x / xStepSize;
                const uint32_t yy = y / yStepSize;

                const float weightedLuminance                    = (0.2126f * r + 0.7152f * g + 0.0722f * b) * sinTheta;
                samplingData[(yy * samplingSize + xx) * 4u + 0u] = weightedLuminance;
                samplingData[(yy * samplingSize + xx) * 4u + 1u] = weightedLuminance;
                samplingData[(yy * samplingSize + xx) * 4u + 2u] = weightedLuminance;
                samplingData[(yy * samplingSize + xx) * 4u + 3u] = weightedLuminance;
            }
        }

        samplingImg = vzt::DeviceImage::fromData(
            device, vzt::ImageUsage::TransferSrc | vzt::ImageUsage::TransferDst | vzt::ImageUsage::Sampled,
            vzt::Format::R32G32B32A32SFloat, samplingSize, samplingSize,
            {reinterpret_cast<const uint8_t*>(samplingData.data()), samplingData.size() * sizeof(float)},
            static_cast<uint32_t>(std::log2(samplingSize)) + 1);

        samplingView = vzt::ImageView(device, samplingImg, vzt::ImageAspect::Color);

        const auto queue = device->getQueue(vzt::QueueType::Graphics | vzt::QueueType::Compute);
        queue->oneShot([this, &pixels](vzt::CommandBuffer& commands) {
            uint32_t mipWidth  = samplingSize;
            uint32_t mipHeight = samplingSize;
            uint32_t mipLevels = samplingImg.getMipLevels();

            vzt::ImageBarrier barrier{};
            barrier.image      = samplingImg;
            barrier.oldLayout  = vzt::ImageLayout::TransferDstOptimal;
            barrier.newLayout  = vzt::ImageLayout::TransferSrcOptimal;
            barrier.src        = vzt::Access::TransferWrite;
            barrier.dst        = vzt::Access::TransferRead;
            barrier.baseLevel  = 0;
            barrier.levelCount = 1;
            commands.barrier(vzt::PipelineStage::Transfer, vzt::PipelineStage::Transfer, barrier);

            for (uint32_t i = 1; i < mipLevels; i++)
            {
                barrier.oldLayout = vzt::ImageLayout::Undefined;
                barrier.newLayout = vzt::ImageLayout::TransferDstOptimal;
                barrier.baseLevel = i;
                barrier.src       = vzt::Access::None;
                barrier.dst       = vzt::Access::TransferWrite;
                commands.barrier(vzt::PipelineStage::Transfer, vzt::PipelineStage::Transfer, barrier);

                vzt::Blit blit{
                    {{0, 0, 0}, {std::max(mipWidth >> (i - 1), 1u), std::max(mipHeight >> (i - 1), 1u), 1}},
                    vzt::ImageAspect::Color,
                    i - 1,
                    {{0, 0, 0}, {std::max(mipWidth >> i, 1u), std::max(mipHeight >> i, 1u), 1}},
                    vzt::ImageAspect::Color,
                    i,
                };
                commands.blit(samplingImg, vzt::ImageLayout::TransferSrcOptimal, samplingImg,
                              vzt::ImageLayout::TransferDstOptimal, vzt::Filter::Linear, blit);

                barrier.src       = vzt::Access::TransferWrite;
                barrier.dst       = vzt::Access::TransferRead;
                barrier.oldLayout = vzt::ImageLayout::TransferDstOptimal;
                barrier.newLayout = vzt::ImageLayout::TransferSrcOptimal;
                commands.barrier(vzt::PipelineStage::Transfer, vzt::PipelineStage::Transfer, barrier);
            }

            barrier.levelCount = mipLevels;
            barrier.oldLayout  = vzt::ImageLayout::TransferSrcOptimal;
            barrier.newLayout  = vzt::ImageLayout::ShaderReadOnlyOptimal;
            barrier.src        = vzt::Access::TransferRead;
            barrier.dst        = vzt::Access::ShaderRead;
            barrier.baseLevel  = 0;
            barrier.levelCount = mipLevels;
            commands.barrier(vzt::PipelineStage::Transfer, vzt::PipelineStage::RaytracingShader, barrier);

            barrier.image      = image;
            barrier.oldLayout  = vzt::ImageLayout::TransferDstOptimal;
            barrier.newLayout  = vzt::ImageLayout::ShaderReadOnlyOptimal;
            barrier.src        = vzt::Access::None;
            barrier.dst        = vzt::Access::TransferWrite;
            barrier.baseLevel  = 0;
            barrier.levelCount = 1;
            commands.barrier(vzt::PipelineStage::Transfer, vzt::PipelineStage::Transfer, barrier);
        });
    }

} // namespace pto
