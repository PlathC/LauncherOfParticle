#include <vzt/Data/Camera.hpp>
#include <vzt/Utils/MeshLoader.hpp>
#include <vzt/Vulkan/Buffer.hpp>
#include <vzt/Vulkan/Command.hpp>
#include <vzt/Vulkan/Surface.hpp>
#include <vzt/Vulkan/Swapchain.hpp>
#include <vzt/Window.hpp>

#include "pto/Renderer/Geometry.hpp"
#include "pto/Renderer/View/HardwarePathTracing.hpp"
#include "pto/System/System.hpp"

int main(int argc, char** argv)
{
    const std::string ApplicationName = "Particle throwing";

    auto window   = vzt::Window{ApplicationName};
    auto instance = vzt::Instance{window};
    auto surface  = vzt::Surface{window, instance};

    auto device    = instance.getDevice(vzt::DeviceBuilder::rt(), surface);
    auto hardware  = device.getHardware();
    auto swapchain = vzt::Swapchain{device, surface, window.getExtent()};

    pto::System system{};

    entt::handle     dragonEntity = system.create();
    const vzt::Mesh& dragonMesh   = dragonEntity.emplace<vzt::Mesh>(vzt::readObj("samples/Bunny/bunny.obj"));
    dragonEntity.emplace<pto::GeometryHolder>(device, dragonMesh);

    pto::GeometryHandler         geometryHandler{device, system};
    pto::HardwarePathTracingView pathtracingView{device, swapchain.getImageNb(), window.getExtent(), system,
                                                 geometryHandler};

    // Compute AABB to place camera in front of the model
    vzt::Vec3 minimum{std::numeric_limits<float>::max()};
    vzt::Vec3 maximum{std::numeric_limits<float>::lowest()};
    for (const vzt::Vec3& vertex : dragonMesh.vertices)
    {
        minimum = glm::min(minimum, vertex);
        maximum = glm::max(maximum, vertex);
    }

    vzt::Camera camera{};
    camera.front = vzt::Vec3(0.f, 0.f, 1.f);
    camera.up    = vzt::Vec3(0.f, 1.f, 0.f);
    camera.right = vzt::Vec3(1.f, 0.f, 0.f);

    const vzt::Vec3 target         = (minimum + maximum) * .5f;
    const float     bbRadius       = glm::compMax(glm::abs(maximum - target));
    const float     distance       = bbRadius / std::tan(camera.fov * .5f);
    const vzt::Vec3 cameraPosition = target - camera.front * 1.5f * distance;

    const auto queue       = device.getQueue(vzt::QueueType::Compute);
    auto       commandPool = vzt::CommandPool(device, queue, swapchain.getImageNb());

    std::size_t i = 0;

    pto::HardwarePathTracingView::Properties properties{};
    while (window.update())
    {
        const auto& inputs = window.getInputs();
        if (inputs.windowResized)
            swapchain.setExtent(inputs.windowSize);

        auto submission = swapchain.getSubmission();
        if (!submission)
            continue;

        vzt::Extent2D extent = window.getExtent();

        // Per frame update
        vzt::Quat orientation = {1.f, 0.f, 0.f, 0.f};
        if (inputs.mouseLeftPressed || i < swapchain.getImageNb() || inputs.windowResized)
        {
            float           t               = inputs.mousePosition.x * vzt::Tau / static_cast<float>(window.getWith());
            const vzt::Quat rotation        = glm::angleAxis(t, camera.up);
            const vzt::Vec3 currentPosition = rotation * (cameraPosition - target) + target;

            vzt::Vec3       direction  = glm::normalize(target - currentPosition);
            const vzt::Vec3 reference  = camera.front;
            const float     projection = glm::dot(reference, direction);
            if (std::abs(projection) < 1.f - 1e-6f) // If direction and reference are not the same
                orientation = glm::rotation(reference, direction);
            else if (projection < 0.f) // If direction and reference are opposite
                orientation = glm::angleAxis(-vzt::Pi, camera.up);

            vzt::Mat4 view = camera.getViewMatrix(currentPosition, orientation);
            properties     = {glm::inverse(view), glm::inverse(camera.getProjectionMatrix()), 0};
            i++;
        }

        const vzt::View<vzt::Image> image    = swapchain.getImage(submission->imageId);
        vzt::CommandBuffer          commands = commandPool[submission->imageId];
        {
            commands.begin();
            pathtracingView.record(submission->imageId, commands, image, properties);
            commands.end();
        }

        properties.sampleId++;

        queue->submit(commands, *submission);
        if (!swapchain.present())
        {
            // Wait all commands execution
            device.wait();

            // Apply screen size update
            vzt::Extent2D extent = window.getExtent();
            camera.aspectRatio   = static_cast<float>(extent.width) / static_cast<float>(extent.height);

            pathtracingView.resize(extent);

            i = 0;
        }
    }

    return EXIT_SUCCESS;
}
