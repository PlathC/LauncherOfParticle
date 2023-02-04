#ifndef PTO_UI_CONTROLLER_HPP
#define PTO_UI_CONTROLLER_HPP

#include <memory>

#include <vzt/Ui/Input.hpp>

namespace pto
{
    class Controller
    {
      public:
        virtual ~Controller() = default;

        virtual bool update(const vzt::Input& inputs) = 0;
    };

    class ControllerList : public Controller
    {
      public:
        ~ControllerList() override = default;

        template <class ControllerType, class... Args>
        ControllerType& add(Args&&... args)
        {
            static_assert(std::is_base_of<Controller, ControllerType>::value,
                          "ControllerType must inherit from Controller.");

            auto controller = std::make_unique<ControllerType>(std::forward<Args>(args)...);

            ControllerType* ptr = controller.get();
            m_controllers.emplace_back(std::move(controller));
            return *ptr;
        }

        bool update(const vzt::Input& inputs) override
        {
            bool result = false;
            for (auto& controller : m_controllers)
                result |= controller->update(inputs);
            return result;
        }

      private:
        std::vector<std::unique_ptr<Controller>> m_controllers;
    };
} // namespace pto

#endif // PTO_UI_CONTROLLER_HPP
