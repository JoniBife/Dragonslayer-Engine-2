#pragma once

#include <PxActor.h>
#include "ECS/Component.hpp"

using namespace physx;

struct CollisionEvent final {
    PxActor* actorA;
    PxActor* actorB;
};

ENGINE_DSTRUCT(CollisionEvents) final {

    GENERATE_DSTRUCT_BODY(CollisionEvents)

    Array<CollisionEvent, FreeListAllocator> events = { 32, gGameAllocator };

}; END_DSTRUCT(CollisionEvents)