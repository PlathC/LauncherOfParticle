#include "lop/Renderer/Pass/HardwarePathTracing.hpp"

namespace lop
{
    inline vzt::View<vzt::DeviceImage> HardwarePathTracingPass::getRenderImage() const { return m_renderImage; }
} // namespace lop
