#include "pto/Renderer/Sky.hpp"

#include <vzt/Core/Math.hpp>
#include <vzt/Utils/IOHDR.hpp>
#include <vzt/Vulkan/Command.hpp>
#include <vzt/Vulkan/Device.hpp>

#include "pto/Math/Sampling.hpp"

namespace pto
{
    Sky Sky::fromFile(vzt::View<vzt::Device> device, const vzt::Path& path)
    {
        Image<float> pixels = vzt::readEXR(path);
        return Sky(device, pixels);
    }

    Sky Sky::fromFunction(vzt::View<vzt::Device> device, const ProceduralSkyFunction& function, uint32_t width,
                          uint32_t height)
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

        return Sky(device, Image<float>{width, height, 4u, pixelsData});
    }

    Sky::Sky(const vzt::View<vzt::Device> device, const Image<float>& pixels)
        : m_image(vzt::DeviceImage::fromData(
              device, vzt::ImageUsage::TransferSrc | vzt::ImageUsage::TransferDst | vzt::ImageUsage::Sampled,
              vzt::Format::R32G32B32A32SFloat, pixels, std::log2(std::max(pixels.width, pixels.height)))),
          m_view(device, m_image, vzt::ImageAspect::Color), m_sampler(device)
    {
        std::vector<float> cdfs = getCumulativeDistributionFunctions(pixels);

        const auto queue = device->getQueue(vzt::QueueType::Graphics | vzt::QueueType::Compute);
        queue->oneShot([this, &pixels](vzt::CommandBuffer& commands) {
            uint32_t mipWidth  = pixels.width;
            uint32_t mipHeight = pixels.height;
            uint32_t mipLevels = m_image.getMipLevels();

            vzt::ImageBarrier barrier{};
            barrier.image      = m_image;
            barrier.oldLayout  = vzt::ImageLayout::Undefined;
            barrier.newLayout  = vzt::ImageLayout::TransferDstOptimal;
            barrier.baseLevel  = 0;
            barrier.levelCount = mipLevels;
            commands.barrier(vzt::PipelineStage::TopOfPipe, vzt::PipelineStage::Transfer, barrier);

            barrier.levelCount = 1;
            for (uint32_t i = 1; i < mipLevels; i++)
            {
                barrier.oldLayout = vzt::ImageLayout::TransferDstOptimal;
                barrier.newLayout = vzt::ImageLayout::TransferSrcOptimal;
                barrier.baseLevel = i - 1;
                commands.barrier(vzt::PipelineStage::Transfer, vzt::PipelineStage::Transfer, barrier);

                vzt::Blit blit{
                    {{0, 0, 0}, {mipWidth, mipHeight, 1}},
                    vzt::ImageAspect::Color,
                    i - 1,
                    {{0, 0, 0}, {std::max(mipWidth / 2, 1u), std::max(mipHeight / 2, 1u), 1}},
                    vzt::ImageAspect::Color,
                    i,
                };
                commands.blit(m_image, vzt::ImageLayout::TransferSrcOptimal, m_image,
                              vzt::ImageLayout::TransferDstOptimal, vzt::Filter::Linear, blit);

                barrier.oldLayout = vzt::ImageLayout::TransferSrcOptimal;
                barrier.newLayout = vzt::ImageLayout::ShaderReadOnlyOptimal;
                commands.barrier(vzt::PipelineStage::Transfer, vzt::PipelineStage::RaytracingShader, barrier);

                if (mipWidth > 1)
                    mipWidth /= 2;
                if (mipHeight > 1)
                    mipHeight /= 2;
            }

            barrier.baseLevel = mipLevels - 1;
            barrier.oldLayout = vzt::ImageLayout::TransferDstOptimal;
            barrier.newLayout = vzt::ImageLayout::ShaderReadOnlyOptimal;
            commands.barrier(vzt::PipelineStage::Transfer, vzt::PipelineStage::BottomOfPipe, barrier);
        });
    }

} // namespace pto
