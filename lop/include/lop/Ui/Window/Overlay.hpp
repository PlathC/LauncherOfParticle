#ifndef LOP_UI_OVERLAY_HPP
#define LOP_UI_OVERLAY_HPP

#include <functional>

namespace lop
{
    class Overlay
    {
      public:
        using OverlayContent = std::function<void()>;
        void render(const OverlayContent& content) const;
    };
} // namespace lop

#endif // LOP_UI_OVERLAY_HPP
