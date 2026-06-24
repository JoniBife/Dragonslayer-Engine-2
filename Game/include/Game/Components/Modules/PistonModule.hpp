#pragma once

#include <ECS/Component.hpp>

#include "Game/StaticGameSettings.hpp"

DSTRUCT(PistonModule, .numComponents = MAX_MODULES, .hasMetadata = true, .isDisplayable = true) final{

    GENERATE_DSTRUCT_BODY(PistonModule)

    Vec3 offsetLocation = Vec3::ZERO;
    Quat offsetRotation = Quat::IDENTITY;
    float pushDistance = 2.f;
    float pushDuration = .2f;
    float pushTime = 0.f;

#if WITH_EDITOR
    void OnHierarchy(){
        ImGui::InputFloat("Push Distance", &pushDistance);
        ImGui::InputFloat("Push Duration", &pushDuration);
        ImGui::BeginDisabled();
        ImGui::InputFloat("Current Push Time", &pushTime);
        ImGui::EndDisabled();
    }
#endif

}; END_DSTRUCT(PistonModule)
