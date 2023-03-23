#ifndef LOP_SYSTEM_TRANSFORM_HPP
#define LOP_SYSTEM_TRANSFORM_HPP

#include <vzt/Core/Math.hpp>

namespace lop
{
    struct Transform
    {
        static constexpr vzt::Vec3 Up    = {0.f, 0.f, 1.f};
        static constexpr vzt::Vec3 Front = {1.f, 0.f, 0.f};
        static constexpr vzt::Vec3 Right = {0.f, -1.f, 0.f};

        vzt::Vec3 position{};
        vzt::Quat rotation{1.f, 0.f, 0.f, 0.f};

        inline void rotate(float angle, const vzt::Vec3& axis);
        inline void rotate(const vzt::Vec3 angles);
        inline void translate(const vzt::Vec3& t);

        void lookAt(const vzt::Vec3& target);
    };
} // namespace lop

#include "lop/System/Transform.inl"

#endif // LOP_SYSTEM_TRANSFORM_HPP
