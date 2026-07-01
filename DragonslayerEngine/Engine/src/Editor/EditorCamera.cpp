#include "Editor/EditorCamera.hpp"
#include "Math/MathAux.hpp"
#include "Math/Vec2.hpp"
#include <algorithm>

#include "Runtime/Input.hpp"

bool EditorCamera::FreeMovement(float elapsedTime, const Vec2& currMousePosition)
{
    bool performedAction = false;

    Input::SetCursorVisibility(false);

    Vec2 mouseMovement = (currMousePosition - lastMousePosition);

    if (mouseMovement.Magnitude() > 0.1f) {

        yaw += mouseMovement.x * elapsedTime * rotationSpeed;

        // Minus because the origin of the coordinates is at the top left corner
        pitch -= mouseMovement.y * elapsedTime * rotationSpeed;

        // Avoiding float overflow by returning to degrees after a full rotation
        yaw = std::fmod(yaw, 360.0f);

        // Cannot move camera above or below origin
        pitch = std::clamp(pitch, -89.0f, 89.0f);

        // Spherical to Cartesian conversion
        Vec3 newForward;
        newForward.x = cosf(DegreesToRadians(yaw)) * cosf(DegreesToRadians(pitch));
        newForward.y = sinf(DegreesToRadians(pitch));
        newForward.z = cosf(DegreesToRadians(pitch)) * sinf(DegreesToRadians(yaw));
        camera.SetFront(newForward);

        performedAction = true;
    }

    Vec3 positionUpdate = Vec3::ZERO;

    const Vec3 forward = camera.GetForward();
    const Vec3 right = camera.GetRight();

    if (Input::IsKeyHeld(KeyCode::Up) || Input::IsKeyHeld(KeyCode::W)) {
        positionUpdate += forward;
    }
    else if (Input::IsKeyHeld(KeyCode::Down) || Input::IsKeyHeld(KeyCode::S)) {
        positionUpdate -= forward;
    }

    if (Input::IsKeyHeld(KeyCode::Left) || Input::IsKeyHeld(KeyCode::A)) {
        positionUpdate += -right;
    }
    else if (Input::IsKeyHeld(KeyCode::Right) || Input::IsKeyHeld(KeyCode::D)) {
        positionUpdate = right;
    }

    const float scroll = Input::GetMouseScroll();
    movementSpeed += scroll*movementSpeedScrollIncrease; // If we scroll while in free movement mode we increase/decrease the speed
    movementSpeed = std::max(1.f, movementSpeed);

    // Only update position if there was any change to positionUpdate
    if (positionUpdate.MagnitudeSquared() > 0) {
        camera.SetPosition(camera.GetPosition() + positionUpdate.Normalize() * movementSpeed * elapsedTime);
        performedAction = true;
    }

    return performedAction;
}

bool EditorCamera::Drag(float elapsedTime, const Vec2& currMousePosition)
{

    bool performedAction = false;

    if (startingDragPosition.x == 0.0f && startingDragPosition.y == 0.0f) {
        startingDragPosition = currMousePosition;
    }

    Vec2 mouseMovement = (currMousePosition - lastMousePosition);
    mouseMovement.y *= -1;

    if (mouseMovement.Magnitude() > 0.0f) {

        const Vec3 forward = camera.GetForward();
        const Vec3 up = camera.GetUp();
        Vec3 cameraRight = Cross(forward, up).Normalize();
        Vec3 cameraUp = Cross(cameraRight, forward).Normalize();

        // TODO Improve the way the cursor follows the drag motion, currently
        // if the drag is too quick the cursor will not follow correctly

        camera.SetPosition(camera.GetPosition() - cameraRight * mouseMovement.x * elapsedTime * dragSpeed
                                                 - cameraUp * mouseMovement.y * elapsedTime * dragSpeed);
        performedAction = true;
    }

    return performedAction;
}

bool EditorCamera::Zoom(float elapsedTime, float scroll)
{
    if (scroll != 0.0f) {

        camera.SetPosition(camera.GetPosition() + camera.GetForward().Normalize() * scroll * elapsedTime * zoomSpeed);

        return true;
    }

    return false;
}

void EditorCamera::SetMovementSpeed(float movementSpeed)
{
    ASSERT(movementSpeed > 0.0f && movementSpeed < 1000.0f);
    this->movementSpeed = movementSpeed;
}

void EditorCamera::SetRotationSpeed(float rotationSpeed)
{
    ASSERT(rotationSpeed > 0.0f && rotationSpeed < 180.0f);
    this->rotationSpeed = rotationSpeed;
}

void EditorCamera::SetDragSpeed(float dragSpeed)
{
    ASSERT(dragSpeed > 0.0f && dragSpeed < 50.0f);
    this->dragSpeed = dragSpeed;
}

void EditorCamera::SetZoomSpeed(float zoomSpeed)
{
    ASSERT(zoomSpeed > 0.0f && zoomSpeed < 1000.0f);
    this->zoomSpeed = zoomSpeed;
}

void EditorCamera::Move(float elapsedTime)
{
    Vec2 currMousePosition = Input::GetCursorPosition();

    if (Input::IsMouseButtonDown(MouseButtonCode::Right)) {

        freeModeEnabled = true;
        FreeMovement(elapsedTime, currMousePosition);

    } else if (Input::IsMouseButtonDown(MouseButtonCode::Middle)) {

        dragModeEnabled = true;
        Drag(elapsedTime, currMousePosition);

    } else if (const float scroll = Input::GetMouseScroll(); scroll != 0.f) {

        Zoom(elapsedTime, scroll);

    } else if (freeModeEnabled || dragModeEnabled) {

        Input::SetCursorVisibility(true);

        if (dragModeEnabled) {
            Input::SetCursorPosition(currMousePosition);
            startingDragPosition = { 0,0 };
        }

        freeModeEnabled = false;
        dragModeEnabled = false;
    }

    lastMousePosition = currMousePosition;

    camera.Update();
}

float EditorCamera::GetMovementSpeed() const
{
    return movementSpeed;
}

float EditorCamera::GetRotationSpeed() const
{
    return rotationSpeed;
}

float EditorCamera::GetDragSpeed() const
{
    return dragSpeed;
}

float EditorCamera::GetZoomSpeed() const
{
    return zoomSpeed;
}

bool EditorCamera::IsFreeModeEnabled() const
{
    return freeModeEnabled;
}

bool EditorCamera::IsDragModeEnabled() const
{
    return dragModeEnabled;
}

void EditorCamera::Assign(const Camera& other) {
    camera.Assign(other);
    const Vec3 forward = camera.GetForward();
    this->pitch = RadiansToDegrees(std::clamp(asinf(forward.y), -1.f, 1.f));
    this->yaw = RadiansToDegrees(std::atan2(forward.z, forward.x));
}

void EditorCamera::SetTarget(const Vec3 &target) {
    camera.SetTarget(target);
    const Vec3 forward = camera.GetForward();
    this->pitch = RadiansToDegrees(std::clamp(asinf(forward.y), -1.f, 1.f));
    this->yaw = RadiansToDegrees(std::atan2(forward.z, forward.x));
}

void EditorCamera::SetFront(const Vec3 &front) {
    camera.SetFront(front);
    this->pitch = RadiansToDegrees(std::clamp(asinf(front.y), -1.f, 1.f));
    this->yaw = RadiansToDegrees(std::atan2(front.z, front.x));
}

void EditorCamera::SetPosition(const Vec3& position) {
    camera.SetPosition(position);
}
