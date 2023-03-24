#include "lop/System/Transform.hpp"

namespace lop
{
    inline glm::mat4 Transform::get() const
    {
        glm::mat4 transform = glm::mat4_cast(rotation);
        transform[3]        = glm::vec4(position, 1.f);
        return transform;
    }

    inline void Transform::rotate(float angle, const vzt::Vec3& axis)
    {
        rotation *= glm::angleAxis(angle, axis);
        rotation = glm::normalize(rotation);
    }

    inline void Transform::rotate(const vzt::Vec3 angles) { rotation *= glm::quat(angles); }

    inline void Transform::translate(const vzt::Vec3& t) { position += t; }
} // namespace lop
