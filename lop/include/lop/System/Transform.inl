#include "lop/System/Transform.hpp"

namespace lop
{
    inline void Transform::rotate(float angle, const vzt::Vec3& axis)
    {
        rotation *= glm::angleAxis(angle, axis);
        rotation = glm::normalize(rotation);
    }

    inline void Transform::rotate(const vzt::Vec3 angles) { rotation *= glm::quat(angles); }

    inline void Transform::translate(const vzt::Vec3& t) { position += t; }
} // namespace lop
