#pragma once

#include "ECS/Component.hpp"
#include "Renderer/Camera.hpp"
#include "Math/Vec2.hpp"

DCLASS(EditorCamera, .isDisplayable = true) final {

    GENERATE_DCLASS_BODY(EditorCamera)

private:
	float movementSpeed = 30.0f; // In units (meters) per second
	float rotationSpeed = 20.0f; // In degrees per second
	float movementSpeedScrollIncrease = 5.0f; // In units (meters) per second
	float dragSpeed = 2.0f;
	float zoomSpeed = 120.0f;
	float pitch = 0.0f;
	float yaw = -90.0f;

	Vec2 lastMousePosition = { 0.0f , 0.0f };
	Vec2 startingDragPosition = { 0.0f , 0.0f };
	bool freeModeEnabled = false;
	bool dragModeEnabled = false;

	// Editor camera actions (return true if there was action)
	bool FreeMovement(float elapsedTime, const Vec2& currMousePosition);
	bool Drag(float elapsedTime, const Vec2& currMousePosition);
	bool Zoom(float elapsedTime, float scroll);

public:
	Camera camera;

	EditorCamera() = default;

    void Assign(const Camera& other);

	void SetTarget(const Vec3& target);
	void SetFront(const Vec3& front);
	void SetPosition(const Vec3& position);

	void SetMovementSpeed(float movementSpeed);
	void SetRotationSpeed(float rotationSpeed);
	void SetDragSpeed(float dragSpeed);
	void SetZoomSpeed(float zoomSpeed);

	NO_DISCARD float GetMovementSpeed() const;
	NO_DISCARD float GetRotationSpeed() const;
	NO_DISCARD float GetDragSpeed() const;
	NO_DISCARD float GetZoomSpeed() const;
	NO_DISCARD bool IsFreeModeEnabled() const;
	NO_DISCARD bool IsDragModeEnabled() const;

	void Move(float elapsedTime);

#if WITH_EDITOR
	void OnHierarchy() {
	    constexpr float minMovementSpeed = 10.f;
	    constexpr float maxMovementSpeed = 1000.0f;
	    ImGui::SliderScalar("Movement Speed", ImGuiDataType_Float, &movementSpeed, &minMovementSpeed, &maxMovementSpeed);

	    constexpr float minRotationSpeed = 1.f;
	    constexpr float maxRotationSpeed = 180.0f;
	    ImGui::SliderScalar("Rotation Speed", ImGuiDataType_Float, &rotationSpeed, &minRotationSpeed, &maxRotationSpeed);

	    constexpr float minDragSpeed = 0.1f;
	    constexpr float maxDragSpeed = 50.0f;
	    ImGui::SliderScalar("Drag Speed", ImGuiDataType_Float, &dragSpeed, &minDragSpeed, &maxDragSpeed);

	    constexpr float minZoomSpeed = 1.f;
	    constexpr float maxZoomSpeed = 1000.0f;
	    ImGui::SliderScalar("Zoom Speed", ImGuiDataType_Float, &zoomSpeed, &minZoomSpeed, &maxZoomSpeed);
	}
#endif

}; END_DCLASS(EditorCamera)