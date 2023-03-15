#include "pto/Renderer/Pass/HardwarePathTracing.hpp"

namespace pto
{
    inline vzt::View<vzt::DeviceImage> HardwarePathTracingPass::getRenderImage() const { return m_renderImage; }
} // namespace pto
