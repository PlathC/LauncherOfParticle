#ifndef LOP_MATH_SAMPLING_HPP
#define LOP_MATH_SAMPLING_HPP

#include <vzt/Core/Type.hpp>
#include <vzt/Data/Image.hpp>

namespace lop
{
    float              getCumulativeDistributionFunctions(vzt::CSpan<float> data, vzt::Span<float> cdf);
    std::vector<float> getCumulativeDistributionFunctions(const Image<float>& pixels);
} // namespace lop

#endif // LOP_MATH_SAMPLING_HPP
