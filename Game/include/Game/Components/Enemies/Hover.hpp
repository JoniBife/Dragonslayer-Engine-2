#pragma once

#include <ECS/Component.hpp>

DSTRUCT(Hover, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DSTRUCT_BODY(Hover)

    float maxSpeed = 8.0f;
    float acceleration = 1.f;
    float hoverHeight = 3.f;
    float springConstant = 30.0f;  // How hard it pushes back up
    float dampingConstant = 15.0f;  // How much it resists bouncing
    float shootRange = 50.f;
    float shootRate = 15.f;
    float nextShootTime = 0.f;
    float changeHoverHeightRate = 2.f;
    float nextChangeHoverHeight = 0.f;

#if WITH_EDITOR
    void OnHierarchy() {
        ImGui::InputFloat("Max Speed", &maxSpeed);
        ImGui::InputFloat("Acceleration", &acceleration);
        ImGui::InputFloat("Hover Height", &hoverHeight);
        ImGui::InputFloat("Spring", &springConstant);
        ImGui::InputFloat("Dampening", &dampingConstant);
    }
#endif

}; END_DSTRUCT(Hover)
