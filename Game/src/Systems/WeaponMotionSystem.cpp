#include "Game/Systems/WeaponMotionSystem.hpp"

#include <Core/EngineGlobals.hpp>
#include <Core/Time.hpp>

#if WITH_EDITOR
#include <Editor/EditorGlobals.hpp>
#endif

#include <Math/MathAux.hpp>
#include <Physics/Components/CharacterController.hpp>
#include <Physics/Components/CollisionEvent.hpp>
#include <Physics/Components/DynamicRigidBody.hpp>
#include <Physics/PhysicsCommon.hpp>
#include <PxPhysicsAPI.h>
#include <Renderer/Camera.hpp>
#include <Renderer/Components/PrimitiveRenderer.hpp>
#include <Runtime/Components/PhysicsNPC.hpp>
#include <Runtime/Components/Transform.hpp>
#include <Runtime/Input.hpp>

#include "Game/Components/Enemies/Enemy.hpp"
#include "Game/Components/FollowEntity.hpp"
#include "Game/Components/GameState.hpp"
#include "Game/Components/Handles/ShootV1Handle.hpp"
#include "Game/Components/Handles/ShootV2Handle.hpp"
#include "Game/Components/Handles/ShootV3Handle.hpp"
#include "Game/Components/Handles/ShootV4Handle.hpp"
#include "Game/Components/Handles/SwordHandle.hpp"
#include "Game/Components/Modules/AirModule.hpp"
#include "Game/Components/Modules/PistonModule.hpp"
#include "Game/Components/Modules/RotatorModule.hpp"
#include "Game/Components/Modules/ShootModule.hpp"
#include "Game/Components/Player.hpp"
#include "Game/Components/Projectile.hpp"
#include "Game/Components/WeaponRuntime.hpp"
#include "Game/ModulesAndHandles.hpp"
#include "Game/PhysicsCollisionGroups.hpp"

void WeaponMotionSystem::Start(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }

}

static void UpdateFollowEntities(Vault& vault, bool skipRigidBodies) {
    for (auto& [followEntity, entity] : vault.GetComponents<FollowEntity>()) {

        if (followEntity.target == ECS::InvalidEntity) {
            continue;
        }

        if (DynamicRigidBody* dynamicRigidBody = vault.TryGetComponent<DynamicRigidBody>(entity)) {
            if (skipRigidBodies) {
                continue;
            }

            if (!dynamicRigidBody->ExistsInScene()) {
                continue;
            }

            if (!dynamicRigidBody->IsKinematic()) {
                continue;
            }

            Transform& transform = vault.GetComponent<Transform>(entity);

            const Transform& targetTransform = vault.GetComponent<Transform>(followEntity.target);

            Vec3 targetPosition = targetTransform.location;
            Quat targetRotation = targetTransform.rotation;
            if (DynamicRigidBody* rigidBody = vault.TryGetComponent<DynamicRigidBody>(followEntity.target)) {
                targetPosition = rigidBody->GetInterpolatedPosition(targetPosition);
                targetRotation = rigidBody->GetInterpolatedRotation(targetRotation);
            }

            const Vec3 offsetWorldSpace = targetRotation.ToRotationMatrix() * followEntity.offsetLocation;

            transform.location = targetPosition + offsetWorldSpace;
            transform.rotation = targetRotation * followEntity.offsetRotation;

            dynamicRigidBody->MoveKinematic(ToPX(transform));

        } else {
            if (!skipRigidBodies) {
                continue;
            }

            Transform& transform = vault.GetComponent<Transform>(entity);

            const Transform& targetTransform = vault.GetComponent<Transform>(followEntity.target);

            Vec3 targetPosition = targetTransform.location;
            Quat targetRotation = targetTransform.rotation;
            if (DynamicRigidBody* rigidBody = vault.TryGetComponent<DynamicRigidBody>(followEntity.target)) {
                targetPosition = rigidBody->GetInterpolatedPosition(targetPosition);
                targetRotation = rigidBody->GetInterpolatedRotation(targetRotation);
            }

            const Vec3 offsetWorldSpace = targetRotation.ToRotationMatrix() * followEntity.offsetLocation;

            transform.location = targetPosition + offsetWorldSpace;
            transform.rotation = targetRotation * followEntity.offsetRotation;
        }
    }
}

static void UpdateAirModules(Vault& vault, bool activate) {

    if (!activate) {
        return;
    }

    TempArray<uint32> entitiesToRemove(8);
    for (auto& [airModule, airModuleEntity] : vault.GetComponents<AirModule>()) {

        const Transform& airModuleTransform = vault.GetComponent<Transform>(airModuleEntity);
        const Vec3 airForce = (airModuleTransform.GetUpAxis() + Vec3::UP * .1f).Normalize() * airModule.force;

        for (uint32 i = 0; i < airModule.entitiesWithinAirVolume.GetSize(); ++i) {

            const Entity entity = airModule.entitiesWithinAirVolume[i];

            // An entity may have been destroyed before thus we have to remove it from the list
            // when that happens, technically the entity will stay in the list until player shoots, but that is ok!
            if (!vault.IsEntityValid(entity)) {
                entitiesToRemove.Add(i);
                continue;
            }

            if (DynamicRigidBody* rigidBody = vault.TryGetComponent<DynamicRigidBody>(entity)) {
                rigidBody->AddForce(airForce, PxForceMode::eFORCE);
            } else if (PhysicsNPC* physicsNPC = vault.TryGetComponent<PhysicsNPC>(entity)) {
                physicsNPC->dynamicRigidBody.AddForce(airForce, PxForceMode::eFORCE);
            }
        }

        // Removing the entities in reverse order, this was we can safely use
        // remove at swap with index, since they were added in ascending order
        for (int32 i = static_cast<int32>(entitiesToRemove.GetSize()) - 1; i >= 0; --i) {
            airModule.entitiesWithinAirVolume.RemoveAndSwapAt(entitiesToRemove[i]);
        }

        entitiesToRemove.Reset();
    }
}

static void UpdateRotatorModules(Vault& vault, bool activate) {
    for (auto& [rotatorModule, entity] : vault.GetComponents<RotatorModule>()) {

        if (activate) {
            rotatorModule.UpdateRotation(1.f, gTime.deltaTime);
        } else {
            rotatorModule.UpdateRotation(0.f, gTime.deltaTime);
        }

        FollowEntity& followEntity = vault.GetComponent<FollowEntity>(entity);
        followEntity.offsetRotation = rotatorModule.offsetRotation * Quat(rotatorModule.rotation, Vec3::UP);
    }
}

static float PistonFunction(float x) {
    if (x < .5f) {
        return x * 2.f; // Linear start
    }
    return sin(x * PI);
}

static void UpdatePistonModules(Vault& vault, bool activate) {
    for (auto& [pistonModule, entity] : vault.GetComponents<PistonModule>()) {

        if (pistonModule.pushTime > 0.f || activate) {

            pistonModule.pushTime += gTime.deltaTime;

            if (pistonModule.pushTime >= pistonModule.pushDuration) {
                pistonModule.pushTime = 0.f;
            }
        }

        const float pushFactor = std::clamp(pistonModule.pushTime / pistonModule.pushDuration, 0.f, 1.f);
        const float pushOffset = PistonFunction(pushFactor) * pistonModule.pushDistance;

        const Vec3 pushDirection = pistonModule.offsetRotation.ToRotationMatrix().GetUpAxis();

        FollowEntity& followEntity = vault.GetComponent<FollowEntity>(entity);
        followEntity.offsetLocation = pistonModule.offsetLocation + pushDirection*pushOffset;
    }
}

static void OnProjectileCollisionCallback(Vault& vault, const CollisionEvent& collisionEvent) {
    Entity otherEntity = ToEntity(*collisionEvent.actorB);
    if (Enemy* enemy = vault.TryGetComponent<Enemy>(otherEntity)) {
        enemy->currentHealth -= 50.f;
        if (enemy->currentHealth <= 0.f) {
            vault.DestroyEntity(otherEntity);
        }
    }
}

static bool drawCrosshair = false;

static void UpdateShootModules(Vault& vault, bool activate) {

    // React to projectile collisions
    TempArray<Entity> entitiesToDestroy(64);
    for (auto& [projectile, entity] : vault.GetComponents<Projectile>()) {

        if (CollisionEvents* collisionEvents = vault.TryGetComponent<CollisionEvents>(entity)) {
            for (CollisionEvent& collisionEvent : collisionEvents->events) {
                OnProjectileCollisionCallback(vault, collisionEvent);
            }
            entitiesToDestroy.Add(entity);
        }
    }

    for (Entity& entity : entitiesToDestroy) {
        vault.DestroyEntity(entity);
    }

    struct ProjectilePhysics {
        DynamicRigidBody& dynamicRigidBody;
        Vec3 initialForce;
    };

    // We add track all the new projectiles physics data so they can be added to the scene in a single batch
    // we do that here because AddForce cannot be called if the PxActor is not in the scene, as such we add manually
    // since we want to create a projectile and apply a force immediately
    TempArray<ProjectilePhysics> newProjectilesPhysics(8);

    for (auto& [shootModule, entity] : vault.GetComponents<ShootModule>()) {

        if (shootModule.time > 0.f) {
            shootModule.time += gTime.physicsDeltaTime;
            if (shootModule.time >= shootModule.rate + shootModule.randomRateOffset) {
                shootModule.time = 0.f;
                shootModule.RandomizeIntervalOffset();
            }
        }

        if (shootModule.time == 0.f && activate) {

            const Entity projectileEntity = vault.CreateEntity("Projectile", {"Weapon", "Projectiles"});
            vault.EmplaceComponent<Projectile>(projectileEntity);
            Transform& transform = vault.EmplaceComponent<Transform>(projectileEntity);

            Transform& shootModuleTransform = vault.GetComponent<Transform>(entity);
            transform.location = shootModuleTransform.location;
            transform.scale = Vec3(.5f);

            PrimitiveRenderer& renderer = vault.EmplaceComponent<PrimitiveRenderer>(projectileEntity);
            renderer.primitiveType = PrimitiveType::Sphere;

            DynamicRigidBody& rigidBody = vault.EmplaceComponent<DynamicRigidBody>(projectileEntity, shootModuleTransform, true);
            rigidBody.AddExclusiveShape(PxSphereGeometry(transform.scale.x * .5f));
            rigidBody.SetCollisionGroup(CollisionGroup::Projectile, CollisionGroup::Enemy | CollisionGroup::EnemySmall | CollisionGroup::Environment, true);
            rigidBody.receiveCollisionEvents = true;
            rigidBody.SetGravity(false);

            newProjectilesPhysics.Emplace(rigidBody, shootModuleTransform.GetUpAxis() * shootModule.force);

            shootModule.time += gTime.physicsDeltaTime;
        }
    }

    if (newProjectilesPhysics.IsEmpty()) {
        return;
    }

    TempArray<PxActor*> newBodies(newProjectilesPhysics.GetSize());
    for (ProjectilePhysics& projectilePhysics : newProjectilesPhysics) {
        newBodies.Emplace(projectilePhysics.dynamicRigidBody.body);
    }
    gPhysicsScene->addActors(newBodies.GetData(), newBodies.GetSize());

    // Add force only after adding all projectiles to physics scene manually, if not done this way
    // they would only be in scene once the Physics Engine updates
    for (ProjectilePhysics& projectilePhysics : newProjectilesPhysics) {
        projectilePhysics.dynamicRigidBody.AddForce(projectilePhysics.initialForce, PxForceMode::eIMPULSE, true);
    }
}

static void UpdateHandleMotion(Vault& vault, bool inputMotion) {

    drawCrosshair = false;

    auto& [player, playerEntity] = *vault.GetComponents<Player>().begin();

    const Transform& playerTransform = vault.GetComponent<Transform>(playerEntity);
    //const DynamicRigidBody& playerRigidBody = vault.GetComponent<DynamicRigidBody>(playerEntity);
    const CharacterController& playerController = vault.GetComponent<CharacterController>(playerEntity);

    // We follow the time interpolated position and rotation of the player since it is moved via the physics system
    const Vec3 interpolatedPlayerPosition = playerController.GetInterpolatedPosition(playerTransform.location);
    const Quat interpolatedPlayerRotation = playerController.GetInterpolatedRotation(playerTransform.rotation);
    const PxTransform interpolatedPlayerTransform = { ToPX(interpolatedPlayerPosition), ToPX(interpolatedPlayerRotation) };

    Transform& leftHandTransform = vault.GetComponent<Transform>(player.leftHandEntity);
    Transform& rightHandTransform = vault.GetComponent<Transform>(player.rightHandEntity);

    const Mat4 playerRotationMatrixForm = interpolatedPlayerRotation.ToRotationMatrix();
    leftHandTransform.location = interpolatedPlayerPosition + -playerRotationMatrixForm.GetRightAxis() * .7f;
    rightHandTransform.location = interpolatedPlayerPosition + playerRotationMatrixForm.GetRightAxis() * .7f;

    WeaponRuntime* weaponRuntime = vault.TryGetComponent<WeaponRuntime>(playerEntity);
    if (!weaponRuntime || weaponRuntime->handleEntity == ECS::InvalidEntity) {
        return;
    }

    for (auto& [swordHandle, entity] : vault.GetComponents<SwordHandle>()) {

        Transform& handleTransform = vault.GetComponent<Transform>(weaponRuntime->handleEntity);

        // Start swing on input
        if (inputMotion && !swordHandle.isSwinging) {
            swordHandle.isSwinging = true;
            swordHandle.swingTime = 0.f;
            swordHandle.activatedThisSwing = false;
        }

        // Advance swing
        if (swordHandle.isSwinging) {
            swordHandle.swingTime += gTime.physicsDeltaTime;
            if (swordHandle.swingTime >= swordHandle.swingDuration) {
                swordHandle.swingTime = 0.f;
                swordHandle.isSwinging = false;
                swordHandle.activatedThisSwing = false;
            }
        }

        // Compute handle world transform (same formula as the original PlayerSystem bob)
        const float swingProgress = swordHandle.isSwinging
            ? (swordHandle.swingTime / swordHandle.swingDuration)
            : 0.f;

        const Quat swingRotation = Quat(std::sinf(swingProgress * PI) * PI, Vec3::UP);
        const PxTransform swingTransform(ToPX(swingRotation));

        PxTransform localHandleTransform =
            PxTransform(ToPX(GetHandleOffset(HandleType::Sword)), ToPX(GetHandleRotation(HandleType::Sword)));

        const PxTransform finalHandleWorldTransform = interpolatedPlayerTransform * swingTransform * localHandleTransform;

        handleTransform.location = ToDS(finalHandleWorldTransform.p);
        handleTransform.rotation = ToDS(finalHandleWorldTransform.q);

        PxTransform localLeftHandTransform = localHandleTransform;
        localLeftHandTransform.p -= PxVec3(.1f, 0.f, 0.f);

        PxTransform localRightHandTransform = localHandleTransform;
        localRightHandTransform.p += PxVec3(.1f, 0.f, 0.f);

        leftHandTransform.location = ToDS((interpolatedPlayerTransform * swingTransform * localLeftHandTransform).p);
        rightHandTransform.location = ToDS((interpolatedPlayerTransform * swingTransform * localRightHandTransform).p);
    }

    // Compute world-space aim target from screen center for shoot handles
    const Vec3 centerDir = gCamera->DirectionTowardsScreen({
        gCamera->GetViewportWidth() * 0.5f,
        gCamera->GetViewportHeight() * 0.5f
    });
    Vec3 aimTarget = gCamera->GetPosition() + centerDir * 100000.f;

    RayCastHit hit;
    if (Physics::RayCast(gCamera->GetPosition(), centerDir, 1000000.f, hit, CollisionGroup::All)) {
        aimTarget = hit.position;
    }

    for (auto& [shootHandle, entity] : vault.GetComponents<ShootV1Handle>()) {

        drawCrosshair = true;

        Transform& handleTransform = vault.GetComponent<Transform>(weaponRuntime->handleEntity);

        const PxTransform localHandlePose =
            PxTransform(ToPX(GetHandleOffset(HandleType::ShootV1)), ToPX(GetHandleRotation(HandleType::ShootV1)));

        const PxTransform finalHandleWorldTransform = interpolatedPlayerTransform * localHandlePose;

        const Vec3 handlePos = ToDS(finalHandleWorldTransform.p);

        const Vec3 aimDirection = (aimTarget - handlePos).Normalize();
        Vec3 right = Cross(Vec3::UP, aimDirection);
        if (right.MagnitudeSquared() < 1e-6f) {
            right = Cross(Vec3::FORWARD, aimDirection);
        }
        right = right.Normalize();
        const Vec3 up = Cross(aimDirection, right).Normalize();
        const Mat3 rotation(right, up, aimDirection);

        handleTransform.location = handlePos;
        handleTransform.rotation = rotation.ToQuaternion();

        leftHandTransform.location = handlePos;
        rightHandTransform.location = handlePos;
    }

    for (auto& [shootHandle, entity] : vault.GetComponents<ShootV2Handle>()) {

        drawCrosshair = true;

        Transform& handleTransform = vault.GetComponent<Transform>(weaponRuntime->handleEntity);

        const PxTransform localHandlePose =
            PxTransform(ToPX(GetHandleOffset(HandleType::ShootV2)), ToPX(GetHandleRotation(HandleType::ShootV2)));

        const PxTransform finalHandleWorldTransform = interpolatedPlayerTransform * localHandlePose;

        const Vec3 handlePos = ToDS(finalHandleWorldTransform.p);

        const Vec3 aimDirection = (aimTarget - handlePos).Normalize();
        Vec3 right = Cross(Vec3::UP, aimDirection);
        if (right.MagnitudeSquared() < 1e-6f) {
            right = Cross(Vec3::FORWARD, aimDirection);
        }
        right = right.Normalize();
        const Vec3 up = Cross(aimDirection, right).Normalize();
        const Mat3 rotation(aimDirection, right, up);

        handleTransform.location = handlePos;
        handleTransform.rotation = rotation.ToQuaternion();

        leftHandTransform.location = handlePos;
        rightHandTransform.location = handlePos;
    }

    for (auto& [shootHandle, entity] : vault.GetComponents<ShootV3Handle>()) {

        drawCrosshair = true;

        Transform& handleTransform = vault.GetComponent<Transform>(weaponRuntime->handleEntity);

        const PxTransform localHandlePose =
            PxTransform(ToPX(GetHandleOffset(HandleType::ShootV3)), ToPX(GetHandleRotation(HandleType::ShootV3)));

        const PxTransform finalHandleWorldTransform = interpolatedPlayerTransform * localHandlePose;

        const Vec3 handlePos = ToDS(finalHandleWorldTransform.p);

        const Vec3 aimDirection = (aimTarget - handlePos).Normalize();
        Vec3 right = Cross(Vec3::UP, aimDirection);
        if (right.MagnitudeSquared() < 1e-6f) {
            right = Cross(Vec3::FORWARD, aimDirection);
        }
        right = right.Normalize();
        const Vec3 up = Cross(aimDirection, right).Normalize();
        const Mat3 rotation(aimDirection, right, up);

        handleTransform.location = handlePos;
        handleTransform.rotation = rotation.ToQuaternion();

        leftHandTransform.location = handlePos;
        rightHandTransform.location = handlePos;
    }

    for (auto& [shootHandle, entity] : vault.GetComponents<ShootV4Handle>()) {

        drawCrosshair = true;

        Transform& handleTransform = vault.GetComponent<Transform>(weaponRuntime->handleEntity);

        const PxTransform localHandlePose =
            PxTransform(ToPX(GetHandleOffset(HandleType::ShootV4)), ToPX(GetHandleRotation(HandleType::ShootV4)));

        const PxTransform finalHandleWorldTransform = interpolatedPlayerTransform * localHandlePose;

        const Vec3 handlePos = ToDS(finalHandleWorldTransform.p);

        const Vec3 aimDirection = (aimTarget - handlePos).Normalize();
        Vec3 right = Cross(Vec3::UP, aimDirection);
        if (right.MagnitudeSquared() < 1e-6f) {
            right = Cross(Vec3::FORWARD, aimDirection);
        }
        right = right.Normalize();
        const Vec3 up = Cross(aimDirection, right).Normalize();
        const Mat3 rotation(aimDirection, right, up);

        handleTransform.location = handlePos;
        handleTransform.rotation = rotation.ToQuaternion();

        leftHandTransform.location = handlePos;
        rightHandTransform.location = handlePos;
    }
}

static void DrawCrosshair() {
#if WITH_EDITOR
    if (gDetachedFromGame) {
        return;
    }
    const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    const ImVec2 center(displaySize.x * 0.5f, displaySize.y * 0.5f);
    constexpr float radius = 32.f;
    constexpr float thickness = 5.f;
    constexpr ImU32 green = IM_COL32(0, 255, 0, 100);

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    drawList->AddCircle(center, radius, green, 10, thickness);
    drawList->AddCircleFilled(center, 2.5f, green);
#endif
}

static bool inputMotion = false;

void WeaponMotionSystem::Update(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }

    auto& [gameState, gameStateEntity] = *vault.GetComponents<GameState>().begin();
    if (gameState.isBuildingWeapon) {
        return;
    }

    if (Input::IsMouseButtonDown(MouseButtonCode::Left)) {
        inputMotion = true;
    }

    UpdateHandleMotion(vault, inputMotion);

    UpdateRotatorModules(vault, inputMotion);
    UpdatePistonModules(vault, inputMotion);

    UpdateFollowEntities(vault, true);

    if (drawCrosshair) {
        DrawCrosshair();
    }
}

void WeaponMotionSystem::PrePhysicsUpdate(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }

    TempArray<Entity> projectilesToDestroy(8);
    for (auto& [projectile, entity] : vault.GetComponents<Projectile>()) {
        projectile.age += gTime.physicsDeltaTime;

        if (projectile.age >= projectile.destroyAge) {
            projectilesToDestroy.Add(entity);
        }
    }

    for (const Entity entity : projectilesToDestroy) {
        vault.DestroyEntity(entity);
    }

    UpdateShootModules(vault, inputMotion);
    UpdateAirModules(vault, inputMotion);

    inputMotion = false;

    UpdateFollowEntities(vault, false);
}
