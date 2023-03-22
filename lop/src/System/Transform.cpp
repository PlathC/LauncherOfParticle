#include "lop/System/Transform.hpp"

#include <glm/gtc/quaternion.hpp>

namespace lop
{
    void Transform::lookAt(const vzt::Vec3& target)
    {
        // Based on https://stackoverflow.com/a/49824672

        vzt::Vec3   direction       = target - position;
        const float directionLength = glm::length(direction);

        // Check if the direction is valid; Also deals with NaN
        if (!(directionLength > 0.0001))
        {
            rotation = {1, 0, 0, 0};
            return;
        }

        direction /= directionLength;
        if (glm::abs(glm::dot(direction, Transform::Up)) > .9999f)
            rotation = glm::quatLookAt(direction, Transform::Right);
        else
            rotation = glm::quatLookAt(direction, Transform::Up);
    }
} // namespace lop
