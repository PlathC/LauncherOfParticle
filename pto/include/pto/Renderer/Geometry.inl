#include "pto/Renderer/Geometry.hpp"

namespace pto
{
    inline const vzt::AccelerationStructure& GeometryHolder::getAccelerationStructure() const
    {
        return accelerationStructure;
    }

    inline const vzt::AccelerationStructure& GeometryHandler::getAccelerationStructure() const
    {
        return m_accelerationStructure;
    }

    inline const vzt::Buffer& GeometryHandler::getDescriptions() const { return m_objectDescriptionBuffer; }

} // namespace pto
