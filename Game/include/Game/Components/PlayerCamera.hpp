#pragma once

#include <ECS/System.hpp>
#include <Renderer/Camera.hpp>

DCLASS(PlayerCamera, .numComponents = 1, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DCLASS_BODY(PlayerCamera)

public:
    Camera camera;
	Vec2 rotation = Vec2(0.f); // x = pitch, y = yaw
	float zoom = 1.f;
	float minFov = 60.f;
	float maxFov = 120.f;
	float fovChangeSmoothness = 0.05f;
	float maxSpeed = 100.f;
	float minSpeed = 60.f;
    bool directionFromPlayer = false;

#if WITH_EDITOR
    void OnHierarchy() {
        ImGui::InputFloat("Min FOV: ", &minFov);
        ImGui::InputFloat("Max FOV: ", &maxFov);
        ImGui::SliderFloat("FOV Change Smoothness: ", &fovChangeSmoothness, 0.000001f, 1.f, "%.5f");
        ImGui::TextDisabled("Current FOV: %f", camera.GetFov());
        ImGui::Checkbox("Direction From Player", &directionFromPlayer);
    }
#endif
}; END_DCLASS(PlayerCamera)