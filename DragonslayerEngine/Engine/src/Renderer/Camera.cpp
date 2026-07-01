#include "Renderer/Camera.hpp"

#include <cmath>

#include "Core/Assert.hpp"
#include "Math/Mat3.hpp"
#include "Math/MathAux.hpp"
#include "Math/Quat.hpp"
#include "Math/Vec2.hpp"
#include "Math/Vec4.hpp"

void Camera::Update() {
    if (dirty) {
        dirty = false;
        wasDirtyRecently = true;

        if (std::fabs(Dot(forward, Vec3::Y)) < .999999f) {
            right = Cross(forward, Vec3::Y).Normalize();
        } else {
            right = Vec3::X * (std::signbit(forward.y) ? -1.f : 1.f);
        }

        up = Cross(right, forward).Normalize();

        // front = target - position;
        view = Mat4::LookAt(position, position + forward, up);
        // projection = ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 30.0f);

        // TODO We don't need to update this every frame while moving the camera only once!
        projection = Mat4::Perspective(DegreesToRadians(fov), viewportWidth / viewportHeight, zNear, zFar);
    } else {
        wasDirtyRecently = false;
    }
}
void Camera::Assign(const Camera& camera) {
    this->view = camera.view;
    this->projection = camera.projection;
    this->position = camera.position;
    this->forward = camera.forward;
    this->up = camera.up;
    this->right = camera.right;
    this->viewportWidth = camera.viewportWidth;
    this->viewportHeight = camera.viewportHeight;
    this->wasDirtyRecently = camera.wasDirtyRecently;
    this->dirty = camera.dirty;
}

void Camera::SetPosition(const Vec3& position)
{
	dirty = true;
	this->position = position;
}

void Camera::SetTarget(const Vec3& target)
{
	dirty = true;
	this->forward = (target - position).Normalize();
}

void Camera::SetUp(const Vec3& up)
{
	dirty = true;
	this->up = up;
}

void Camera::SetFront(const Vec3& front) {
	dirty = true;
	this->forward = front;
}

void Camera::SetNearPlane(float nearPlane)
{
	ASSERT(nearPlane > 0.01f);
	ASSERT(nearPlane < zFar);
	dirty = true;
	this->zNear = nearPlane;
}

void Camera::SetFarPlane(float farPlane)
{
	ASSERT(farPlane > 0);
	ASSERT(farPlane > zNear);
	dirty = true;
	this->zFar = farPlane;
}

void Camera::SetFov(float fov)
{
	// TODO Check range of fov
	ASSERT(fov > 0);
	dirty = true;
	this->fov = fov;
}

void Camera::SetViewportSize(float viewportWidth, float viewportHeight)
{
	ASSERT(viewportWidth > 0 && viewportHeight > 0);
    dirty = dirty || this->viewportWidth != viewportWidth || this->viewportHeight != viewportHeight;
	this->viewportWidth = viewportWidth;
	this->viewportHeight = viewportHeight;
}

Vec3 Camera::GetPosition() const
{
	return position;
}

Vec3 Camera::GetUp() const
{
	return up;
}

Vec3 Camera::GetRight() const
{
	return right;
}

float Camera::GetNearPlane() const
{
	return zNear;
}

float Camera::GetFarPlane() const
{
	return zFar;
}

float Camera::GetFov() const
{
	return fov;
}

Mat4 Camera::GetView() const
{
	return view;
}

Mat4 Camera::GetProjection() const
{
	return projection;
}

float Camera::GetViewportWidth() const
{
	return viewportWidth;
}

float Camera::GetViewportHeight() const
{
	return viewportHeight;
}

Vec2 Camera::GetViewportSize() const
{
	return {viewportWidth, viewportHeight};
}

bool Camera::WasDirty() const
{
	return wasDirtyRecently;
}

float Camera::GetAspectRatio() const
{
	return viewportWidth / viewportHeight;
}

Quat Camera::GetRotation() const {
    return Mat3(forward, right, up).ToQuaternion();
}

Vec3 Camera::GetForward() const
{
	return forward;
}

Vec3 Camera::DirectionTowardsScreen(const Vec2& screenPos) const
{
    const float ndcX = (screenPos.x / viewportWidth) * 2.0f - 1.0f;
    const float ndcY = 1.0f - (screenPos.y / viewportHeight) * 2.0f;

    const Mat4 vp = projection * view;
	Mat4 invVP; vp.Inverse(invVP);

    const Vec4 nearClip(ndcX, ndcY, -1.0f, 1.0f);
    const Vec4 farClip(ndcX, ndcY, 1.0f, 1.0f);

    const Vec4 nearWorld = invVP * nearClip;
    const Vec4 farWorld = invVP * farClip;

    const Vec3 nearPoint = Vec3(nearWorld.x, nearWorld.y, nearWorld.z) / nearWorld.w;
    const Vec3 farPoint = Vec3(farWorld.x, farWorld.y, farWorld.z) / farWorld.w;

	return (farPoint - nearPoint).Normalize();
}