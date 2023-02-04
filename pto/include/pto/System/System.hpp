#ifndef PTO_SYSTEM_SYSTEM_HPP
#define PTO_SYSTEM_SYSTEM_HPP

#include <entt/entt.hpp>

namespace pto
{
    struct System
    {
        entt::registry registry;
        entt::handle   create() { return entt::handle{registry, registry.create()}; }
    };
} // namespace pto

#endif // PTO_SYSTEM_SYSTEM_HPP
