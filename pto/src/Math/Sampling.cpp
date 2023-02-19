#include "pto/Math/Sampling.hpp"

#include <cassert>

namespace pto
{
    float getCumulativeDistributionFunctions(vzt::CSpan<float> data, vzt::Span<float> cdf)
    {
        assert(cdf.size >= data.size + 2 &&
               "CDFs must be able to contain at least data.size + 2 elements to store the exclusive sum and the result "
               "of the sum");

        const float weight = static_cast<float>(data.size);
        cdf[0]             = 0.f;
        for (std::size_t i = 1; i < data.size + 1; i++)
            cdf[i] = cdf[i - 1] + data[i - 1] / weight;

        const float sum    = cdf[data.size];
        cdf[data.size + 1] = sum;
        for (int i = 1; i < data.size + 1; ++i)
            cdf[i] /= sum;

        return sum;
    }

    std::vector<float> getCumulativeDistributionFunctions(const Image<float>& pixels)
    {
        // Store the <image.width> lines' cdf + the cdf of all lines as a single column
        std::vector<float> cdfs{};
        cdfs.resize((pixels.width + 2) * pixels.height + (pixels.height + 2ul));

        std::vector<float> sums{};
        sums.resize(pixels.height);
        for (std::size_t j = 0; j < pixels.height; j++)
        {
            getCumulativeDistributionFunctions(vzt::CSpan<float>(pixels.data.data() + pixels.width * j, pixels.width),
                                               vzt::Span(cdfs, j * (pixels.width + 2ul)));
            sums[j] = cdfs[(j + 1) * (pixels.width + 2ul)];
        }

        getCumulativeDistributionFunctions(sums, vzt::Span(cdfs, pixels.height * (pixels.width + 2ul)));

        return cdfs;
    }
} // namespace pto
