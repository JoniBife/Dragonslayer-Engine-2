#pragma once

#include <ECS/Component.hpp>
#include <Math/Quat.hpp>

#include "Game/StaticGameSettings.hpp"

DSTRUCT(FollowEntity, .numComponents = MAX_MODULES, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DSTRUCT_BODY(FollowEntity)

    Vec3 offsetLocation = Vec3::ZERO;
    Quat offsetRotation = Quat::IDENTITY;
    Entity target = ECS::InvalidEntity;

#if WITH_EDITOR
    void OnHierarchy() {
        ImGui::InputVec3("Offset Location", offsetLocation);
        ImGui::InputQuat("Offset Rotation", offsetRotation);
        ImGui::Text("Entity to follow: %d", target);
    }
#endif

}; END_DSTRUCT(FollowEntity)
