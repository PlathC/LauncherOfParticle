#include "lop/Renderer/Geometry.hpp"

namespace lop
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
} // namespace lop
