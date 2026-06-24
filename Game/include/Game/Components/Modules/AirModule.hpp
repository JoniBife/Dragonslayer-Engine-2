#pragma once

#include <Core/Allocators/FreeListAllocator.hpp>
#include <ECS/Component.hpp>

#include "Game/StaticGameSettings.hpp"

/**
 * AirModules are separated between visuals and trigger (actual module)
 * Because the transform of the trigger volume is much larger than visuals
 */

DSTRUCT(AirModuleVisuals, .numComponents = MAX_MODULES, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DSTRUCT_BODY(AirModuleVisuals)

    Entity triggerEntity = ECS::InvalidEntity;

#if WITH_EDITOR
    void OnHierarchy() {
    }
#endif

}; END_DSTRUCT(AirModuleVisuals)

DSTRUCT(AirModule, .numComponents = MAX_MODULES, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DSTRUCT_BODY(AirModule)

    Array<Entity, FreeListAllocator> entitiesWithinAirVolume = {64, gGameAllocator };
    Vec3 size = {5.f, 15.f, 5.f};
    float force = 100.f;

#if WITH_EDITOR
    void OnHierarchy() {
        ImGui::InputVec3("Size", size);
        ImGui::InputFloat("Force", &force);
    }
#endif

}; END_DSTRUCT(AirModule)
