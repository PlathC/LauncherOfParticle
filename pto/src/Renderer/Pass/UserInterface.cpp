#include "pto/Renderer/Pass/UserInterface.hpp"

#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
#include <vzt/Vulkan/Command.hpp>
#include <vzt/Vulkan/Device.hpp>
#include <vzt/Vulkan/Instance.hpp>
#include <vzt/Vulkan/Swapchain.hpp>
#include <vzt/Window.hpp>

namespace pto
{
    UserInterfacePass::UserInterfacePass(vzt::Window& window, vzt::View<vzt::Instance> instance,
                                         vzt::View<vzt::Device> device, vzt::View<vzt::Swapchain> swapchain)
        : m_device(device), m_imageNb(swapchain->getImageNb())
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();

        ImGui_ImplVulkan_LoadFunctions(
            [](const char* name, void*) { return vkGetInstanceProcAddr(volkGetLoadedInstance(), name); });

        // Setup Platform/Renderer backends
        ImGui_ImplSDL2_InitForVulkan(window.getHandle());
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance                  = instance->getHandle();
        init_info.PhysicalDevice            = m_device->getHardware().getHandle();
        init_info.Device                    = m_device->getHandle();

        vzt::View<vzt::Queue> graphicsQueue = m_device->getQueue(vzt::QueueType::Graphics);
        init_info.QueueFamily               = graphicsQueue->getId();
        init_info.Queue                     = graphicsQueue->getHandle();
        init_info.PipelineCache             = VK_NULL_HANDLE;

        std::unordered_set<vzt::DescriptorType> poolTypes{
            vzt::DescriptorType::Sampler,
            vzt::DescriptorType::CombinedSampler,
            vzt::DescriptorType::SampledImage,
            vzt::DescriptorType::StorageImage,
            vzt::DescriptorType::UniformTexelBuffer,
            vzt::DescriptorType::StorageTexelBuffer,
            vzt::DescriptorType::UniformBuffer,
            vzt::DescriptorType::StorageBuffer,
            vzt::DescriptorType::UniformBufferDynamic,
            vzt::DescriptorType::StorageBufferDynamic,
            vzt::DescriptorType::InputAttachment,
        };
        m_descriptorPool         = vzt::DescriptorPool(m_device, poolTypes, 1000 * poolTypes.size());
        init_info.DescriptorPool = m_descriptorPool.getHandle();
        init_info.Subpass        = 0;
        init_info.MinImageCount  = 2;
        init_info.ImageCount     = m_imageNb;
        init_info.MSAASamples    = VK_SAMPLE_COUNT_1_BIT;

        m_renderPass = vzt::RenderPass(m_device);

        vzt::AttachmentUse attachment{};
        attachment.format         = swapchain->getFormat();
        attachment.loadOp         = vzt::LoadOp::DontCare;
        attachment.storeOp        = vzt::StoreOp::Store;
        attachment.stencilLoapOp  = vzt::LoadOp::DontCare;
        attachment.stencilStoreOp = vzt::StoreOp::DontCare;
        attachment.initialLayout  = vzt::ImageLayout::Undefined;
        attachment.usedLayout     = vzt::ImageLayout::ColorAttachmentOptimal;
        attachment.finalLayout    = vzt::ImageLayout::PresentSrcKHR;
        m_renderPass.addColor(attachment);

        attachment.format         = m_device->getHardware().getDepthFormat();
        attachment.loadOp         = vzt::LoadOp::DontCare;
        attachment.storeOp        = vzt::StoreOp::DontCare;
        attachment.stencilLoapOp  = vzt::LoadOp::DontCare;
        attachment.stencilStoreOp = vzt::StoreOp::DontCare;
        attachment.initialLayout  = vzt::ImageLayout::Undefined;
        attachment.usedLayout     = vzt::ImageLayout::DepthStencilAttachmentOptimal;
        attachment.finalLayout    = vzt::ImageLayout::DepthStencilAttachmentOptimal;
        m_renderPass.setDepth(attachment);
        m_renderPass.compile();

        ImGui_ImplVulkan_Init(&init_info, m_renderPass.getHandle());

        graphicsQueue->oneShot(
            [&](vzt::CommandBuffer& commands) { ImGui_ImplVulkan_CreateFontsTexture(commands.getHandle()); });
        ImGui_ImplVulkan_DestroyFontUploadObjects();

        m_frameBuffers.reserve(swapchain->getImageNb());
        for (uint32_t i = 0; i < swapchain->getImageNb(); i++)
        {
            m_depthStencils.emplace_back(device, swapchain->getExtent(), vzt::ImageUsage::DepthStencilAttachment,
                                         m_device->getHardware().getDepthFormat());
            m_frameBuffers.emplace_back(m_device, swapchain->getExtent());

            vzt::FrameBuffer& frameBuffer = m_frameBuffers.back();
            frameBuffer.addAttachment(vzt::ImageView(device, swapchain->getImage(i), vzt::ImageAspect::Color));
            frameBuffer.addAttachment(vzt::ImageView(device, m_depthStencils.back(), vzt::ImageAspect::Depth));
            frameBuffer.compile(m_renderPass);
        }

        window.setEventCallback([](SDL_Event* windowEvent) { ImGui_ImplSDL2_ProcessEvent(windowEvent); });
    }

    void UserInterfacePass::resize(vzt::Extent2D extent) {}

    void UserInterfacePass::record(uint32_t imageId, vzt::CommandBuffer& commands,
                                   const vzt::View<vzt::DeviceImage> outputImage)
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        bool showDemoWindow = true;
        ImGui::ShowDemoWindow(&showDemoWindow);

        ImGui::Render();

        commands.beginPass(m_renderPass, m_frameBuffers[imageId]);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commands.getHandle());
        commands.endPass();
    }
} // namespace pto
