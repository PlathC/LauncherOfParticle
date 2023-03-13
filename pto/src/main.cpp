#include <vzt/Data/Camera.hpp>
#include <vzt/Utils/IOMesh.hpp>
#include <vzt/Vulkan/Buffer.hpp>
#include <vzt/Vulkan/Command.hpp>
#include <vzt/Vulkan/Surface.hpp>
#include <vzt/Vulkan/Swapchain.hpp>
#include <vzt/Window.hpp>

#include "pto/Renderer/Geometry.hpp"
#include "pto/Renderer/View/HardwarePathTracing.hpp"
#include "pto/Renderer/View/Snapshot.hpp"
#include "pto/System/System.hpp"
#include "pto/System/Transform.hpp"
#include "pto/Ui/Camera.hpp"

auto proceduralSky(const vzt::Vec3 rd) -> vzt::Vec4
{
    const vzt::Vec3 palette[2] = {vzt::Vec3(0.557f, 0.725f, 0.984f), vzt::Vec3(0.957f, 0.373f, 0.145f)};
    const float     angle      = std::acos(glm::dot(rd, vzt::Vec3(0.f, 0.f, 1.f)));

    vzt::Vec3 color = glm::pow(glm::mix(palette[0], palette[1], std::abs(angle) / (vzt::Pi)), vzt::Vec3(1.5f));
    if (angle < 0.3f)
        color += glm::smoothstep(0.f, 0.3f, 0.3f - angle) * 100.f;

    return vzt::Vec4(color, 1.f);
}

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

    entt::handle entity = system.create();
    vzt::Mesh&   mesh   = entity.emplace<vzt::Mesh>(vzt::readObj("samples/Dragon/dragon.obj"));

    // Compute AABB to place camera in front of the model
    vzt::Vec3 minimum{std::numeric_limits<float>::max()};
    vzt::Vec3 maximum{std::numeric_limits<float>::lowest()};
    for (std::size_t i = 0; i < mesh.vertices.size(); i++)
    {
        vzt::Vec3& vertex = mesh.vertices[i];
        vzt::Vec3& normal = mesh.normals[i];

        // Current model is Y up
        std::swap(vertex.y, vertex.z);
        std::swap(normal.y, normal.z);

        minimum = glm::min(minimum, vertex);
        maximum = glm::max(maximum, vertex);
    }

    entity.emplace<pto::MeshHolder>(device, mesh);
    pto::MeshHandler geometryHandler{device, system};

    pto::Environment sky = pto::Environment::fromFunction(device, proceduralSky);
    // pto::Environment sky = pto::Environment::fromFile(device, "vestibule_4k.exr");

    pto::HardwarePathTracingView pathtracingView{
        device, swapchain.getImageNb(), window.getExtent(), system, geometryHandler, std::move(sky),
    };

    vzt::Camera camera{};
    camera.up    = pto::Transform::Up;
    camera.front = pto::Transform::Front;
    camera.right = pto::Transform::Right;

    const vzt::Vec3 target         = (minimum + maximum) * .5f;
    const float     bbRadius       = glm::compMax(glm::abs(maximum - target));
    const float     distance       = bbRadius / std::tan(camera.fov * .5f);
    const vzt::Vec3 cameraPosition = target - camera.front * 1.5f * distance;

    pto::Transform cameraTransform{cameraPosition};

    pto::ControllerList cameraControllers{};
    cameraControllers.add<pto::CameraController>(cameraTransform);

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
        if (cameraControllers.update(inputs) || i < swapchain.getImageNb() || inputs.windowResized)
        {
            vzt::Mat4 view = camera.getViewMatrix(cameraTransform.position, cameraTransform.rotation);
            properties     = {glm::inverse(view), camera.getProjectionMatrix(), 0};
            i++;
        }

        const vzt::View<vzt::DeviceImage> image    = swapchain.getImage(submission->imageId);
        vzt::CommandBuffer                commands = commandPool[submission->imageId];
        {
            commands.begin();
            pathtracingView.record(submission->imageId, commands, image, properties);

            commands.end();
        }

        if (inputs.isClicked(vzt::Key::Space))
            pto::snapshot(device, image, window.getExtent(), "test.png");

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
