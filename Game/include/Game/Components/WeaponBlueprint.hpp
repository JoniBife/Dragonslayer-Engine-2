#pragma once

#include <ECS/Component.hpp>
#include <Game/ModulesAndHandles.hpp>
#include "Game/StaticGameSettings.hpp"

DSTRUCT(WeaponBlueprint, .numComponents = 1, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DSTRUCT_BODY(WeaponBlueprint)

    InlineArray<ModuleSlot, MAX_MODULES> moduleSlots;
    uint32 handleType = 0; // 0 = melee

#if WITH_EDITOR
    void OnHierarchy() {
    }
#endif

}; END_DSTRUCT(WeaponBlueprint)
