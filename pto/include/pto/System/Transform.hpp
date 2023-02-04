#ifndef PTO_SYSTEM_TRANSFORM_HPP
#define PTO_SYSTEM_TRANSFORM_HPP

#include <vzt/Core/Math.hpp>

namespace pto
{
    struct Transform
    {
        static constexpr vzt::Vec3 Up    = {0.f, 0.f, 1.f};
        static constexpr vzt::Vec3 Front = {0.f, 1.f, 0.f};
        static constexpr vzt::Vec3 Right = {1.f, 0.f, 0.f};

        vzt::Vec3 position{};
        vzt::Quat rotation{1.f, 0.f, 0.f, 0.f};

        inline void rotate(float angle, const vzt::Vec3& axis);
        inline void rotate(const vzt::Vec3 angles);
        inline void translate(const vzt::Vec3& t);

        void lookAt(const vzt::Vec3& target);
    };
} // namespace pto

#include "pto/System/Transform.inl"

#endif // PTO_SYSTEM_TRANSFORM_HPP
