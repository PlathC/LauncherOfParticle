#include "lop/Ui/Controller/Camera.hpp"

#include <imgui.h>

#include "lop/System/Transform.hpp"
#include "lop/Ui/Controller/Controller.hpp"

namespace lop
{
    CameraController::CameraController(Transform& transform) : m_transform(&transform) {}

    bool CameraController::update(const vzt::Input& inputs)
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse)
            return false;

        bool changed = false;

        const auto handleStrength = [&inputs](float strength) {
            if (inputs.isPressed(vzt::Key::LShift))
                strength *= 2.f;
            if (inputs.isPressed(vzt::Key::LCtrl))
                strength /= 2.f;
            return strength;
        };

        if (inputs.mouseLeftPressed)
        {
            const float     strength = handleStrength(1.f);
            const vzt::Vec2 delta    = vzt::Vec2(inputs.deltaMousePosition);
            m_transform->rotate(strength * inputs.deltaTime * glm::vec3(0.f, -delta.y, delta.x));

            changed = true;
        }

        if (inputs.mouseRightPressed)
        {
            const float strength = handleStrength(1e1f);
            const float delta    = static_cast<float>(inputs.deltaMousePosition.x);
            m_transform->rotate(strength * -inputs.deltaTime * glm::vec3(delta, 0.f, 0.f));

            changed = true;
        }

        vzt::Vec3 translation{};
        if (inputs.isPressed(vzt::Key::W))
            translation += Transform::Front;
        if (inputs.isPressed(vzt::Key::A))
            translation -= Transform::Right;
        if (inputs.isPressed(vzt::Key::S))
            translation -= Transform::Front;
        if (inputs.isPressed(vzt::Key::D))
            translation += Transform::Right;

        if (translation != vzt::Vec3{})
        {
            const float strength = handleStrength(2.f);
            m_transform->translate(strength * inputs.deltaTime * glm::normalize(m_transform->rotation * translation));
            changed = true;
        }

        return changed;
    }
} // namespace lop
