#include "pto/Ui/Camera.hpp"

#include "pto/System/Transform.hpp"
#include "pto/Ui/Controller.hpp"

namespace pto
{
    CameraController::CameraController(Transform& transform) : m_transform(&transform) {}

    bool CameraController::update(const vzt::Input& inputs)
    {
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
            const float     strength = handleStrength(1e1f);
            const vzt::Vec2 delta    = vzt::Vec2(inputs.deltaMousePosition);
            m_transform->rotate(strength * -inputs.deltaTime * glm::vec3(delta.y, 0.f, delta.x));

            changed = true;
        }

        if (inputs.mouseRightPressed)
        {
            const float strength = handleStrength(1e1f);
            const float delta    = static_cast<float>(inputs.deltaMousePosition.x);
            m_transform->rotate(strength * -inputs.deltaTime * glm::vec3(0.f, delta, 0.f));

            changed = true;
        }

        vzt::Vec3 translation{};
        if (inputs.isPressed(vzt::Key::W))
            translation += pto::Transform::Front;
        if (inputs.isPressed(vzt::Key::A))
            translation -= pto::Transform::Right;
        if (inputs.isPressed(vzt::Key::S))
            translation -= pto::Transform::Front;
        if (inputs.isPressed(vzt::Key::D))
            translation += pto::Transform::Right;

        if (translation != vzt::Vec3{})
        {
            const float strength = handleStrength(2.f);
            m_transform->translate(strength * inputs.deltaTime * glm::normalize(m_transform->rotation * translation));
            changed = true;
        }

        return changed;
    }
} // namespace pto
