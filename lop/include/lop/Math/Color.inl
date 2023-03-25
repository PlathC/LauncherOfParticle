#include "lop/Math/Color.hpp"

namespace lop
{
    constexpr inline float getLuminance(const vzt::Vec3 x) { return (0.2126f * x.r + 0.7152f * x.g + 0.0722f * x.b); }

    namespace tonemap
    {
        // Reference: https://www.shadertoy.com/view/WdjSW3
        constexpr inline vzt::Vec3 aces(const vzt::Vec3 x)
        {
            // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
            const float a = 2.51;
            const float b = 0.03;
            const float c = 2.43;
            const float d = 0.59;
            const float e = 0.14;
            return (x * (a * x + b)) / (x * (c * x + d) + e);
        }
    } // namespace tonemap
} // namespace lop
