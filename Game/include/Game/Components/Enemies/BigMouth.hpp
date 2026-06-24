#pragma once

#include <ECS/Component.hpp>

DSTRUCT(BigMouth, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DSTRUCT_BODY(BigMouth)

    float maxSpeed = 13.0f;
    float acceleration = .5f;
    float attackRate = 2.f;
    float nextAttackTime = 0.f;

#if WITH_EDITOR
    void OnHierarchy() {
        ImGui::InputFloat("Max Speed", &maxSpeed);
        ImGui::InputFloat("Acceleration", &acceleration);
    }
#endif

}; END_DSTRUCT(BigMouth)
