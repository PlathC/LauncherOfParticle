#include "pto/Renderer/Sky.hpp"

namespace pto
{
    inline const vzt::ImageView& Sky::getSkyImageView() const { return m_view; }
    inline const vzt::ImageView& Sky::getSkySamplingImageView() const { return m_samplingView; }
    inline const vzt::Sampler&   Sky::getSampler() const { return m_sampler; }
} // namespace pto
