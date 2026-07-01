#pragma once

#include "Math/Mat4.hpp"
#include "Math/Vec3.hpp"

/*
 * TODO Update this camera class, this is extremely outdated! Function names etc..
 */
class ENGINE_API Camera final {

protected:
	Mat4 view;
	Mat4 projection; // TODO Create a projection class

	Vec3 position = { 0.0f, 3.0f, 10.0f }; // eye
	Vec3 forward = { 0.0f, 0.0f, -1.0f};
	Vec3 up = { 0.0f, 1.0f, 0.0f }; // up
	Vec3 right = {-1.0f, 0.0f, 0.0f};

	float viewportWidth = 1366.f;
	float viewportHeight = 768.f;

	float zNear = 0.01f;
	float zFar = 1000.0f;

	float fov = 60.0f;

	bool wasDirtyRecently = false; // Indicates whether the camera was updated recently
	bool dirty = true; // Indicates whether a setter was called

public:
	Camera() { Update(); }
	~Camera() = default;

	void Update();

    void Assign(const Camera& camera);
	void SetPosition(const Vec3& position);
	void SetTarget(const Vec3& target);
	void SetUp(const Vec3& up);
	void SetFront(const Vec3& front);
	void SetNearPlane(float nearPlane);
	void SetFarPlane(float farPlane);
	void SetFov(float fov);
	void SetViewportSize(float viewportWidth, float viewportHeight);

	NO_DISCARD Vec3 GetPosition() const;
	NO_DISCARD Vec3 GetForward() const;
	NO_DISCARD Vec3 GetUp() const;
	NO_DISCARD Vec3 GetRight() const;
	NO_DISCARD float GetNearPlane() const;
	NO_DISCARD float GetFarPlane() const;
	NO_DISCARD float GetFov() const;
	NO_DISCARD Mat4 GetView() const;
	NO_DISCARD Mat4 GetProjection() const;
	NO_DISCARD float GetViewportWidth() const;
	NO_DISCARD float GetViewportHeight() const;
	NO_DISCARD Vec2 GetViewportSize() const;
	NO_DISCARD bool WasDirty() const; // Checks if the camera was dirty recently
	NO_DISCARD float GetAspectRatio() const;
	NO_DISCARD Quat GetRotation() const;

	NO_DISCARD Vec3 DirectionTowardsScreen(const Vec2& screenPos) const;
};
