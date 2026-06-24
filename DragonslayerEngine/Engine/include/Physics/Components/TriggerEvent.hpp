#pragma once

#include <PxActor.h>
#include "ECS/Component.hpp"

using namespace physx;

struct TriggerEvent final {
    PxActor* triggerActor;
    PxActor* actorInside;
    bool enteredTrigger;
};

ENGINE_DSTRUCT(TriggerEvents) final {

    GENERATE_DSTRUCT_BODY(TriggerEvents)

    Array<TriggerEvent, FreeListAllocator> events = { 32, gGameAllocator };

}; END_DSTRUCT(TriggerEvents)
