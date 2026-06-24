#pragma once

#include <ECS/System.hpp>
#include <Renderer/Camera.hpp>

DCLASS(WeaponEditorCamera, .numComponents = 1, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DCLASS_BODY(WeaponEditorCamera)

public:
    Camera camera;
	Vec2 rotation = Vec2(0.f); // x = pitch, y = yaw
	float zoom = 1.f;
	float minFov = 60.f;
	float maxFov = 120.f;
	float maxSpeed = 100.f;
	float minSpeed = 60.f;

#if WITH_EDITOR
    void OnHierarchy() {
        ImGui::InputFloat("Min FOV: ", &minFov);
        ImGui::InputFloat("Max FOV: ", &maxFov);
        ImGui::TextDisabled("Current FOV: %f", camera.GetFov());
    }
#endif
}; END_DCLASS(WeaponEditorCamera)