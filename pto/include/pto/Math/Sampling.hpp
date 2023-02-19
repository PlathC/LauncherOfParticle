#ifndef PTO_MATH_SAMPLING_HPP
#define PTO_MATH_SAMPLING_HPP

#include <vzt/Core/Type.hpp>
#include <vzt/Data/Image.hpp>

namespace pto
{
    float              getCumulativeDistributionFunctions(vzt::CSpan<float> data, vzt::Span<float> cdf);
    std::vector<float> getCumulativeDistributionFunctions(const Image<float>& pixels);
} // namespace pto

#endif // PTO_MATH_SAMPLING_HPP
