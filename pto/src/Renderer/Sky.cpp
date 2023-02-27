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
        assert(pixels.width % 2 == 0 && pixels.height % 2 == 0 && "Skybox image must be of a size of power of 2.");
        return Sky(device, pixels);
    }

    Sky Sky::fromFunction(vzt::View<vzt::Device> device, const ProceduralSkyFunction& function, uint32_t width,
                          uint32_t height)
    {
        assert(width % 2 == 0 && height % 2 == 0 && "Skybox image must be of a size of power of 2.");

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
              vzt::Format::R32G32B32A32SFloat, pixels)),
          m_view(device, m_image, vzt::ImageAspect::Color), m_sampler(device)
    {
        m_samplingImgSize = std::min(pixels.width, pixels.height);
        assert(m_samplingImgSize % pixels.width == 0 && m_samplingImgSize % pixels.height == 0);

        std::vector<float> samplingData{};
        samplingData.resize(m_samplingImgSize * m_samplingImgSize * 4);

        const uint32_t xStepSize = pixels.width / m_samplingImgSize;
        const uint32_t yStepSize = pixels.height / m_samplingImgSize;
        for (uint32_t y = 0; y < pixels.height; y += yStepSize)
        {
            // Weighting term to avoid to much sampling on the pole
            const float sinTheta =
                std::sin(vzt::Pi * (static_cast<float>(y) + .5f) / static_cast<float>(pixels.height));
            for (uint32_t x = 0; x < pixels.width; x += xStepSize)
            {
                const float r = pixels.data[(y * pixels.width + x) * 4u + 0u];
                const float g = pixels.data[(y * pixels.width + x) * 4u + 1u];
                const float b = pixels.data[(y * pixels.width + x) * 4u + 2u];

                const uint32_t xx = x / xStepSize;
                const uint32_t yy = y / yStepSize;

                const float weightedLuminance = (0.2126f * r + 0.7152f * g + 0.0722f * b) * sinTheta;
                samplingData[(yy * m_samplingImgSize + xx) * 4u + 0u] = weightedLuminance;
                samplingData[(yy * m_samplingImgSize + xx) * 4u + 1u] = weightedLuminance;
                samplingData[(yy * m_samplingImgSize + xx) * 4u + 2u] = weightedLuminance;
                samplingData[(yy * m_samplingImgSize + xx) * 4u + 3u] = weightedLuminance;
            }
        }

        m_samplingImg = vzt::DeviceImage::fromData(
            device, vzt::ImageUsage::TransferSrc | vzt::ImageUsage::TransferDst | vzt::ImageUsage::Sampled,
            vzt::Format::R32G32B32A32SFloat, m_samplingImgSize, m_samplingImgSize,
            {reinterpret_cast<const uint8_t*>(samplingData.data()), samplingData.size() * sizeof(float)},
            std::log2(m_samplingImgSize) + 1);

        m_samplingView = vzt::ImageView(device, m_samplingImg, vzt::ImageAspect::Color);

        const auto queue = device->getQueue(vzt::QueueType::Graphics | vzt::QueueType::Compute);
        queue->oneShot([this, &pixels](vzt::CommandBuffer& commands) {
            uint32_t mipWidth  = m_samplingImgSize;
            uint32_t mipHeight = m_samplingImgSize;
            uint32_t mipLevels = m_samplingImg.getMipLevels();

            vzt::ImageBarrier barrier{};
            barrier.image      = m_samplingImg;
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
                commands.blit(m_samplingImg, vzt::ImageLayout::TransferSrcOptimal, m_samplingImg,
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

            barrier.image      = m_image;
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
