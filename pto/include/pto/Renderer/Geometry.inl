#include "pto/Renderer/Geometry.hpp"

namespace pto
{
    inline const vzt::AccelerationStructure& MeshHolder::getAccelerationStructure() const
    {
        return accelerationStructure;
    }

    inline const vzt::AccelerationStructure& MeshHandler::getAccelerationStructure() const
    {
        return m_accelerationStructure;
    }

    inline const vzt::Buffer& MeshHandler::getDescriptions() const { return m_objectDescriptionBuffer; }
} // namespace pto
