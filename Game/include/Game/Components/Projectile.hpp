#pragma once

#include <ECS/Component.hpp>

DSTRUCT(Projectile) final {

    GENERATE_DSTRUCT_BODY(Projectile)

    float destroyAge = 2.f;
    float age = 0.f;

}; END_DSTRUCT(Projectile)
