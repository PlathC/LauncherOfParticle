#ifndef LOP_RENDERER_SNAPSHOT_HPP
#define LOP_RENDERER_SNAPSHOT_HPP

#include <vzt/Core/File.hpp>
#include <vzt/Vulkan/Command.hpp>
#include <vzt/Vulkan/Image.hpp>

namespace lop
{
    void snapshot(vzt::View<vzt::Device> device, vzt::View<vzt::DeviceImage> outputImage, const vzt::Path& outputPath);
} // namespace lop

#endif // PTO_RENDERER_VIEW_SNAPSHOT_HPP
