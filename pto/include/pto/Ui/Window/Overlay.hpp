#ifndef PTO_UI_OVERLAY_HPP
#define PTO_UI_OVERLAY_HPP

#include <functional>

namespace pto
{
    class Overlay
    {
      public:
        using OverlayContent = std::function<void()>;
        void render(const OverlayContent& content) const;
    };
} // namespace pto

#endif // PTO_UI_OVERLAY_HPP
