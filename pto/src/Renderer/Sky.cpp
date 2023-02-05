#include "pto/Renderer/Sky.hpp"

#include <vzt/Core/Math.hpp>
#include <vzt/Utils/IOHDR.hpp>
#include <vzt/Vulkan/Command.hpp>
#include <vzt/Vulkan/Device.hpp>

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
        : m_image(
              vzt::DeviceImage::fromData(device, vzt::ImageUsage::Sampled, vzt::Format::R32G32B32A32SFloat, pixels)),
          m_view(device, m_image, vzt::ImageAspect::Color), m_sampler(device)
    {
        const auto queue = device->getQueue(vzt::QueueType::Graphics | vzt::QueueType::Compute);

        queue->oneShot([this](vzt::CommandBuffer& commands) {
            vzt::ImageBarrier barrier{};
            barrier.image     = m_image;
            barrier.oldLayout = vzt::ImageLayout::Undefined;
            barrier.newLayout = vzt::ImageLayout::ShaderReadOnlyOptimal;
            commands.barrier(vzt::PipelineStage::TopOfPipe, vzt::PipelineStage::BottomOfPipe, barrier);
        });
    }

} // namespace pto
