#pragma once

#include <Core/Types.hpp>

namespace CollisionGroup {
    constexpr uint32 Player      = 1 << 0;
    constexpr uint32 Enemy       = 1 << 1;
    constexpr uint32 EnemySmall  = 1 << 2;
    constexpr uint32 Projectile  = 1 << 3;
    constexpr uint32 Weapon      = 1 << 4;
    constexpr uint32 Environment = 1 << 5;
    constexpr uint32 All         = 0xFFFFFFFF;
}
