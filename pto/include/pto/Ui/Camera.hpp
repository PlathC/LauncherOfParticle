#ifndef PTO_UI_CAMERA_HPP
#define PTO_UI_CAMERA_HPP

#include "pto/Ui/Controller.hpp"

namespace pto
{
    struct Transform;

    class CameraController : public Controller
    {
      public:
        CameraController(Transform& transform);
        ~CameraController() override = default;

        bool update(const vzt::Input& inputs) override;

      private:
        Transform* m_transform;
    };
} // namespace pto

#endif // PTO_UI_CAMERA_HPP
