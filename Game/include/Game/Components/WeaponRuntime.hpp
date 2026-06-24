#pragma once

#include <ECS/Component.hpp>

#include "Game/StaticGameSettings.hpp"
#include "Game/ModulesAndHandles.hpp"

DSTRUCT(WeaponRuntime, .numComponents = 1, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DSTRUCT_BODY(WeaponRuntime)

    InlineArray<Entity, MAX_MODULES> moduleEntities;
    Entity handleEntity = ECS::InvalidEntity;
    HandleType handleType = HandleType::ShootV1;

#if WITH_EDITOR
    void OnHierarchy() {

    }
#endif

}; END_DSTRUCT(WeaponRuntime)
