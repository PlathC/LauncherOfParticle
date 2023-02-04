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
} // namespace pto
