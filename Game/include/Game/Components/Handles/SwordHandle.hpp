#pragma once

#include <ECS/Component.hpp>

DSTRUCT(SwordHandle, .numComponents = 1, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DSTRUCT_BODY(SwordHandle)

    float swingTime = 0.f;
    float swingDuration = 0.25f;
    float activationPoint = 0.5f;
    bool isSwinging = false;
    bool activatedThisSwing = false;

#if WITH_EDITOR
    void OnHierarchy(){
        ImGui::InputFloat("Swing Duration (s)", &swingDuration);
    }
#endif

}; END_DSTRUCT(SwordHandle)
