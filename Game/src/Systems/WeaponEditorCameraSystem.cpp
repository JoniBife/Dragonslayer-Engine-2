#include "Game/Systems/WeaponEditorCameraSystem.hpp"

#include <Core/EngineGlobals.hpp>
#include <Core/Time.hpp>
#include <Math/MathAux.hpp>
#include <Physics/Components/DynamicRigidBody.hpp>
#include <Runtime/Input.hpp>
#include <algorithm> // For clamp TODO Add clamp to math functions

#include "Game/Components/GameState.hpp"
#include "Game/Components/WeaponEditor.hpp"
#include "Game/Components/WeaponEditorCamera.hpp"
#include "Physics/Components/StaticRigidBody.hpp"

constexpr float ROTATION_SPEED = 30.f;
constexpr float ZOOM_SPEED = 15.f;
const Vec3 FOLLOW_OFFSET = { 0.f, 5.f, 10.f };

void WeaponEditorCameraSystem::Start(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }
    const Entity weaponEditorCameraEntity = vault.CreateEntity("Weapon Editor Camera", { "WeaponEditor" });
    vault.EmplaceComponent<WeaponEditorCamera>(weaponEditorCameraEntity);
}

static Vec2 GetInputRotation() {

    if (gTime.paused) {
        return {0.f, 0.f};
    }

	// TODO Read gamepad input

    if (Input::IsMouseButtonDown(MouseButtonCode::Right)) {
        return Input::GetCursorDelta();
    }

    return {0.f, 0.f};
}

static float GetInputZoom() {

	const float mouseScroll = Input::GetMouseScroll();

	// TODO Read gamepad input

	return mouseScroll;
}

void WeaponEditorCameraSystem::Update(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }

    auto& [gameState, gameStateEntity] = *vault.GetComponents<GameState>().begin();
    if (!gameState.isBuildingWeapon) {
        return;
    }

    auto& [camera, weaponEditorCameraEntity] = *vault.GetComponents<WeaponEditorCamera>().begin();

    if (gameState.updatedIsBuildingWeapon && gameState.isBuildingWeapon) {
        gCamera = &camera.camera;
        Input::SetCursorVisibility(true);
    }

    // Gather input
    Vec2 requestedRotation = GetInputRotation();
    float requestedZoom = GetInputZoom();

    // Update tracked rotation and zoom
    camera.rotation += requestedRotation * ROTATION_SPEED * gTime.deltaTime;
    camera.rotation.y = std::clamp(camera.rotation.y, -80.f, 50.f);
    camera.rotation.x = std::fmod(camera.rotation.x, 360.f);
    camera.zoom = std::clamp(camera.zoom - requestedZoom * ZOOM_SPEED * gTime.deltaTime, 0.2f, 2.f);

    // Compute new camera location and orientation based on the rotation and zoom
    const Vec3 yawAxis = Vec3::UP;
    const Vec3 pitchAxis = Vec3::RIGHT;

    float playerYaw = DegreesToRadians(-camera.rotation.x);
    const Quat cameraYawRotation = Quat(playerYaw, yawAxis);
    const Quat cameraPitchRotation = Quat(DegreesToRadians(-camera.rotation.y), pitchAxis);

    const Vec3 newCameraLocation = WEAPON_EDITOR_LOCATION + (cameraYawRotation * cameraPitchRotation).ToRotationMatrix() * (FOLLOW_OFFSET * camera.zoom);

    // Set new camera location and orientation
    camera.camera.SetPosition(newCameraLocation);
    camera.camera.SetTarget(WEAPON_EDITOR_LOCATION);
}
