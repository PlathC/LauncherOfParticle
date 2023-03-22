#ifndef LOP_RENDERER_ENVIRONMENT_HPP
#define LOP_RENDERER_ENVIRONMENT_HPP

#include <functional>

#include <vzt/Core/File.hpp>
#include <vzt/Core/Math.hpp>
#include <vzt/Vulkan/Image.hpp>
#include <vzt/Vulkan/Texture.hpp>

namespace lop
{
    struct SkyDeviceData
    {
        uint32_t width;
        uint32_t height;
        uint64_t pixelAddress;
    };

    class Environment
    {
      public:
        static Environment fromFile(vzt::View<vzt::Device> device, const vzt::Path& path);

        using ProceduralEnvironmentFunction = std::function<vzt::Vec4(const vzt::Vec3 direction)>;
        static Environment fromFunction(vzt::View<vzt::Device> device, const ProceduralEnvironmentFunction& function,
                                        uint32_t width = 4096, uint32_t height = 4096);

        Environment(const vzt::View<vzt::Device> device, const Image<float>& pixels);

        Environment(const Environment&)            = delete;
        Environment& operator=(const Environment&) = delete;

        Environment(Environment&& other) noexcept            = default;
        Environment& operator=(Environment&& other) noexcept = default;

        ~Environment() = default;

        vzt::DeviceImage image;
        vzt::ImageView   view;
        vzt::Sampler     sampler;

        uint32_t         samplingSize;
        vzt::DeviceImage samplingImg;
        vzt::ImageView   samplingView;
    };
} // namespace lop

#endif // LOP_RENDERER_ENVIRONMENT_HPP
