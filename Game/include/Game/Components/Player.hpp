#pragma once

#include <ECS/Component.hpp>

DSTRUCT(Player, .numComponents = 1, .isSerializable = true, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DSTRUCT_BODY(Player)

    Entity leftHandEntity = ECS::InvalidEntity;
    Entity rightHandEntity = ECS::InvalidEntity;

    float movementSpeed = .35f;
    float jumpSpeed = 25.f;
    float gravityScale = 6.5f;

    float currentRotation = 0.f;

    /* Vertical velocity (m/s). Integrated each physics tick via gravity and jump input. */
    float verticalVelocity = 0.f;

    /* True when the controller is touching the ground (eCOLLISION_DOWN from last Move). */
    bool isGrounded = false;

#if WITH_EDITOR
    void OnHierarchy() {
        ImGui::SliderFloat("Movement Speed", &movementSpeed, 0.f, 1.f);
        ImGui::SliderFloat("Jump Speed", &jumpSpeed, 0.f, 100.f);
        ImGui::SliderFloat("Gravity Scale", &gravityScale, 0.f, 20.f);
        ImGui::Text("CurrentRotation: %f", currentRotation);
    }
#endif

}; END_DSTRUCT(Player)
