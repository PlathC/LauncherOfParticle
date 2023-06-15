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

#include "lop/Renderer/Geometry.hpp"
#include "lop/Renderer/Pass/HardwarePathTracing.hpp"
#include "lop/Renderer/Pass/UserInterface.hpp"
#include "lop/Renderer/Snapshot.hpp"
#include "lop/System/System.hpp"
#include "lop/System/Transform.hpp"
#include "lop/Ui/Controller/Camera.hpp"
#include "lop/Ui/Window/Overlay.hpp"

#include <portable-file-dialogs.h>

auto proceduralSky(const vzt::Vec3 rd) -> vzt::Vec3
{
    const vzt::Vec3 palette[2] = {vzt::Vec3(0.557f, 0.725f, 0.984f) * 1.1f, vzt::Vec3(0.957f, 0.573f, 0.445f) * 1.2f};
    const float     angle      = std::acos(glm::dot(rd, vzt::Vec3(0.f, 0.f, 1.f)));

    vzt::Vec3 color = glm::pow(glm::mix(palette[0], palette[1], std::abs(angle) / (vzt::Pi)), vzt::Vec3(1.5f));
    if (angle < 0.3f)
        color += glm::smoothstep(0.f, 0.3f, 0.3f - angle) * 100.f;

    return color;
}

int main(int argc, char** argv)
{
    const std::string ApplicationName = "Launcher of particle";

    auto window   = vzt::Window{ApplicationName};
    auto instance = vzt::Instance{window};
    auto surface  = vzt::Surface{window, instance};

    auto device    = instance.getDevice(vzt::DeviceBuilder::rt(), surface);
    auto hardware  = device.getHardware();
    auto swapchain = vzt::Swapchain{device, surface, window.getExtent()};

    lop::System system{};

    lop::MeshHandler geometryHandler{device, system};

    lop::HardwarePathTracingPass pathtracingPass{
        device,
        swapchain.getImageNb(),
        window.getExtent(),
        geometryHandler,
        lop::Environment::fromFunction(device, proceduralSky),
    };
    lop::UserInterfacePass userInterfacePass{window, instance, device, swapchain};

    vzt::Camera camera{};
    camera.up    = lop::Transform::Up;
    camera.front = lop::Transform::Front;
    camera.right = lop::Transform::Right;

    const vzt::Vec3     cameraPosition  = -10.f * camera.front;
    lop::Transform      cameraTransform = {cameraPosition};
    lop::ControllerList cameraControllers{};
    cameraControllers.add<lop::CameraController>(cameraTransform);

    lop::Overlay overlay{};

    const auto queue       = device.getQueue(vzt::QueueType::Compute);
    auto       commandPool = vzt::CommandPool(device, queue, swapchain.getImageNb());

    vzt::Mat4 view = camera.getViewMatrix(cameraTransform.position, cameraTransform.rotation);
    lop::HardwarePathTracingPass::Properties properties{glm::inverse(view), camera.getProjectionMatrix(), 0};

    bool forceUpdate = false;
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
        if (cameraControllers.update(inputs) || inputs.windowResized || forceUpdate)
        {
            view                  = camera.getViewMatrix(cameraTransform.position, cameraTransform.rotation);
            properties.view       = glm::inverse(view);
            properties.projection = camera.getProjectionMatrix();
            properties.sampleId   = 0;

            forceUpdate = false;
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
            ImGui::Text("Launcher of particle");
            ImGui::Separator();
            ImGui::Text("Framerate: (%.1f)", io.Framerate);
            ImGui::Text("SPP: (%d)", properties.sampleId);
        });

        // Main window
        {
            ImGui::SetNextWindowBgAlpha(0.35f);
            if (ImGui::Begin("Main", nullptr, 0))
            {
                ImGui::SeparatorText("Configuration");
                int32_t maxSample = properties.maxSample;
                if (ImGui::InputInt("Max sample", &maxSample, 0, 100))
                    properties.maxSample = maxSample;

                int32_t bounces = properties.bounces;
                if (ImGui::InputInt("Bounces", &bounces, 0, 128))
                    properties.bounces = bounces;

                ImGui::SeparatorText("Export");
                {
                    bool transparentBackground = properties.transparentBackground;
                    if (ImGui::Checkbox("Transparent background", &transparentBackground))
                    {
                        properties.transparentBackground = transparentBackground;
                        properties.sampleId              = 0;
                    }

                    static std::string fileName = "";
                    if (ImGui::Button("Select file"))
                    {
                        auto fileDialog = pfd::save_file("Choose file to save", pfd::path::home(),
                                                         {"Image file (.png)", "*.png"}, pfd::opt::force_overwrite);
                        fileName        = fileDialog.result();
                    }
                    ImGui::SameLine();
                    ImGui::InputText("##ExportFile", fileName.data(), fileName.size() + 1);

                    if (ImGui::Button("Export") && !fileName.empty())
                    {
                        lop::snapshot(device, pathtracingPass.getRenderImage(), fileName);
                        pfd::message("Export done!", fmt::format("{} has been saved.", fileName), pfd::choice::ok,
                                     pfd::icon::info);
                    }
                }

                ImGui::Separator();
                if (ImGui::CollapsingHeader("World"))
                {
                    ImGui::SeparatorText("Environment");
                    {
                        static std::string fileName = "";
                        if (ImGui::Button("Select environment file"))
                        {
                            auto fileDialog = pfd::open_file("Choose files environment file", pfd::path::home(),
                                                             {"Environment Files (.exr)", "*.exr"});
                            auto results    = fileDialog.result();
                            if (!results.empty())
                            {
                                fileName = results.back();
                                pathtracingPass.setEnvironment(lop::Environment::fromFile(device, fileName));
                                properties.sampleId = 0;
                            }
                        }
                    }

                    ImGui::SeparatorText("Entities");

                    static entt::entity selected = entt::null;
                    if (ImGui::BeginListBox("##Entities"))
                    {
                        const auto transformView = system.registry.view<lop::Name, lop::Transform>();
                        for (const entt::entity entity : transformView)
                        {
                            const auto& name = system.registry.get<lop::Name>(entity);

                            bool isSelected = selected == entity;
                            if (ImGui::Selectable(fmt::format("{}##{}", name.value, vzt::toUnderlying(entity)).c_str(),
                                                  &isSelected))
                                selected = entity;

                            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                            if (isSelected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndListBox();
                    }

                    {
                        static std::string fileName = "";
                        if (ImGui::Button("Add"))
                        {
                            auto fileDialog =
                                pfd::open_file("Choose wavefront file", pfd::path::home(),
                                               {"Wavefront Files (.obj)", "*.obj"}, pfd::opt::multiselect);
                            auto results = fileDialog.result();
                            for (vzt::Path result : results)
                            {
                                entt::handle entity = system.create();
                                entity.emplace<lop::Name>(result.filename().stem().string());
                                entity.emplace<lop::Material>();
                                entity.emplace<lop::Transform>();
                                auto& newMesh = entity.emplace<vzt::Mesh>(vzt::readObj(result));
                                entity.emplace<lop::MeshHolder>(device, newMesh);

                                selected = entity;
                            }
                            geometryHandler.update();
                            pathtracingPass.update();
                            properties.sampleId = 0;
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Remove") && selected != entt::null)
                    {
                        system.registry.destroy(selected);
                        selected = entt::null;

                        geometryHandler.update();
                        pathtracingPass.update();
                        properties.sampleId = 0;
                    }

                    if (selected != entt::null)
                    {
                        const auto& [name, transform, material] =
                            system.registry.get<lop::Name, lop::Transform, lop::Material>(selected);
                        ImGui::SeparatorText(fmt::format("{}", name.value).c_str());

                        ImGui::Text("Position");

                        bool       update   = false;
                        glm::vec3& position = transform.position;
                        update |= ImGui::InputFloat("X", &position.x, 0.01f, 1.0f, "%.3f");
                        update |= ImGui::InputFloat("Y", &position.y, 0.01f, 1.0f, "%.3f");
                        update |= ImGui::InputFloat("Z", &position.z, 0.01f, 1.0f, "%.3f");

                        ImGui::Text("Material");

                        glm::vec3& baseColor = material.baseColor;
                        update |= ImGui::SliderFloat("Scatt R", &baseColor.x, 0.f, 1.0f, "%.3f");
                        update |= ImGui::SliderFloat("Scatt G", &baseColor.y, 0.f, 1.0f, "%.3f");
                        update |= ImGui::SliderFloat("Scatt B", &baseColor.z, 0.f, 1.0f, "%.3f");

                        update |= ImGui::SliderFloat("Roughness", &material.roughness, 0.f, 1.0f, "%.3f");
                        update |= ImGui::SliderFloat("Metallic", &material.metallic, 0.f, 1.0f, "%.3f");
                        update |= ImGui::SliderFloat("IOR", &material.ior, 1.f, 3.0f, "%.3f");
                        update |= ImGui::SliderFloat("Transmission", &material.specularTransmission, 0.f, 1.0f, "%.3f");
                        update |= ImGui::SliderFloat("SpecularTint", &material.specularTint, 0.f, 1.0f, "%.3f");

                        glm::vec3& transmittance = material.transmittance;
                        update |= ImGui::SliderFloat("Transm R", &transmittance.x, 0.f, 1.0f, "%.3f");
                        update |= ImGui::SliderFloat("Transm G", &transmittance.y, 0.f, 1.0f, "%.3f");
                        update |= ImGui::SliderFloat("Transm B", &transmittance.z, 0.f, 1.0f, "%.3f");

                        update |= ImGui::SliderFloat("atDistance", &material.atDistance, 0.f, 100.0f, "%.3f");
                        update |= ImGui::SliderFloat("clearcoat", &material.clearcoat, 0.f, 1.f, "%.3f");
                        update |= ImGui::SliderFloat("clearcoatGloss", &material.clearcoatGloss, 0.f, 1.f, "%.3f");

                        glm::vec3& emission = material.emission;
                        update |= ImGui::SliderFloat("Emission R", &emission.x, 0.f, 1000.0f, "%.3f");
                        update |= ImGui::SliderFloat("Emission G", &emission.y, 0.f, 1000.0f, "%.3f");
                        update |= ImGui::SliderFloat("Emission B", &emission.z, 0.f, 1000.0f, "%.3f");

                        if (update)
                        {
                            geometryHandler.update();
                            pathtracingPass.update();
                            properties.sampleId = 0;
                        }
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

            forceUpdate = true;
        }

        if (properties.maxSample == 0 || properties.sampleId < properties.maxSample)
            properties.sampleId++;
    }

    return EXIT_SUCCESS;
}
