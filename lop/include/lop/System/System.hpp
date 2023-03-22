#ifndef LOP_SYSTEM_SYSTEM_HPP
#define LOP_SYSTEM_SYSTEM_HPP

#include <entt/entt.hpp>

namespace lop
{
    struct Name
    {
        std::string value;
    };

    struct System
    {
        entt::registry registry;
        entt::handle   create() { return entt::handle{registry, registry.create()}; }
    };
} // namespace lop

#endif // LOP_SYSTEM_SYSTEM_HPP
