#pragma once

#include <Core/Types.hpp>

using Entity = uint32; // All bits until last are the ID, the last represents if the entity is active

namespace ECS {
    static constexpr Entity InvalidEntity = 0;
    static constexpr Entity FirstEntity = 1;
}
