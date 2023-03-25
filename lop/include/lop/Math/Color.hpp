#ifndef LOP_MATH_COLOR_HPP
#define LOP_MATH_COLOR_HPP

#include <vzt/Core/Math.hpp>

namespace lop
{
    constexpr inline float getLuminance(const vzt::Vec3 x);
    namespace tonemap
    {
        constexpr inline vzt::Vec3 aces(const vzt::Vec3 x);
    } // namespace tonemap
} // namespace lop

#include "lop/Math/Color.inl"

#endif // LOP_MATH_COLOR_HPP
