#include <fmt/chrono.h>
#include <imgui.h>
#include <vzt/Core/Logger.hpp>
#include <vzt/Data/Camera.hpp>
#include <vzt/Utils/IOMesh.hpp>
#include <vzt/Vulkan/Buffer.hpp>
#include <vzt/Vulkan/Command.hpp>
#include <vzt/Vulkan/Surface.hpp>
#include <vzt/Vulkan/Swapchain.hpp>
#include <vzt/Window.hpp>

#include "pto/Renderer/Geometry.hpp"
#include "pto/Renderer/Pass/HardwarePathTracing.hpp"
#include "pto/Renderer/Pass/UserInterface.hpp"
#include "pto/Renderer/Snapshot.hpp"
#include "pto/System/System.hpp"
#include "pto/System/Transform.hpp"
#include "pto/Ui/Camera.hpp"
#include "pto/Ui/Overlay.hpp"

auto proceduralSky(const vzt::Vec3 rd) -> vzt::Vec4
{
    const vzt::Vec3 palette[2] = {vzt::Vec3(0.557f, 0.725f, 0.984f) * 1.1f, vzt::Vec3(0.957f, 0.573f, 0.445f) * 1.2f};
    const float     angle      = std::acos(glm::dot(rd, vzt::Vec3(0.f, 0.f, 1.f)));

    vzt::Vec3 color = glm::pow(glm::mix(palette[0], palette[1], std::abs(angle) / (vzt::Pi)), vzt::Vec3(1.5f));
    if (angle < 0.3f)
        color += glm::smoothstep(0.f, 0.3f, 0.3f - angle) * 100.f;

    return vzt::Vec4(color, 1.f);
}

int main(int argc, char** argv)
{
    const std::string ApplicationName = "Launcher of particles";

    auto window   = vzt::Window{ApplicationName};
    auto instance = vzt::Instance{window};
    auto surface  = vzt::Surface{window, instance};

    auto device    = instance.getDevice(vzt::DeviceBuilder::rt(), surface);
    auto hardware  = device.getHardware();
    auto swapchain = vzt::Swapchain{device, surface, window.getExtent()};

    pto::System system{};

    entt::handle entity = system.create();
    entity.emplace<pto::Name>("Dragon");
    vzt::Mesh& mesh = entity.emplace<vzt::Mesh>(vzt::readObj("samples/Dragon/dragon.obj"));

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

    // pto::Environment sky = pto::Environment::fromFunction(device, proceduralSky);
    pto::Environment sky = pto::Environment::fromFile(device, "vestibule_4k.exr");

    pto::HardwarePathTracingPass pathtracingPass{
        device, swapchain.getImageNb(), window.getExtent(), geometryHandler, std::move(sky),
    };
    pto::UserInterfacePass userInterfacePass{window, instance, device, swapchain};

    vzt::Camera camera{};
    camera.up    = pto::Transform::Up;
    camera.front = pto::Transform::Front;
    camera.right = pto::Transform::Right;

    const vzt::Vec3     target          = (minimum + maximum) * .5f;
    const float         bbRadius        = glm::compMax(glm::abs(maximum - target));
    const float         distance        = bbRadius / std::tan(camera.fov * .5f);
    const vzt::Vec3     cameraPosition  = target - camera.front * 1.5f * distance;
    pto::Transform      cameraTransform = {cameraPosition};
    pto::ControllerList cameraControllers{};
    cameraControllers.add<pto::CameraController>(cameraTransform);

    pto::Overlay overlay{};

    const auto queue       = device.getQueue(vzt::QueueType::Compute);
    auto       commandPool = vzt::CommandPool(device, queue, swapchain.getImageNb());

    vzt::Mat4 view = camera.getViewMatrix(cameraTransform.position, cameraTransform.rotation);
    pto::HardwarePathTracingPass::Properties properties{glm::inverse(view), camera.getProjectionMatrix(), 0};
    while (window.update())
    {
        const auto& inputs = window.getInputs();
        if (inputs.windowResized)
            swapchain.setExtent(inputs.windowSize);

        auto submission = swapchain.getSubmission();
        if (!submission)
            continue;

        const vzt::View<vzt::DeviceImage> backBuffer = swapchain.getImage(submission->imageId);

        vzt::Extent2D extent = window.getExtent();

        // Per frame update
        vzt::Quat orientation = {1.f, 0.f, 0.f, 0.f};
        if (cameraControllers.update(inputs) || inputs.windowResized)
        {
            view                  = camera.getViewMatrix(cameraTransform.position, cameraTransform.rotation);
            properties.view       = glm::inverse(view);
            properties.projection = camera.getProjectionMatrix();
            properties.sampleId   = 0;
        }

        userInterfacePass.startFrame();
        ImGuiIO& io = ImGui::GetIO();

        // Main menu bar
        {
            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("New"))
                        vzt::logger::info("New !");
                    if (ImGui::MenuItem("Open", "Ctrl+O"))
                        vzt::logger::info("Open !");

                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }
        }

        overlay.render([&]() {
            ImGui::Text("Launcher of particles");
            ImGui::Separator();
            ImGui::Text("Framerate: (%.1f)", io.Framerate);
            ImGui::Text("SPP: (%d)", properties.sampleId);
        });

        // Main window
        {
            ImGui::SetNextWindowBgAlpha(0.35f);
            if (ImGui::Begin("Main", nullptr, 0))
            {
                int32_t maxSample = properties.maxSample;
                if (ImGui::InputInt("Max sample", &maxSample, -1, 100))
                    properties.maxSample = maxSample;

                bool transparentBackground = properties.transparentBackground;
                if (ImGui::Checkbox("Transparent background", &transparentBackground))
                {
                    properties.transparentBackground = transparentBackground;
                    properties.sampleId              = 0;
                }

                static std::string fileName{"Output.png"};
                fileName.resize(128);
                ImGui::InputText("Output name", fileName.data(), fileName.size());

                if (ImGui::Button("Save"))
                    pto::snapshot(device, pathtracingPass.getRenderImage(), fileName);

                ImGui::Separator();
                if (ImGui::CollapsingHeader("World"))
                {
                    static std::string_view selectedName;
                    if (ImGui::BeginListBox("Entities"))
                    {
                        const auto transformView = system.registry.view<pto::Name>();
                        for (const entt::entity entity : transformView)
                        {
                            const auto& name = system.registry.get<pto::Name>(entity);

                            bool isSelected = selectedName == name.value;
                            if (ImGui::Selectable(name.value.c_str(), &isSelected))
                                selectedName = name.value;

                            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                            if (isSelected)
                                ImGui::SetItemDefaultFocus();
                        }

                        ImGui::EndListBox();
                    }
                }
                ImGui::End();
            }
        }

        vzt::CommandBuffer commands = commandPool[submission->imageId];
        {
            commands.begin();
            {
                pathtracingPass.record(submission->imageId, commands, backBuffer, properties);
                userInterfacePass.record(submission->imageId, commands, backBuffer);
            }
            commands.end();
        }

        queue->submit(commands, *submission);

        // Handle resize
        if (!swapchain.present())
        {
            // Wait all commands execution
            device.wait();

            // Apply screen size update
            vzt::Extent2D extent = window.getExtent();
            camera.aspectRatio   = static_cast<float>(extent.width) / static_cast<float>(extent.height);

            pathtracingPass.resize(extent);
            userInterfacePass.resize(extent);
        }

        if (properties.maxSample == -1 || properties.sampleId < properties.maxSample)
            properties.sampleId++;
    }

    return EXIT_SUCCESS;
}
