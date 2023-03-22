#ifndef LOP_RENDERER_USERINTERFACE_HPP
#define LOP_RENDERER_USERINTERFACE_HPP

#include <vzt/Core/Math.hpp>
#include <vzt/Core/Type.hpp>
#include <vzt/Vulkan/Descriptor.hpp>
#include <vzt/Vulkan/FrameBuffer.hpp>
#include <vzt/Vulkan/Image.hpp>
#include <vzt/Vulkan/RenderPass.hpp>

namespace vzt
{
    class CommandBuffer;
    class Device;
    class Instance;
    class Swapchain;
    class Window;
} // namespace vzt

namespace lop
{
    class UserInterfacePass
    {
      public:
        UserInterfacePass(vzt::Window& window, vzt::View<vzt::Instance> instance, vzt::View<vzt::Device> device,
                          vzt::View<vzt::Swapchain> swapchain);
        ~UserInterfacePass();

        void startFrame() const;
        void resize(vzt::Extent2D extent);
        void record(uint32_t imageId, vzt::CommandBuffer& commands, const vzt::View<vzt::DeviceImage> outputImage);

      private:
        vzt::View<vzt::Device>    m_device;
        vzt::View<vzt::Swapchain> m_swapchain;
        uint32_t                  m_imageNb;

        vzt::DescriptorPool           m_descriptorPool;
        vzt::RenderPass               m_renderPass;
        std::vector<vzt::FrameBuffer> m_frameBuffers;
        std::vector<vzt::DeviceImage> m_depthStencils;
    };
} // namespace lop

#endif // LOP_RENDERER_USERINTERFACE_HPP
