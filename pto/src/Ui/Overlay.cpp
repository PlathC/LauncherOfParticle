#include "pto/Ui/Overlay.hpp"

#include <imgui.h>

namespace pto
{
    void Overlay::render(const OverlayContent& content) const
    {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        constexpr ImGuiWindowFlags overlayFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
                                                  ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                                  ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2               window_pos{viewport->WorkPos.x + 2.f, viewport->WorkPos.y + 2.f};
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::SetNextWindowBgAlpha(0.35f);
        if (ImGui::Begin("Overlay", nullptr, overlayFlags))
        {
            content();
            ImGui::End();
        }
    }
} // namespace pto
