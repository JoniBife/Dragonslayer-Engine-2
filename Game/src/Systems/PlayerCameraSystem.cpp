#include "Game/Systems/PlayerCameraSystem.hpp"

#include <Core/EngineGlobals.hpp>
#include <Core/Time.hpp>

#include <Math/MathAux.hpp>
#include <Physics/Components/CharacterController.hpp>
#include <Physics/Components/DynamicRigidBody.hpp>
#include <Renderer/Components/PrimitiveRenderer.hpp>
#include <Runtime/Components/Transform.hpp>
#include <Runtime/Input.hpp>
#include <algorithm> // For clamp TODO Add clamp to math functions

#include "Game/Components/GameState.hpp"
#include "Game/Components/Player.hpp"
#include "Game/Components/PlayerCamera.hpp"

constexpr float ROTATION_SPEED = 10.f;
constexpr float ZOOM_SPEED = 15.f;
const Vec3 FOLLOW_OFFSET = { 0.f, 5.f, 30.f };

void PlayerCameraSystem::Start(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }

	Entity playerCameraEntity = vault.CreateEntity("Player Camera", {"Player"});

	PlayerCamera& playerCamera = vault.EmplaceComponent<PlayerCamera>(playerCameraEntity);

	Input::SetCursorVisibility(false);

    gCamera = &playerCamera.camera;
}

static Vec2 GetInputRotation() {

    if (gTime.paused) {
        return {0.f, 0.f};
    }

	// TODO Read gamepad input

	return Input::GetCursorDelta();
}

static float GetInputZoom() {

	const float mouseScroll = Input::GetMouseScroll();

	// TODO Read gamepad input

	return mouseScroll;
}

void PlayerCameraSystem::Update(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }

    auto& [gameState, gameStateEntity] = *vault.GetComponents<GameState>().begin();

    if (gameState.isBuildingWeapon) {
        return;
    }

    auto& [player, playerEntity] = *vault.GetComponents<Player>().begin();
	auto& [camera, cameraEntity] = *vault.GetComponents<PlayerCamera>().begin();
	const Transform& playerTransform = vault.GetComponent<Transform>(playerEntity);
	const CharacterController& playerController = vault.GetComponent<CharacterController>(playerEntity);

    if (gameState.updatedIsBuildingWeapon && !gameState.isBuildingWeapon) {
        gCamera = &camera.camera;
        Input::SetCursorVisibility(false);
    }

    if (Input::IsKeyPressed(KeyCode::Q)) {
        camera.directionFromPlayer = !camera.directionFromPlayer;
    }

	// We follow the time interpolated position and rotation of the player since it is moved via the physics system
	const Vec3 interpolatedPosition = playerController.GetInterpolatedPosition(playerTransform.location);
    const Quat interpolatedRotation = playerController.GetInterpolatedRotation(playerTransform.rotation);

	// Gather input
	Vec2 requestedRotation = GetInputRotation();
	float requestedZoom = GetInputZoom();

	// Update tracked rotation and zoom
	camera.rotation += requestedRotation * ROTATION_SPEED * gTime.deltaTime;
	camera.rotation.y = std::clamp(camera.rotation.y, -50.f, 50.f);
    camera.rotation.x = std::fmod(camera.rotation.x, 360.f);
	camera.zoom = std::clamp(camera.zoom - requestedZoom * ZOOM_SPEED * gTime.deltaTime, 0.2f, 2.f);

	// Compute new camera location and orientation based on the rotation and zoom
	const Vec3 yawAxis = Vec3::UP;
	const Vec3 pitchAxis = Vec3::RIGHT;

    float playerYaw = DegreesToRadians(-camera.rotation.x);
    if (camera.directionFromPlayer) {
        const Vec3 playerForward = interpolatedRotation.ToRotationMatrix() * -Vec3::FORWARD;
        playerYaw = std::atan2(playerForward.x, playerForward.z);
    }

	const Quat cameraYawRotation = Quat(playerYaw, yawAxis);
	const Quat cameraPitchRotation = Quat(DegreesToRadians(-camera.rotation.y), pitchAxis);

	const Vec3 newCameraLocation = interpolatedPosition + (cameraYawRotation * cameraPitchRotation).ToRotationMatrix() * (FOLLOW_OFFSET * camera.zoom);

	// Set new camera location and orientation
	camera.camera.SetPosition(newCameraLocation);
	camera.camera.SetTarget(interpolatedPosition + Vec3(0.f, 6.f, 0.f));
}
