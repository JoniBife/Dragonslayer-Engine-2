#pragma once

#include <ECS/Component.hpp>

DSTRUCT(GameState, .numComponents = 1, .isSerializable = true, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DSTRUCT_BODY(GameState)

    bool updatedIsBuildingWeapon = false;
    bool isBuildingWeapon = false;

#if WITH_EDITOR
    void OnHierarchy() {
        if (ImGui::Checkbox("Is Building Weapon", &isBuildingWeapon)) {
            updatedIsBuildingWeapon = true;
        }
    }
#endif

}; END_DSTRUCT(GameState)
