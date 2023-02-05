#include "pto/Renderer/Sky.hpp"

namespace pto
{
    inline const vzt::ImageView& Sky::getImageView() const { return m_view; }
    inline const vzt::Sampler&   Sky::getSampler() const { return m_sampler; }
} // namespace pto
