#ifndef PTO_RENDERER_SKY_HPP
#define PTO_RENDERER_SKY_HPP

#include <functional>

#include <vzt/Core/File.hpp>
#include <vzt/Core/Math.hpp>
#include <vzt/Vulkan/Image.hpp>
#include <vzt/Vulkan/Texture.hpp>

namespace pto
{
    struct SkyDeviceData
    {
        uint32_t width;
        uint32_t height;
        uint64_t pixelAddress;
    };

    class Sky
    {
      public:
        static Sky fromFile(vzt::View<vzt::Device> device, const vzt::Path& path);

        using ProceduralSkyFunction = std::function<vzt::Vec4(const vzt::Vec3 direction)>;
        static Sky fromFunction(vzt::View<vzt::Device> device, const ProceduralSkyFunction& function,
                                uint32_t width = 1080, uint32_t height = 1080);

        Sky(const vzt::View<vzt::Device> device, const Image<float>& pixels);

        Sky(const Sky&)            = delete;
        Sky& operator=(const Sky&) = delete;

        Sky(Sky&& other) noexcept            = default;
        Sky& operator=(Sky&& other) noexcept = default;

        ~Sky() = default;

        inline const vzt::ImageView& getImageView() const;
        inline const vzt::Sampler&   getSampler() const;

      private:
        vzt::DeviceImage m_image;
        vzt::ImageView   m_view;
        vzt::Sampler     m_sampler;
    };
} // namespace pto

#include "pto/Renderer/Sky.inl"

#endif // PTO_RENDERER_SKY_HPP
