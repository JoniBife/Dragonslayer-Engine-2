#include "Game/Systems/PlayerSystem.hpp"

#include <Core/Time.hpp>
#include <Math/MathAux.hpp>
#include <Physics/Components/CharacterController.hpp>
#include <Physics/Components/DynamicRigidBody.hpp>
#include <Renderer/Components/PrimitiveRenderer.hpp>
#include <Runtime/Components/Transform.hpp>
#include <Runtime/Input.hpp>

#include "Game/PhysicsCollisionGroups.hpp"
#include "Game/Components/Player.hpp"
#include "Game/Components/PlayerCamera.hpp"
#include "Game/Components/GameState.hpp"

void PlayerSystem::Start(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }

    const Entity playerEntity = vault.CreateEntity("Player", {"Player"});
    Player& player = vault.EmplaceComponent<Player>(playerEntity);

    Transform& transform = vault.EmplaceComponent<Transform>(playerEntity);
    transform.location = Vec3(0.f, 1.5f, 0.f);

    PrimitiveRenderer& renderer = vault.EmplaceComponent<PrimitiveRenderer>(playerEntity);
    renderer.primitiveType = PrimitiveType::Capsule;
    renderer.color = Vec3(0.f, 0.f, 1.f);

    CharacterController& characterController = vault.EmplaceComponent<CharacterController>(
        playerEntity,
        transform.location,
        1.f,
        .5f
    );
    characterController.SetCollisionGroup(CollisionGroup::Player, CollisionGroup::All);

    const Vec3 handOffset = Vec3(0.2f, 0.f, 0.f);
    // Left hand
    {
        const Entity leftHand = vault.CreateEntity("LeftHand", {"Player"});
        player.leftHandEntity = leftHand;

        Transform& handTransform = vault.EmplaceComponent<Transform>(leftHand);
        handTransform.location = transform.location + transform.scale.x*.5f + handOffset;
        handTransform.scale = Vec3(.3f);

        PrimitiveRenderer& handRenderer = vault.EmplaceComponent<PrimitiveRenderer>(leftHand);
        handRenderer.primitiveType = PrimitiveType::Sphere;
        handRenderer.color = Vec3(0.f, 0.f, 1.f);
    }

    // Left hand
    {
        const Entity rightHand = vault.CreateEntity("RightHand", {"Player"});
        player.rightHandEntity = rightHand;

        Transform& handTransform = vault.EmplaceComponent<Transform>(rightHand);
        handTransform.location = transform.location - transform.scale.x*.5f - handOffset;
        handTransform.scale = Vec3(.3f);

        PrimitiveRenderer& handRenderer = vault.EmplaceComponent<PrimitiveRenderer>(rightHand);
        handRenderer.primitiveType = PrimitiveType::Sphere;
        handRenderer.color = Vec3(0.f, 0.f, 1.f);
    }
}

static Vec2 GetInputMovement() {

    const bool isForwardPressed = Input::IsKeyDown(KeyCode::W);
    const bool isBackwardPressed = Input::IsKeyDown(KeyCode::S);
    const bool isRightPressed = Input::IsKeyDown(KeyCode::D);
    const bool isLeftPressed = Input::IsKeyDown(KeyCode::A);

    // TODO Gamepad input

    Vec2 movement;

    movement += Vec2::X * isForwardPressed;
    movement -= Vec2::X * isBackwardPressed;
    movement += Vec2::Y * isRightPressed;
    movement -= Vec2::Y * isLeftPressed;

    return movement;
}

static bool GetInputJump() {

    const bool isJumpPressed = Input::IsKeyPressed(KeyCode::Space);

    return isJumpPressed;
}

static float GetInputRotation() {

    if (gTime.paused) {
        return 0.f;
    }

    // TODO Read gamepad input

    return Input::GetCursorDelta().x;
}

static Vec2 inputMovement;
static bool inputJump;
static bool inputSwing;

void PlayerSystem::Update(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }

    auto& [gameState, gameStateEntity] = *vault.GetComponents<GameState>().begin();

    if (gameState.isBuildingWeapon) {
        return;
    }

    inputMovement = GetInputMovement();

    if (GetInputJump()) {
        inputJump = true;
    }

    // Capture swing input in Update (not PrePhysicsUpdate) to avoid missing presses
    if (Input::IsMouseButtonPressed(MouseButtonCode::Left)) {
        inputSwing = true;
    }
}

static Quat rotation = Quat::IDENTITY;

void PlayerSystem::PrePhysicsUpdate(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }

    auto& [gameState, gameStateEntity] = *vault.GetComponents<GameState>().begin();
    if (gameState.isBuildingWeapon) {
        return;
    }

    auto& [player, playerEntity] = *vault.GetComponents<Player>().begin();
    CharacterController& playerController = vault.GetComponent<CharacterController>(playerEntity);

    auto& [playerCamera, playerCameraEntity] = *vault.GetComponents<PlayerCamera>().begin();

    Quat characterRotation;
    Vec3 movementDirection;

    // Update the character rotation and its up vector to match the up vector set by the user settings
    if (playerCamera.directionFromPlayer)
    {
        constexpr float ROTATION_SPEED = 10.f;
        player.currentRotation += GetInputRotation() * ROTATION_SPEED * gTime.physicsDeltaTime;
        player.currentRotation = std::fmod(player.currentRotation, 360.f);
        characterRotation = Quat(-DegreesToRadians(player.currentRotation), Vec3::UP);
        const Vec3 forward = characterRotation.ToRotationMatrix() * Vec3::FORWARD;
        const Vec3 right = characterRotation.ToRotationMatrix() * Vec3::RIGHT;
        movementDirection = (forward * inputMovement.x - right * inputMovement.y).Normalize();
    }
    else // Calculate movement direction and player rotation
    {
        Vec3 cameraForward = playerCamera.camera.GetForward();
        Vec3 cameraRight = playerCamera.camera.GetRight();
        cameraForward = Vec3(cameraForward.x, 0.f, cameraForward.z).Normalize();
        cameraRight = Vec3(cameraRight.x, 0.f, cameraRight.z).Normalize();

        movementDirection = (cameraForward * inputMovement.x + cameraRight * inputMovement.y).Normalize();
        const float rotationAngle = std::fmod(std::atan2(cameraForward.x, cameraForward.z), 2.f * PI);
        characterRotation = Quat(rotationAngle, Vec3::UP);
    }

    rotation = characterRotation;

    // Integrate gravity into vertical velocity
    player.verticalVelocity += -9.81f * player.gravityScale * gTime.physicsDeltaTime;

    // Jump impulse: only when grounded, and consume the input so a single press doesn't
    // trigger multiple jumps if PrePhysicsUpdate runs more than once per Update.
    if (inputJump) {
        inputJump = false;
        if (player.isGrounded) {
            player.verticalVelocity = player.jumpSpeed;
        }
    }

    // Horizontal kept as displacement-per-tick to preserve existing feel; vertical is true m/s.
    Vec3 characterMovement = movementDirection * player.movementSpeed;
    characterMovement += Vec3::UP * player.verticalVelocity * gTime.physicsDeltaTime;

    const PxControllerCollisionFlags collisionFlags = playerController.Move(characterMovement, gTime.physicsDeltaTime);

    // Stick to ground when touching down: small negative velocity keeps the CC pressed onto
    // slopes/steps for reliable eCOLLISION_DOWN detection next tick.
    player.isGrounded = collisionFlags.isSet(PxControllerCollisionFlag::eCOLLISION_DOWN);
    if (player.isGrounded && player.verticalVelocity < 0.f) {
        player.verticalVelocity = -1.f;
    }

    // Cancel upward velocity on ceiling bonk so we fall immediately instead of hugging it.
    if (collisionFlags.isSet(PxControllerCollisionFlag::eCOLLISION_UP) && player.verticalVelocity > 0.f) {
        player.verticalVelocity = 0.f;
    }
}

void PlayerSystem::PostPhysicsUpdate(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }

    auto& [player, playerEntity] = *vault.GetComponents<Player>().begin();
    Transform& playerTransform = vault.GetComponent<Transform>(playerEntity);
    CharacterController& playerController = vault.GetComponent<CharacterController>(playerEntity);

    playerController.positionLastPhysicsUpdate = playerTransform.location;
    playerController.rotationLastPhysicsUpdate = playerTransform.rotation;
    playerTransform.location = playerController.GetPosition();
    playerTransform.rotation = rotation;
}
