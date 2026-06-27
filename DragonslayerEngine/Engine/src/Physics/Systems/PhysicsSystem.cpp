#include "Physics/Systems/PhysicsSystem.hpp"

#if WITH_EDITOR
#include "Editor/EditorGlobals.hpp"
#endif

#include "Core/Containers/ArrayHelper.hpp"
#include "Core/Time.hpp"
#include "Editor/TimeScope.hpp"
#include "Physics/Components/CharacterController.hpp"
#include "Physics/Components/DynamicRigidBody.hpp"
#include "Physics/Components/StaticRigidBody.hpp"
#include "Runtime/Components/PhysicsNPC.hpp"
#include "Runtime/Components/Transform.hpp"
#include "Runtime/Input.hpp"

PhysicsSystem::PhysicsSystem(const PhysicsSettings& settings) :
    settings(settings),
    allocator(*gGlobalAllocator, settings.usedMemory),
    scratchBuffer(allocator.Allocate(settings.scratchMemory, 16 /*PhysX requires alignment of 16*/)),
    physXAllocator(allocator),
    simulationCallback(allocator),
    materials(32, &allocator){

    // Create foundation
    foundation = PxCreateFoundation(PX_PHYSICS_VERSION, physXAllocator, errorCallback);

    // Create socket to PhysX visual debugger if enabled
    if (settings.enablePhysicsVisualDebugger) {
        pvd = PxCreatePvd(*foundation);
    }

    physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale(), false, pvd);

    if (settings.enablePhysicsVisualDebugger) {
        PxInitExtensions(*physics, pvd);
    }

    defaultMaterial = physics->createMaterial(0.6f, 0.6f, 0);

    dispatcher = PxDefaultCpuDispatcherCreate(std::thread::hardware_concurrency() - 1);

    gPhysics = physics;
    gDefaultPhysicsMaterial = defaultMaterial;
}

PhysicsSystem::~PhysicsSystem() {
    DestroyPhysicsScene();
    PX_RELEASE(dispatcher);
    PxCloseExtensions();
    PX_RELEASE(physics);
    if(pvd)
    {
        PxPvdTransport* pvdTransport = pvd->getTransport();
        PX_RELEASE(pvd);
        PX_RELEASE(pvdTransport);
    }
    PX_RELEASE(foundation);
    allocator.Free();
}

// Class to mark a new static rigid body
DSTRUCT(NewStaticBody) final {
    GENERATE_DSTRUCT_BODY(NewStaticBody)
    PxRigidStatic* body;
}; END_DSTRUCT(NewStaticBody)

// Class to mark a new dynamic rigid body
DSTRUCT(NewDynamicBody) final {
    GENERATE_DSTRUCT_BODY(NewDynamicBody)
    PxRigidDynamic* body;
}; END_DSTRUCT(NewDynamicBody)

// Class to mark a destroyed body
DSTRUCT(DestroyedBody) final {
    GENERATE_DSTRUCT_BODY(DestroyedBody)
    PxActor* body = nullptr;
}; END_DSTRUCT(DestroyedBody)

// We store the Entity in a void* which only works under the assumption our addresses are at least 32bits
// But that's fine since 16-bit CPUs are too old so not supported :)
static_assert(sizeof(Entity) <= sizeof(void*));

static void OnCreateStaticRigidBody(Entity entity, StaticRigidBody& staticRigidBody, Vault& vault) {
    staticRigidBody.body->userData = reinterpret_cast<void*>(static_cast<uintptr>(entity));
    if (staticRigidBody.addManually) {
        return;
    }
    vault.EmplaceComponent<NewStaticBody>(entity).body = staticRigidBody.body;
}

static void OnCreateDynamicRigidBody(Entity entity, DynamicRigidBody& dynamicRigidBody, Vault& vault) {
    dynamicRigidBody.body->userData = reinterpret_cast<void*>(static_cast<uintptr>(entity));
    if (dynamicRigidBody.addManually) {
        return;
    }
    vault.EmplaceComponent<NewDynamicBody>(entity).body = dynamicRigidBody.body;
}

static void OnCreatePhysicsNPC(Entity entity, PhysicsNPC& physicsNPC, Vault& vault) {

    DynamicRigidBody& dynamicRigidBody = physicsNPC.dynamicRigidBody;
    dynamicRigidBody.body->userData = reinterpret_cast<void*>(static_cast<uintptr>(entity));

    if (dynamicRigidBody.addManually) {
        return;
    }
    vault.EmplaceComponent<NewDynamicBody>(entity).body = dynamicRigidBody.body;
}

static void OnDestroyStaticRigidBody(Entity, StaticRigidBody& staticRigidBody, Vault& vault) {

    // Sever link with entity, since they are no longer connected
    staticRigidBody.body->userData = reinterpret_cast<void*>(static_cast<uintptr>(ECS::InvalidEntity));

    // If we destroy a body before it gets added to the scene
    // We can skip the creation of the DestroyedBody and just release it directly!
    if (!staticRigidBody.ExistsInScene()) {
        staticRigidBody.body->release();
        return;
    }
    DestroyedBody& destroyedBody = vault.EmplaceComponent<DestroyedBody>(vault.CreateEntity());
    destroyedBody.body = staticRigidBody.body;
    staticRigidBody.body = nullptr;
}

static void OnDestroyDynamicRigidBody(Entity, DynamicRigidBody& dynamicRigidBody, Vault& vault) {

    // Sever link with entity, since they are no longer connected
    dynamicRigidBody.body->userData = reinterpret_cast<void*>(static_cast<uintptr>(ECS::InvalidEntity));

    // If we destroy a body before it gets added to the scene
    // We can skip the creation of the DestroyedBody and just release it directly!
    if (!dynamicRigidBody.ExistsInScene()) {
        dynamicRigidBody.body->release();
        return;
    }
    DestroyedBody& destroyedBody = vault.EmplaceComponent<DestroyedBody>(vault.CreateEntity());
    destroyedBody.body = dynamicRigidBody.body;
    dynamicRigidBody.body = nullptr;
}

static void OnDestroyPhysicsNPC(Entity, PhysicsNPC& physicsNPC, Vault& vault) {
    DynamicRigidBody& dynamicRigidBody = physicsNPC.dynamicRigidBody;

    // Sever link with entity, since they are no longer connected
    dynamicRigidBody.body->userData = reinterpret_cast<void*>(static_cast<uintptr>(ECS::InvalidEntity));

    // If we destroy a body before it gets added to the scene
    // We can skip the creation of the DestroyedBody and just release it directly!
    if (!dynamicRigidBody.ExistsInScene()) {
        dynamicRigidBody.body->release();
        return;
    }
    DestroyedBody& destroyedBody = vault.EmplaceComponent<DestroyedBody>(vault.CreateEntity());
    destroyedBody.body = dynamicRigidBody.body;
    dynamicRigidBody.body = nullptr;
}

static void OnCreateCharacterController(Entity entity, CharacterController& characterController, Vault&) {
    // PxControllerManager already added the internal kinematic actor to the scene during createController().
    // So we only need to link entity <-> actor via userData; no NewDynamicBody enqueue.
    characterController.controller->getActor()->userData = reinterpret_cast<void*>(static_cast<uintptr>(entity));
}

static void OnDestroyCharacterController(Entity, CharacterController& characterController, Vault&) {
    if (!characterController.controller) {
        return;
    }

    // Sever link with entity, since they are no longer connected
    characterController.controller->getActor()->userData = reinterpret_cast<void*>(static_cast<uintptr>(ECS::InvalidEntity));

    // PxController::release releases the internal actor and removes it from the scene
    characterController.controller->release();
    characterController.controller = nullptr;
}

void PhysicsSystem::Start(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }

    CreatePhysicsScene();

    vault.RegisterOnCreate<StaticRigidBody, OnCreateStaticRigidBody>();
    vault.RegisterOnCreate<DynamicRigidBody, OnCreateDynamicRigidBody>();
    vault.RegisterOnCreate<PhysicsNPC, OnCreatePhysicsNPC>();
    vault.RegisterOnCreate<CharacterController, OnCreateCharacterController>();

    vault.RegisterOnDestroy<StaticRigidBody, OnDestroyStaticRigidBody>();
    vault.RegisterOnDestroy<DynamicRigidBody, OnDestroyDynamicRigidBody>();
    vault.RegisterOnDestroy<PhysicsNPC, OnDestroyPhysicsNPC>();
    vault.RegisterOnDestroy<CharacterController, OnDestroyCharacterController>();

    // Setting up creation from entities that come from the map (thus not created at game runtime)
    for (auto& [newBody, entity]: vault.GetComponents<StaticRigidBody>()) {
        OnCreateStaticRigidBody(entity, newBody, vault);
    }
    for (auto& [newBody, entity]: vault.GetComponents<DynamicRigidBody>()) {
        OnCreateDynamicRigidBody(entity, newBody, vault);
    }
    for (auto& [physicsNPC, entity]: vault.GetComponents<PhysicsNPC>()) {
        OnCreatePhysicsNPC(entity, physicsNPC, vault);
    }
    for (auto& [characterController, entity]: vault.GetComponents<CharacterController>()) {
        OnCreateCharacterController(entity, characterController, vault);
    }
}

static void TryPropagateCollisionEvent(Entity entity, const CollisionEvent& collisionEvent, Vault& vault, bool flipOrder = false) {

    bool propagateCollisionEvent = false;
    if (DynamicRigidBody* dynamicRigidBody = vault.TryGetComponent<DynamicRigidBody>(entity)) {
        propagateCollisionEvent = dynamicRigidBody->receiveCollisionEvents;

    } else if (PhysicsNPC* physicsNPC = vault.TryGetComponent<PhysicsNPC>(entity)) {
        propagateCollisionEvent = physicsNPC->dynamicRigidBody.receiveCollisionEvents;

    } else if (CharacterController* characterController = vault.TryGetComponent<CharacterController>(entity)) {
        propagateCollisionEvent = characterController->receiveCollisionEvents;

    } else if (StaticRigidBody* staticRigidBody = vault.TryGetComponent<StaticRigidBody>(entity)) {
        propagateCollisionEvent = staticRigidBody->receiveCollisionEvents;
    }

    if (propagateCollisionEvent) {
        CollisionEvents* collisionEvents = vault.TryGetComponent<CollisionEvents>(entity);
        if (!collisionEvents) {
            collisionEvents = &vault.EmplaceComponent<CollisionEvents>(entity);
        }
        if (flipOrder) {
            // Flip the order since the convention is that the first actor is the owner of the callback
            // Otherwise callbacks would have to check which actor am I
            collisionEvents->events.Emplace(collisionEvent.actorB, collisionEvent.actorA);
        } else {
            collisionEvents->events.Add(collisionEvent);
        }
    }
}

static void TryPropagateTriggerEvent(Entity entity, const TriggerEvent& triggerEvent, Vault& vault) {

    bool propagateTriggerEvent = false;
    if (DynamicRigidBody* dynamicRigidBody = vault.TryGetComponent<DynamicRigidBody>(entity)) {
        propagateTriggerEvent = dynamicRigidBody->receiveTriggerEvents;

    } else if (PhysicsNPC* physicsNPC = vault.TryGetComponent<PhysicsNPC>(entity)) {
        propagateTriggerEvent = physicsNPC->dynamicRigidBody.receiveTriggerEvents;

    } else if (CharacterController* characterController = vault.TryGetComponent<CharacterController>(entity)) {
        propagateTriggerEvent = characterController->receiveTriggerEvents;

    } else if (StaticRigidBody* staticRigidBody = vault.TryGetComponent<StaticRigidBody>(entity)) {
        propagateTriggerEvent = staticRigidBody->receiveTriggerEvents;
    }

    if (propagateTriggerEvent) {
        TriggerEvents* triggerEvents = vault.TryGetComponent<TriggerEvents>(entity);
        if (!triggerEvents) {
            triggerEvents = &vault.EmplaceComponent<TriggerEvents>(entity);
        }
        triggerEvents->events.Add(triggerEvent);
    }
}

void PhysicsSystem::Update(ThreadContext& threadContext, Vault& vault) {

    if (!threadContext.IsMainThread()) {
        return;
    }

    // Propagate collision and trigger events
    {
        TIME_SCOPE(NameString("Physics_PropagateEvents"));

        for (CollisionEvent& collisionEvent : simulationCallback.collisionEvents) {

            const Entity entityA = ToEntity(*collisionEvent.actorA);
            if (entityA != ECS::InvalidEntity) {
                TryPropagateCollisionEvent(entityA, collisionEvent, vault);
            }

            const Entity entityB = ToEntity(*collisionEvent.actorB);
            if (entityB != ECS::InvalidEntity) {
                TryPropagateCollisionEvent(entityA, collisionEvent, vault, true);
            }
        }

        for (TriggerEvent& triggerEvent : simulationCallback.triggerEvents) {

            const Entity triggerEntity = ToEntity(*triggerEvent.triggerActor);
            if (triggerEntity != ECS::InvalidEntity) {
                TryPropagateTriggerEvent(triggerEntity, triggerEvent, vault);
            }

            const Entity entityInsideTrigger = ToEntity(*triggerEvent.actorInside);
            if (entityInsideTrigger != ECS::InvalidEntity) {
                TryPropagateTriggerEvent(entityInsideTrigger, triggerEvent, vault);
            }
        }

        // Events are reset every physics tick, after they are consumed
        simulationCallback.ResetEvents();
    }

    const uint32 numDestroyedBodies = vault.GetNumberOfComponents<DestroyedBody>();
    const uint32 numCreatedBodies = vault.GetNumberOfComponents<NewStaticBody>() + vault.GetNumberOfComponents<NewDynamicBody>();

    const uint32 numLocalBodies = std::max(numDestroyedBodies, numCreatedBodies);

    if (numLocalBodies > 0) {

        TempArray<PxActor*> localBodies(numLocalBodies);

        // Remove bodies from physics scene
        {
            TIME_SCOPE(NameString("Physics_DestroyBodies"));

            for (const auto& [destroyedBody, entity]: vault.GetComponents<DestroyedBody>()) {

                const PxActorType::Enum bodyType = destroyedBody.body->getType();

                // Bodies without scene don't need to have removeActor called on them!
                if (!destroyedBody.body->getScene()) {
                    destroyedBody.body->release();
                    continue;
                }

                // RemoveActors only supports rigid bodies
                if (bodyType == PxActorType::eRIGID_DYNAMIC || bodyType == PxActorType::eRIGID_STATIC) {
                    localBodies.Emplace(destroyedBody.body);
                } else {
                    scene->removeActor(*destroyedBody.body);
                    destroyedBody.body->release();
                }
            }

            if (!localBodies.IsEmpty()) {
                scene->removeActors(localBodies.GetData(), localBodies.GetSize());

                for (PxActor* actor: localBodies) {
                    actor->release();
                }
            }

            localBodies.Reset();

            vault.DestroyEntitiesWithComponent<DestroyedBody>();
        }

        // Add newly created rigid bodies in a single batch
        {
            TIME_SCOPE(NameString("Physics_AddBodies"));

            for (const auto& [newBody, entity]: vault.GetComponents<NewStaticBody>()) {
                ASSERT(!newBody.body->getScene())
                localBodies.Add(newBody.body);
            }

            for (const auto& [newBody, entity]: vault.GetComponents<NewDynamicBody>()) {
                ASSERT(!newBody.body->getScene())
                localBodies.Add(newBody.body);
            }

            vault.DestroyComponents<NewStaticBody>();
            vault.DestroyComponents<NewDynamicBody>();

            if (!localBodies.IsEmpty()) {
                scene->addActors(localBodies.GetData(), localBodies.GetSize());
            }
        }
    }

    // Advance physics
    {
        TIME_SCOPE(NameString("Physics_StartSimulate"));

        // PhysX benefits from a scratch buffer for temp allocations
        // We clear it every frame
        memset(scratchBuffer, 0, settings.scratchMemory);
        scene->simulate(gTime.physicsDeltaTime, nullptr, scratchBuffer, settings.scratchMemory);

        // We don't call fetchResults now because we want physics to run parallel to rest of game logic and rendering!
    }
}

void PhysicsSystem::PostPhysicsUpdate(ThreadContext& threadContext, Vault& vault) {

    if (threadContext.IsMainThread())
    {
        TIME_SCOPE(NameString("Physics_FetchResults"));
        gPhysicsScene->fetchResults(true);
    }
    threadContext.Sync();

    {
        // After physics update, now we have to match the transform with the updated rigidBodies
        // as well as update previous transform so it can be used for smooth visual interpolation
        TIME_SCOPE(NameString("Physics_CopyTransform"));

        for (auto& [staticRigidBody, entity] : vault.GetComponents<StaticRigidBody>(threadContext)) {
            const PxTransform transformAfterPhysics = staticRigidBody.body->getGlobalPose();
            Transform& transform = vault.GetComponent<Transform>(entity);
            transform.location = ToDS(transformAfterPhysics.p);
            transform.rotation = ToDS(transformAfterPhysics.q);
        }

        for (auto& [dynamicRigidBody, entity] : vault.GetComponents<DynamicRigidBody>(threadContext)) {
            const PxTransform transformAfterPhysics = dynamicRigidBody.body->getGlobalPose();
            Transform& transform = vault.GetComponent<Transform>(entity);
            dynamicRigidBody.positionLastPhysicsUpdate = transform.location;
            dynamicRigidBody.rotationLastPhysicsUpdate = transform.rotation;
            transform.location = ToDS(transformAfterPhysics.p);
            transform.rotation = ToDS(transformAfterPhysics.q);
        }

        for (auto& [physicsNPC, entity] : vault.GetComponents<PhysicsNPC>(threadContext)) {
            const PxTransform transformAfterPhysics = physicsNPC.dynamicRigidBody.body->getGlobalPose();
            physicsNPC.dynamicRigidBody.positionLastPhysicsUpdate = physicsNPC.transform.location;
            physicsNPC.dynamicRigidBody.rotationLastPhysicsUpdate = physicsNPC.transform.rotation;
            physicsNPC.transform.location = ToDS(transformAfterPhysics.p);
            physicsNPC.transform.rotation = ToDS(transformAfterPhysics.q);
        }
    }
    threadContext.Sync();
}

void PhysicsSystem::LateUpdate(ThreadContext& threadContext, Vault& vault) {

    if (!threadContext.IsMainThread()) {
        return;
    }
    if (settings.enablePhysicsVisualDebugger && scene) {

        PxPvdSceneClient* pvdClient = scene->getScenePvdClient();

        if (!pvdClient) {
            return;
        }

#if WITH_EDITOR
        if (gEditorCamera && gDetachedFromGame) {
            pvdClient->updateCamera("EditorCamera",
                ToPX(gEditorCamera->camera.GetPosition()),
                ToPX(gEditorCamera->camera.GetUp()),
                ToPX(gEditorCamera->camera.GetPosition() + gEditorCamera->camera.GetForward())
            );
        } else {
#endif
            if (gCamera) {
                pvdClient->updateCamera("GameCamera",
                        ToPX(gCamera->GetPosition()),
                        ToPX(gCamera->GetUp()),
                        ToPX(gCamera->GetPosition() + gCamera->GetForward())
                    );
            }
#if WITH_EDITOR
        }
#endif
    }
}

void PhysicsSystem::End(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }
    ASSERT(scene != nullptr && controllerManager != nullptr);
    DestroyPhysicsScene();
}

void PhysicsSystem::AddNewBodiesToPhysicsScene(Vault& vault) {

    // Add newly created rigid bodies in a single batch
    {
        TIME_SCOPE(NameString("Physics_AddBodies"));

        const uint32 numCreatedBodies = vault.GetNumberOfComponents<NewStaticBody>() + vault.GetNumberOfComponents<DynamicRigidBody>();

        if (numCreatedBodies == 0) {
            return;
        }

        TempArray<PxActor*> localBodies(numCreatedBodies);

        for (const auto& [newBody, entity]: vault.GetComponents<NewStaticBody>()) {
            ASSERT(!newBody.body->getScene())
            localBodies.Add(newBody.body);
        }

        for (const auto& [newBody, entity]: vault.GetComponents<NewDynamicBody>()) {
            ASSERT(!newBody.body->getScene())
            localBodies.Add(newBody.body);
        }

        vault.DestroyComponents<NewStaticBody>();
        vault.DestroyComponents<NewDynamicBody>();

        if (!localBodies.IsEmpty()) {
            scene->addActors(localBodies.GetData(), localBodies.GetSize());
        }
    }
}
void PhysicsSystem::CreatePhysicsScene() {

    ASSERT(scene == nullptr && controllerManager == nullptr);

    PxSceneDesc sceneDesc(physics->getTolerancesScale());
    sceneDesc.solverType = PxSolverType::ePGS;
    //sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = dispatcher;
    sceneDesc.filterShader = PhysXFilterShader;
    sceneDesc.simulationEventCallback = &simulationCallback;
    sceneDesc.broadPhaseType = PxBroadPhaseType::ePABP;
    scene = physics->createScene(sceneDesc);

    controllerManager = PxCreateControllerManager(*scene);

    if (settings.enablePhysicsVisualDebugger) {
        if (PxPvdSceneClient* pvdClient = scene->getScenePvdClient()) {
            pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
            pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
            pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
        }
    }

    gPhysicsScene = scene;
    gPhysicsControllerManager = controllerManager;

    if (settings.enablePhysicsVisualDebugger) {
        constexpr const char* PVD_HOST = "127.0.0.1";
        PxPvdTransport* pvdTransport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
        pvd->connect(*pvdTransport, PxPvdInstrumentationFlag::eALL);
    }
}

void PhysicsSystem::DestroyPhysicsScene() {
    PX_RELEASE(controllerManager);
    PX_RELEASE(scene);
    gPhysicsScene = nullptr;
    gPhysicsControllerManager = nullptr;

    for (auto& materialPair : materials) {
        PX_RELEASE(materialPair.value);
    }
    materials.Reset();

    if (settings.enablePhysicsVisualDebugger) {
        pvd->disconnect();
        PxPvdTransport* pvdTransport = pvd->getTransport();
        PX_RELEASE(pvdTransport);
    }
}

const PxMaterial& PhysicsSystem::CreatePhysicsMaterial(
    const NameString& name,
    float staticFriction,
    float dynamicFriction,
    float restitution) {
    ASSERT(!materials.Contains(name));
    PxMaterial* newMaterial = physics->createMaterial(staticFriction, dynamicFriction, restitution);
    materials.Emplace(name, newMaterial);
    return *newMaterial;
}

const PxMaterial& PhysicsSystem::GetExistingMaterial(const NameString& name) {
    ASSERT(materials.Contains(name));
    return *materials[name];
}

#if WITH_EDITOR

void DrawPhysicsStats(const PxPhysics& physics) {
    ImGui::Text("Num Aggregates: %u", physics.getNbAggregates());
    ImGui::Text("Num Articulations: %u", physics.getNbArticulations());
    ImGui::Text("Num BVHs: %u", physics.getNbBVHs());
    ImGui::Text("Num Constraints: %u", physics.getNbConstraints());
    ImGui::Text("Num Convex Meshes: %u", physics.getNbConvexMeshes());

    ImGui::Separator();

    ImGui::Text("Num Deformable Surface Materials: %u", physics.getNbDeformableSurfaceMaterials());
    ImGui::Text("Num Deformable Volume Materials: %u", physics.getNbDeformableVolumeMaterials());
    ImGui::Text("Num FEM Soft Body Materials: %u", physics.getNbFEMSoftBodyMaterials());
    ImGui::Text("Num PBD Materials: %u", physics.getNbPBDMaterials());
    ImGui::Text("Num Materials: %u", physics.getNbMaterials());

    ImGui::Separator();

    ImGui::Text("Num Height Fields: %u", physics.getNbHeightFields());
    ImGui::Text("Num Scenes: %u", physics.getNbScenes());
    ImGui::Text("Num Shapes: %u", physics.getNbShapes());
    ImGui::Text("Num Tetrahedron Meshes: %u", physics.getNbTetrahedronMeshes());
    ImGui::Text("Num Triangle Meshes: %u", physics.getNbTriangleMeshes());
}

void DrawSceneStats(const PxScene& scene) {
    ImGui::Text("Num Actors: %u", scene.getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC));
    ImGui::Text("Num Articulations: %u", scene.getNbArticulations());
    ImGui::Text("Num Constraints: %u", scene.getNbConstraints());
    ImGui::Text("Num Aggregates: %u", scene.getNbAggregates());

    ImGui::Separator(); // Simulation Data

    ImGui::Text("Contact Data Blocks Used: %u", scene.getNbContactDataBlocksUsed());
    ImGui::Text("Max Contact Data Blocks Used: %u", scene.getMaxNbContactDataBlocksUsed());
    ImGui::Text("Num Broadphase Regions: %u", scene.getNbBroadPhaseRegions());

    ImGui::Separator(); // Specialized Bodies

    ImGui::Text("Num Soft Bodies: %u", scene.getNbSoftBodies());
    ImGui::Text("Num Particle Systems: %u", scene.getNbParticleSystems(PxParticleSolverType::ePBD));
    ImGui::Text("Num PBD Particle Systems: %u", scene.getNbPBDParticleSystems());
    ImGui::Text("Num Deformable Surfaces: %u", scene.getNbDeformableSurfaces());
    ImGui::Text("Num Deformable Volumes: %u", scene.getNbDeformableVolumes());
}

void PhysicsSystem::ShowStats() const {

    static bool showPhysicsData = false;

    if (Input::IsKeyHeld(KeyCode::LControl) && Input::IsKeyPressed(KeyCode::Seven)) {
        showPhysicsData = !showPhysicsData;
    }

    if (ImGui::MainMenuBarItem("Stats", "Physics Stats", "CTRL+7", &showPhysicsData)) {
        ImGui::Begin("Physics Data", &showPhysicsData);

        static bool inMB = false;
        const size_t unitConversion = inMB ? 1025 * 1024 : 1024;
        const char* unitName = inMB ? "Mb" : "Kb";

        const size_t usedMemory = allocator.GetUsedMemory();
        const size_t freeMemory = allocator.GetFreeMemory();
        ImGui::Text("Memory Used: %d %s", usedMemory / unitConversion, unitName);
        ImGui::Text("Memory Free: %d %s", freeMemory / unitConversion, unitName);
        ImGui::Checkbox("In Megabytes", &inMB);

        //ImGui::Separator();
//
        //DrawPhysicsStats(*physics);
//
        //ImGui::Separator();
//
        //DrawSceneStats(*scene);
//
        ImGui::End();
    }
}
#endif
