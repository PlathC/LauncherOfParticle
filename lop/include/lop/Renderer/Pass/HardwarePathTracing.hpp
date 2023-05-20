#ifndef LOP_RENDERER_PASS_HARDWAREPATHTRACING_HPP
#define LOP_RENDERER_PASS_HARDWAREPATHTRACING_HPP

#include <vzt/Utils/Compiler.hpp>
#include <vzt/Vulkan/AccelerationStructure.hpp>
#include <vzt/Vulkan/Buffer.hpp>
#include <vzt/Vulkan/Command.hpp>
#include <vzt/Vulkan/Pipeline/RaytracingPipeline.hpp>

#include "lop/Renderer/Environment.hpp"
#include "lop/Renderer/Geometry.hpp"
#include "lop/System/System.hpp"

namespace lop
{
    class HardwarePathTracingPass
    {
      public:
        struct Properties
        {
            vzt::Mat4 view;
            vzt::Mat4 projection;
            uint32_t  sampleId              = 0;
            uint32_t  maxSample             = 0;
            uint32_t  transparentBackground = 0;
            uint32_t  jittering             = 1;
        };

        HardwarePathTracingPass(vzt::View<vzt::Device> device, uint32_t imageNb, vzt::Extent2D extent,
                                vzt::View<MeshHandler> handler, Environment environment);
        ~HardwarePathTracingPass() = default;

        void setEnvironment(Environment environment);

        void resize(vzt::Extent2D extent);
        void update();

        inline vzt::View<vzt::DeviceImage> getRenderImage() const;

        void record(uint32_t imageId, vzt::CommandBuffer& commands, const vzt::View<vzt::DeviceImage> outputImage,
                    Properties properties);

      private:
        vzt::View<vzt::Device> m_device;
        uint32_t               m_imageNb;

        vzt::ShaderGroup        m_shaderGroup;
        vzt::Compiler           m_compiler{};
        vzt::DescriptorLayout   m_layout;
        vzt::RaytracingPipeline m_pipeline;

        vzt::DeviceImage m_accumulationImage;
        vzt::ImageView   m_accumulationImageView;

        vzt::DeviceImage m_renderImage;
        vzt::ImageView   m_renderImageView;

        vzt::DescriptorPool m_descriptorPool;
        std::size_t         m_uboAlignment;
        vzt::Buffer         m_ubo;

        uint32_t    m_handleSizeAligned;
        uint32_t    m_handleSize;
        vzt::Buffer m_raygenShaderBindingTable;
        vzt::Buffer m_missShaderBindingTable;
        vzt::Buffer m_hitShaderBindingTable;

        vzt::Extent2D          m_extent;
        vzt::View<MeshHandler> m_handler;
        Environment            m_environment;
    };
} // namespace lop

#include "lop/Renderer/Pass/HardwarePathTracing.inl"

#endif // LOP_RENDERER_PASS_HARDWAREPATHTRACING_HPP
