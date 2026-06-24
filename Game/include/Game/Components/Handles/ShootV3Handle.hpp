#pragma once

#include <ECS/Component.hpp>

DSTRUCT(ShootV3Handle, .numComponents = 1, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DSTRUCT_BODY(ShootV3Handle)

    float animationDuration = .3f;

#if WITH_EDITOR
    void OnHierarchy(){
        ImGui::InputFloat("Animation Duration (s)", &animationDuration);
    }
#endif

}; END_DSTRUCT(ShootV3Handle)
