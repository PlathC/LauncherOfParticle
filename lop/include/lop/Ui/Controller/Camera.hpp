#ifndef LOP_UI_CONTROLLER_CAMERA_HPP
#define LOP_UI_CONTROLLER_CAMERA_HPP

#include "lop/Ui/Controller/Controller.hpp"

namespace lop
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
} // namespace lop

#endif // LOP_UI_CONTROLLER_CAMERA_HPP
