#include "Game/Systems/EnemyAISystem.hpp"

#include <Core/IO/BufferedFile.hpp>
#include <Physics/Components/DynamicRigidBody.hpp>
#include <Renderer/Components/PrimitiveRenderer.hpp>
#include <Runtime/Components/PhysicsNPC.hpp>
#include <Runtime/Components/Transform.hpp>
#include <Runtime/Input.hpp>

#include "Game/Components/GameState.hpp"
#include "Game/Components/Player.hpp"
#include "Game/PhysicsCollisionGroups.hpp"
#include "Game/Components/Enemies/Enemy.hpp"
#include "Game/Components/Enemies/BigMouth.hpp"
#include "Game/Components/Enemies/Hover.hpp"
#include "Game/Components/Projectile.hpp"

static void SpawnEnemyAtLocation(Vault& vault, const Vec3& location) {

    Entity enemyEntity = vault.CreateEntity({"Enemies"});
    Enemy& enemy = vault.EmplaceComponent<Enemy>(enemyEntity);

    const Vec3 enemyScale = Vec3(RandomFloat(0.f,1.f)*RandomFloat(0.f,1.f) * 5.5f + .5f);

    PhysicsNPC& physicsNPC = vault.EmplaceComponent<PhysicsNPC>(enemyEntity, Transform(location, enemyScale));

    physicsNPC.primitiveRenderer.primitiveType = PrimitiveType::Sphere;
    physicsNPC.primitiveRenderer.color = Vec3(0.5f, 0.5f, 0.6f);

    physicsNPC.dynamicRigidBody.AddExclusiveShape(PxSphereGeometry(enemyScale.x * .5f));

    if (enemyScale.x > 1.5f) {
        enemy.maxHealth = 500.f;
        Hover& hover = vault.EmplaceComponent<Hover>(enemyEntity);
        hover.nextShootTime = gTime.totalTime + RandomFloat(0.f, hover.shootRate);
        hover.nextChangeHoverHeight = gTime.totalTime + RandomFloat(0.f, hover.nextChangeHoverHeight);
        hover.hoverHeight += RandomFloat(0.f, hover.hoverHeight);

        physicsNPC.dynamicRigidBody.SetCollisionGroup(
            CollisionGroup::Enemy,
            CollisionGroup::All & ~CollisionGroup::EnemySmall);
    } else {
        BigMouth& bigMouth = vault.EmplaceComponent<BigMouth>(enemyEntity);
        bigMouth.nextAttackTime = gTime.totalTime + RandomFloat(0.f, bigMouth.attackRate);

        physicsNPC.dynamicRigidBody.SetCollisionGroup(
            CollisionGroup::EnemySmall,
            CollisionGroup::All & ~CollisionGroup::Enemy);
    }

    enemy.currentHealth = enemy.maxHealth;

    physicsNPC.dynamicRigidBody.ConstrainMovement(PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, true);
    physicsNPC.dynamicRigidBody.ConstrainMovement(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, true);
    physicsNPC.dynamicRigidBody.ConstrainMovement(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, true);

    physicsNPC.dynamicRigidBody.SetMass(.1f);
}

void EnemyAISystem::Start(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }

    BufferedFile file = BufferedFile<>::OpenOrCreate("Test.txt");

    file.Write("10");

    file.Close();
}

void EnemyAISystem::Update(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }

#if WITH_EDITOR
    ImGui::Begin("Enemies");

    static int32 numEnemies = 1000;

    ImGui::InputInt("Number of enemies", &numEnemies);

    if (ImGui::Button("Spawn") || Input::IsKeyPressed(KeyCode::C)) {

        const uint32 val = sqrt(numEnemies);

        for (uint32 x = 0; x < val; x++) {
            for (uint32 y = 0; y < val; y++) {
                SpawnEnemyAtLocation(vault, Vec3(x * 3.f, 50.f ,y * 3.f));
            }
        }
    }

    if (ImGui::Button("Destroy")) {
        vault.DestroyEntitiesWithComponent<Enemy>();
    }

    ImGui::End();
#endif
}

static void MoveHovers(ThreadContext& threadContext, Vault& vault, const Vec3& followTarget) {

    struct ProjectilePhysics {
        DynamicRigidBody& dynamicRigidBody;
        Vec3 initialForce;
    };

    // We add track all the new projectiles physics data so they can be added to the scene in a single batch
    // we do that here because AddForce cannot be called if the PxActor is not in the scene, as such we add manually
    // since we want to create a projectile and apply a force immediately
    TempArray<ProjectilePhysics> newProjectilesPhysics(8);

    for (auto& [hover, entity] : vault.GetComponents<Hover>()) {

        PhysicsNPC& physicsNPC = vault.GetComponent<PhysicsNPC>(entity);

        if (!physicsNPC.dynamicRigidBody.ExistsInScene()) {
            continue;
        }

        // Calculate the direction to the player
        Vec3 dirToPlayer = followTarget - physicsNPC.transform.location;
        const float distanceToPlayer = dirToPlayer.Magnitude();
        dirToPlayer = dirToPlayer.Normalize();

        // Calculate Desired Velocity
        Vec3 desiredVelocity = dirToPlayer * hover.maxSpeed;
        desiredVelocity.y = 0.f;

        // Get Current Velocity from PhysX
        Vec3 currentVelocity = physicsNPC.dynamicRigidBody.GetLinearVelocity();
        currentVelocity.y = 0.f;

        // Calculate the velocity difference (Steering Force)
        Vec3 velocityChange = desiredVelocity - currentVelocity;

        // Apply the corrective force
        if (distanceToPlayer < 500.f && distanceToPlayer > hover.shootRange) {
            physicsNPC.primitiveRenderer.color = Vec3(.6f, 0.f, 0.f);
            physicsNPC.dynamicRigidBody.AddForce(velocityChange * hover.acceleration, PxForceMode::eFORCE);
        } else {
            physicsNPC.primitiveRenderer.color = Vec3(0.5f, 0.5f, 0.6f);
        }

        if (gTime.totalTime >= hover.nextChangeHoverHeight) {
            hover.nextChangeHoverHeight = gTime.totalTime + hover.changeHoverHeightRate + RandomFloat(0.f, hover.nextChangeHoverHeight);
            hover.hoverHeight = RandomFloat(2.f, 20.f);

            Vec3 randomDirChangeForce = {
                RandomFloat(0.f, 1.f),
                RandomFloat(0.f, 1.f),
                RandomFloat(0.f, 1.f)
            };
            randomDirChangeForce = randomDirChangeForce * 2.f - 1.f;
            physicsNPC.dynamicRigidBody.AddForce(randomDirChangeForce * 5.f, PxForceMode::eIMPULSE);
            //hover.hoverHeight = std::clamp(hover.hoverHeight, 2.f, 20.f);
        }

        const Vec3 rayOrigin = physicsNPC.transform.location - Vec3::UP*physicsNPC.transform.scale.x * .5f;

        const float compression = hover.hoverHeight - rayOrigin.y;

        if (compression > 0.f) {
            const float verticalVelocity = physicsNPC.dynamicRigidBody.GetLinearVelocity().y;
            // Hooke's Law F = (k * x) - (c * v)
            const float springForce = (hover.springConstant * compression) - (hover.dampingConstant * verticalVelocity);
            // ForceMode.Acceleration ignores the Rigidbody's mass.
            physicsNPC.dynamicRigidBody.AddForce(Vec3::UP * springForce, PxForceMode::eACCELERATION);
        }

        if (gTime.totalTime >= hover.nextShootTime) {

            physicsNPC.dynamicRigidBody.AddForce(dirToPlayer * 60.f, PxForceMode::eFORCE);
            hover.nextShootTime = gTime.totalTime + hover.shootRate + RandomFloat(0.f, hover.shootRate);

            const Entity projectileEntity = vault.CreateEntity("Projectile", {"Weapon", "Projectiles"});
            vault.EmplaceComponent<Projectile>(projectileEntity);
            Transform& transform = vault.EmplaceComponent<Transform>(projectileEntity);
            transform.location = physicsNPC.transform.location;
            transform.scale = Vec3(.5f);

            PrimitiveRenderer& renderer = vault.EmplaceComponent<PrimitiveRenderer>(projectileEntity);
            renderer.primitiveType = PrimitiveType::Sphere;
            renderer.color = Vec3(0, 1.f, 0.f);

            DynamicRigidBody& rigidBody = vault.EmplaceComponent<DynamicRigidBody>(projectileEntity, physicsNPC.transform, true);
            rigidBody.AddExclusiveShape(PxSphereGeometry(transform.scale.x * .5f));
            rigidBody.SetCollisionGroup(CollisionGroup::Projectile, CollisionGroup::Player, true);
            //rigidBody.onCollisionCallback = OnProjectileCollisionCallback;
            rigidBody.SetGravity(false);

            newProjectilesPhysics.Emplace(rigidBody, dirToPlayer * 100.f);
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

static void MoveBigMouths(ThreadContext& threadContext, Vault& vault, const Vec3& followTarget) {

    for (auto& [bigMouth, entity] : vault.GetComponents<BigMouth>()) {

        PhysicsNPC& physicsNPC = vault.GetComponent<PhysicsNPC>(entity);

        if (!physicsNPC.dynamicRigidBody.ExistsInScene()) {
            continue;
        }

        // Calculate the direction to the player
        Vec3 dirToPlayer = followTarget - physicsNPC.transform.location;
        const float distanceToPlayer = dirToPlayer.Magnitude();
        dirToPlayer = dirToPlayer.Normalize();

        // Calculate Desired Velocity
        Vec3 desiredVelocity = dirToPlayer * bigMouth.maxSpeed;
        desiredVelocity.y = 0.f;

        // Get Current Velocity from PhysX
        Vec3 currentVelocity = physicsNPC.dynamicRigidBody.GetLinearVelocity();
        currentVelocity.y = 0.f;

        // Calculate the velocity difference (Steering Force)
        Vec3 velocityChange = desiredVelocity - currentVelocity;

        if (distanceToPlayer < 500.f) {
            physicsNPC.primitiveRenderer.color = Vec3(.6f, 0.f, 0.f);
            physicsNPC.dynamicRigidBody.AddForce(velocityChange * bigMouth.acceleration, PxForceMode::eFORCE);

            if (distanceToPlayer < 20.f) {

                physicsNPC.primitiveRenderer.color = Vec3(1.f, 0.f, 0.f);

                if (gTime.totalTime >= bigMouth.nextAttackTime) {
                    physicsNPC.dynamicRigidBody.AddForce(dirToPlayer * 500.f, PxForceMode::eFORCE);
                    bigMouth.nextAttackTime = gTime.totalTime + bigMouth.attackRate + RandomFloat(0.f, bigMouth.attackRate);
                }
            }
        } else {
            physicsNPC.primitiveRenderer.color = Vec3(0.5f, 0.5f, 0.6f);
        }
    }
}


void EnemyAISystem::PrePhysicsUpdate(ThreadContext& threadContext, Vault& vault) {

    if (!threadContext.IsMainThread()) {
        return;
    }

    auto& [gameState, gameStateEntity] = *vault.GetComponents<GameState>().begin();
    if (gameState.isBuildingWeapon) {
        return;
    }

    auto& [player, playerEntity] = *vault.GetComponents<Player>().begin();
    Transform& playerTransform = vault.GetComponent<Transform>(playerEntity);
    DynamicRigidBody& playerDynamicRigidBody = vault.GetComponent<DynamicRigidBody>(playerEntity);

    // TODO Improve this logic, we should use a proper target follow algorithm
    //const Vec3 followTarget = playerTransform.location + playerDynamicRigidBody.GetLinearVelocity() * gTime.physicsDeltaTime * 50.f;

    MoveHovers(threadContext, vault, playerTransform.location);
    MoveBigMouths(threadContext, vault, playerTransform.location);
}
