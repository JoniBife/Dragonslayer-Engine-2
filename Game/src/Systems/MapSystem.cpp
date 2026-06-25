#include "Game/Systems/MapSystem.hpp"

#include <Game/PhysicsCollisionGroups.hpp>
#include <Physics/Components/DynamicRigidBody.hpp>
#include <Physics/Components/StaticRigidBody.hpp>
#include <Renderer/Components/PrimitiveRenderer.hpp>
#include <Runtime/Components/Transform.hpp>
#include <Runtime/Input.hpp>

void MapSystem::Start(ThreadContext& threadContext, Vault& vault) {

    if (!threadContext.IsMainThread()) {
        return;
    }

    Physics::CreatePhysicsMaterial("DebugCubeMaterial", 1.f, 1.f, 0.f);

    const Entity plane = vault.CreateEntity("Plane");

    Transform& transform = vault.EmplaceComponent<Transform>(plane);
    transform.scale = Vec3(1000.f);

    PrimitiveRenderer& primitiveRenderer = vault.EmplaceComponent<PrimitiveRenderer>(plane);
    primitiveRenderer.color = Vec3(.9f);
    primitiveRenderer.primitiveType = PrimitiveType::Plane;
    primitiveRenderer.castShadow = false;

    const PxMaterial& material = Physics::CreatePhysicsMaterial("GroundPlaneMaterial", 1.f, 1.f, 0.1f);
    StaticRigidBody& staticRigidBody = vault.EmplaceComponent<StaticRigidBody>(plane);
    staticRigidBody.AddExclusiveShape(PxPlaneGeometry(), material);
    staticRigidBody.SetCollisionGroup(1 << 5, 0xFFFFFFFF);
}

void MapSystem::Update(ThreadContext& threadContext, Vault& vault) {

    if (!threadContext.IsMainThread()) {
        return;
    }

    if (Input::IsKeyPressed(KeyCode::J)) {

        {
            Entity cube = vault.CreateEntity("Cube");

            Transform& transform = vault.EmplaceComponent<Transform>(cube);
            transform.location = Vec3(RandomFloat(-3, 3), 10., RandomFloat(-3, 3));
            transform.scale = Vec3(3.f);

            PrimitiveRenderer& primitiveRenderer = vault.EmplaceComponent<PrimitiveRenderer>(cube);
            primitiveRenderer.color = Vec3(RandomFloat(0, 1), RandomFloat(0, 1), RandomFloat(0, 1));
            primitiveRenderer.primitiveType = PrimitiveType::Cube;

            DynamicRigidBody& dynamicRigidBody = vault.EmplaceComponent<DynamicRigidBody>(cube, transform);
            dynamicRigidBody.AddExclusiveShape(PxBoxGeometry(
                transform.scale.x * .5f,
                transform.scale.y * .5f,
                transform.scale.z * .5f),
                Physics::GetExistingPhysicsMaterial("DebugCubeMaterial")
            );
            dynamicRigidBody.SetMass(RandomFloat(1000.f, 2002.f), true);
            dynamicRigidBody.SetCollisionGroup(CollisionGroup::Environment, CollisionGroup::All);
        }
    }

    //if (Input::IsKeyPressed(KeyCode::G)) {
    //    File file = File::OpenOrCreate("Level.txt");
    //    file.ClearContents();
    //    vault.Serialize(file);
    //    file.Close();
    //}
    //
    //if (Input::IsKeyPressed(KeyCode::H)) {
    //    File file = File::OpenOrCreate("Level.txt");
    //    vault.Deserialize(file);
    //    file.Close();
    //}
}
