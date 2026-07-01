#include "Game/Systems/WeaponEditorSystem.hpp"

#include <Core/Time.hpp>
#include <Game/PhysicsCollisionGroups.hpp>
#include <Physics/Components/DynamicRigidBody.hpp>
#include <Physics/Components/StaticRigidBody.hpp>
#include <Physics/Components/TriggerEvent.hpp>
#include <Physics/PhysicsCommon.hpp>
#include <PxPhysicsAPI.h>
#include <Renderer/Components/PrimitiveRenderer.hpp>
#include <Runtime/Components/PhysicsNPC.hpp>
#include <Runtime/Input.hpp>
#include <Math/Mat3.hpp>

#include "Game/Components/FollowEntity.hpp"
#include "Game/Components/GameState.hpp"
#include "Game/Components/Handles/ShootV1Handle.hpp"
#include "Game/Components/Handles/ShootV2Handle.hpp"
#include "Game/Components/Handles/ShootV3Handle.hpp"
#include "Game/Components/Handles/ShootV4Handle.hpp"
#include "Game/Components/Handles/SwordHandle.hpp"
#include "Game/Components/Modules/AirModule.hpp"
#include "Game/Components/Modules/ChainModule.hpp"
#include "Game/Components/Modules/PistonModule.hpp"
#include "Game/Components/Modules/RotatorModule.hpp"
#include "Game/Components/Modules/ShapeModule.hpp"
#include "Game/Components/Modules/ShootModule.hpp"
#include "Game/Components/Player.hpp"
#include "Game/Components/WeaponBlueprint.hpp"
#include "Game/Components/WeaponEditor.hpp"
#include "Game/Components/WeaponEditorCamera.hpp"
#include "Game/Components/WeaponRuntime.hpp"

static void SpawnPlayerPreview(Vault& vault, WeaponEditor& weaponEditor) {

    const Entity playerEntity = vault.CreateEntity("Body", {"WeaponEditor, PlayerPreview"});
    weaponEditor.playerPreviewEntities[0] = playerEntity;

    Transform& transform = vault.EmplaceComponent<Transform>(playerEntity);
    transform.location = WEAPON_EDITOR_LOCATION;

    PrimitiveRenderer& renderer = vault.EmplaceComponent<PrimitiveRenderer>(playerEntity);
    renderer.primitiveType = PrimitiveType::Capsule;
    renderer.color = Vec3(0.f, 0.f, 1.f);

    const Vec3 handOffset = Vec3(0.2f, 0.f, 0.f);
    // Left hand
    {
        const Entity leftHand = vault.CreateEntity("LeftHand", {"WeaponEditor, PlayerPreview"});
        weaponEditor.playerPreviewEntities[1] = leftHand;

        Transform& handTransform = vault.EmplaceComponent<Transform>(leftHand);
        handTransform.location = transform.location + transform.scale.x*.5f + handOffset;
        handTransform.scale = Vec3(.3f);

        PrimitiveRenderer& handRenderer = vault.EmplaceComponent<PrimitiveRenderer>(leftHand);
        handRenderer.primitiveType = PrimitiveType::Sphere;
        handRenderer.color = Vec3(0.f, 0.f, 1.f);
    }

    // Left hand
    {
        const Entity rightHand = vault.CreateEntity("RightHand", {"WeaponEditor, PlayerPreview"});
        weaponEditor.playerPreviewEntities[2] = rightHand;

        Transform& handTransform = vault.EmplaceComponent<Transform>(rightHand);
        handTransform.location = transform.location - transform.scale.x*.5f - handOffset;
        handTransform.scale = Vec3(.3f);

        PrimitiveRenderer& handRenderer = vault.EmplaceComponent<PrimitiveRenderer>(rightHand);
        handRenderer.primitiveType = PrimitiveType::Sphere;
        handRenderer.color = Vec3(0.f, 0.f, 1.f);
    }
}

static void SpawnHandlePreview(Vault& vault, WeaponEditor& weaponEditor) {
    const Entity handlePreview = vault.CreateEntity("WeaponEditorHandle", { "WeaponEditor" });

    Transform& transform = vault.EmplaceComponent<Transform>(handlePreview, WEAPON_EDITOR_LOCATION);
    transform.scale = GetHandleScale(weaponEditor.selectedHandleType);
    transform.rotation = GetHandlePreviewRotation(weaponEditor.selectedHandleType);

    const HandleVisuals handleVisuals = GetHandleVisuals(weaponEditor.selectedHandleType);
    vault.EmplaceComponent<PrimitiveRenderer>(handlePreview, handleVisuals.primitiveType, handleVisuals.color);

    StaticRigidBody& staticRigidBody = vault.EmplaceComponent<StaticRigidBody>(handlePreview, ToPX(transform));
    staticRigidBody.AddExclusiveShape(PxBoxGeometry(transform.scale.x * .5f, transform.scale.y * .5f, transform.scale.z * .5f));

    weaponEditor.handleEntity = handlePreview;
}

static void OnDestroyAirModule(Entity, AirModuleVisuals& airModuleVisuals, Vault& vault) {
    // When visuals get destroyed, we destroy its trigger too
    if (vault.IsEntityValid(airModuleVisuals.triggerEntity)) {
        vault.DestroyEntity(airModuleVisuals.triggerEntity);
    }
}

void WeaponEditorSystem::Start(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }
    vault.RegisterOnDestroy<AirModuleVisuals, OnDestroyAirModule>();

    const Entity weaponEditorEntity = vault.CreateEntity("Weapon Editor", { "WeaponEditor" });
    WeaponEditor& weaponEditor = vault.EmplaceComponent<WeaponEditor>(weaponEditorEntity);
    weaponEditor.modules.EmplaceMany(MAX_MODULES);

    vault.EmplaceComponent<Transform>(weaponEditorEntity);

    PrimitiveRenderer& primitiveRenderer = vault.EmplaceComponent<PrimitiveRenderer>(weaponEditorEntity);
    primitiveRenderer.isVisible = false;

    SpawnHandlePreview(vault, weaponEditor);
}

static void ResetWeaponRuntime(Vault& vault, WeaponRuntime& runtime) {

    vault.DestroyEntity(runtime.handleEntity);
    runtime.handleEntity = ECS::InvalidEntity;

    for (Entity& entity : runtime.moduleEntities) {
        vault.DestroyEntity(entity);
        entity = ECS::InvalidEntity;
    }

    runtime.moduleEntities.Reset();
}

static void OnAirModuleTriggerCallback(Vault& vault, const TriggerEvent& triggerEvent) {

    const Entity airModuleEntity = ToEntity(*triggerEvent.triggerActor);
    const Entity entityInsideTrigger = ToEntity(*triggerEvent.actorInside);

    if (vault.IsEntityValid(entityInsideTrigger)) {
        if (vault.HasComponent<DynamicRigidBody>(entityInsideTrigger) || vault.HasComponent<PhysicsNPC>(entityInsideTrigger)) {
            AirModule& airModule = vault.GetComponent<AirModule>(airModuleEntity);
            if (triggerEvent.enteredTrigger) {
                airModule.entitiesWithinAirVolume.Add(entityInsideTrigger);
            } else {
                airModule.entitiesWithinAirVolume.Remove(entityInsideTrigger);
            }
        }
    }
}

static void SpawnModule(Vault& vault, const ModuleSlotEditor& moduleSlot, WeaponRuntime& weaponRuntime, Entity parentEntity, const Transform& parentTransform) {

    const Entity moduleEntity = vault.CreateEntity(GetModuleName(moduleSlot.type) + NameString("Module"), { "Weapon" });

    const Vec3 moduleScale = GetModuleScale(moduleSlot.type) * moduleSlot.scale;
    const PxTransform transform = ToPX(parentTransform) * ToPX(moduleSlot.transform);
    const Transform& moduleTransform = vault.EmplaceComponent<Transform>(moduleEntity, ToDS(transform.p), moduleScale, ToDS(transform.q));

    PrimitiveRenderer& moduleRenderer = vault.EmplaceComponent<PrimitiveRenderer>(moduleEntity);

    const ModuleVisual moduleVisual = GetModuleVisuals(moduleSlot.type);
    moduleRenderer.primitiveType = moduleVisual.primitiveType;
    moduleRenderer.color = moduleVisual.color;

    vault.EmplaceComponent<FollowEntity>(moduleEntity, moduleSlot.transform.location, moduleSlot.transform.rotation, parentEntity);

    switch (moduleSlot.type) {
        case ModuleType::Air: {
            AirModuleVisuals& airModuleVisuals = vault.EmplaceComponent<AirModuleVisuals>(moduleEntity);
            airModuleVisuals.triggerEntity = vault.CreateEntity("AirModuleTrigger", {"Weapon"});

            AirModule& airModule = vault.EmplaceComponent<AirModule>(airModuleVisuals.triggerEntity);
            vault.EmplaceComponent<FollowEntity>(airModuleVisuals.triggerEntity, Vec3::UP * airModule.size.y * .5f, Quat::IDENTITY, moduleEntity);
            vault.EmplaceComponent<Transform>(airModuleVisuals.triggerEntity);

            DynamicRigidBody& rigidBody = vault.EmplaceComponent<DynamicRigidBody>(airModuleVisuals.triggerEntity, moduleTransform);
            rigidBody.AddExclusiveShape(PxBoxGeometry(airModule.size.x * .5f, airModule.size.y * .5f, airModule.size.z * .5f), *gDefaultPhysicsMaterial, PxShapeFlag::eTRIGGER_SHAPE);
            rigidBody.SetIsKinematic(true);
            rigidBody.SetCollisionGroup(CollisionGroup::Environment, CollisionGroup::Enemy | CollisionGroup::EnemySmall | CollisionGroup::Projectile);
            rigidBody.receiveTriggerEvents = true;
            break;
        }
        case ModuleType::Shoot: {
            ShootModule& shootModule = vault.EmplaceComponent<ShootModule>(moduleEntity);
            shootModule.RandomizeIntervalOffset();
            break;
        }
        case ModuleType::Rotator:
            vault.EmplaceComponent<RotatorModule>(moduleEntity, moduleSlot.transform.location, moduleSlot.transform.rotation);
            break;
        case ModuleType::Chain:
            break;
        case ModuleType::Piston:
            vault.EmplaceComponent<PistonModule>(moduleEntity, moduleSlot.transform.location, moduleSlot.transform.rotation);
            break;
        case ModuleType::Shape:
            break;
    }

    weaponRuntime.moduleEntities.Add(moduleEntity);

    for (const ModuleSlotEditor* childModuleSlot : moduleSlot.childModuleNodes) {
        SpawnModule(vault, *childModuleSlot, weaponRuntime, moduleEntity, moduleTransform);
    }
}

static Transform SpawnHandle(Vault& vault, WeaponEditor& weaponEditor, WeaponRuntime& weaponRuntime) {

    weaponRuntime.handleEntity = vault.CreateEntity("WeaponHandle", { "Weapon" });
    weaponRuntime.handleType = weaponEditor.selectedHandleType;

    Transform& transform = vault.EmplaceComponent<Transform>(weaponRuntime.handleEntity);
    transform.scale = GetHandleScale(weaponEditor.selectedHandleType);
    transform.location = GetHandleOffset(weaponEditor.selectedHandleType);
    transform.rotation = GetHandleRotation(weaponEditor.selectedHandleType);

    PrimitiveRenderer& renderer = vault.EmplaceComponent<PrimitiveRenderer>(weaponRuntime.handleEntity);
    const HandleVisuals handleVisuals = GetHandleVisuals(weaponEditor.selectedHandleType);
    renderer.primitiveType = handleVisuals.primitiveType;
    renderer.color = handleVisuals.color;

    switch (weaponEditor.selectedHandleType) {

        case HandleType::Sword:
            vault.EmplaceComponent<SwordHandle>(weaponRuntime.handleEntity);
            break;
        case HandleType::ShootV1:
            vault.EmplaceComponent<ShootV1Handle>(weaponRuntime.handleEntity);
            break;
        case HandleType::ShootV2:
            vault.EmplaceComponent<ShootV2Handle>(weaponRuntime.handleEntity);
            break;
        case HandleType::ShootV3:
            vault.EmplaceComponent<ShootV3Handle>(weaponRuntime.handleEntity);
            break;
        case HandleType::ShootV4:
            vault.EmplaceComponent<ShootV4Handle>(weaponRuntime.handleEntity);
            break;
    }

    return transform;
}

static void SpawnWeapon(Vault& vault, WeaponEditor& weaponEditor) {

    const auto& [player, playerEntity] = *vault.GetComponents<Player>().begin();

    // Clean up existing weapon if any
    WeaponRuntime* existingRuntime = vault.TryGetComponent<WeaponRuntime>(playerEntity);
    if (existingRuntime) {
        ResetWeaponRuntime(vault, *existingRuntime);
    } else {
        existingRuntime = &vault.EmplaceComponent<WeaponRuntime>(playerEntity);
    }

    WeaponRuntime& weaponRuntime = *existingRuntime;

    const Transform& handleTransform = SpawnHandle(vault, weaponEditor, weaponRuntime);

    for (ModuleSlotEditor& moduleSlotEditor : weaponEditor.modules) {

        if (moduleSlotEditor.entity == ECS::InvalidEntity) {
            continue;
        }

        // Is this module attached to the handle?
        if (moduleSlotEditor.parentModule == nullptr) {
            SpawnModule(vault, moduleSlotEditor, weaponRuntime, weaponRuntime.handleEntity, handleTransform);
        }
    }
}

static void DrawModuleSelectionUI(WeaponEditor& weaponEditor, bool& mousePressed) {
#if WITH_EDITOR
    const ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    ImGui::SetNextWindowBgAlpha(0.7f);
    ImGui::SetNextWindowSizeConstraints(ImVec2(250, 0), ImVec2(FLT_MAX, FLT_MAX));

    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("Choose module", nullptr, windowFlags);
    const Vec2 cursorPos = Input::GetCursorPosition();
    const bool isInsideWindow = ImGui::IsInsideWindow(cursorPos);
    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 2.0f);

    for (uint32 i = 0; i <= static_cast<uint32>(ModuleType::Last); i++) {

        const ModuleType moduleType = static_cast<ModuleType>(i);

        bool selected = weaponEditor.selectedModuleType == moduleType;
        ImGui::Checkbox(GetModuleName(moduleType), &selected);

        const bool isHovered = ImGui::IsInsidePreviousItem(cursorPos);
        if (isHovered) {
            const ImVec2 rectMin = ImGui::GetItemRectMin();
            const ImVec2 rectMax = ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddRectFilled(rectMin, rectMax, IM_COL32(255, 255, 255, 40));
        }

        if (mousePressed && isHovered) {
            if (!selected) {
                weaponEditor.selectedModuleType = moduleType;
            }
            mousePressed = false; // Consume mouse press
        }
    }

    ImGui::PopFont();

    // Show module count and instructions
    ImGui::Separator();
    ImGui::Text("Modules: %u / %u", weaponEditor.numModules, MAX_MODULES);
    ImGui::Text("Press Enter to build weapon");

    const ImVec2 windowSize = ImGui::GetWindowSize();
    ImGui::SetWindowPos(ImVec2(0, (displaySize.y - windowSize.y) * 0.5f));

    ImGui::End();

    if (isInsideWindow) {
        return;
    }
#endif
}

static bool DrawHandleSelectionUI(WeaponEditor& weaponEditor, bool& mousePressed) {
    bool handleChanged = false;
#if WITH_EDITOR
    const ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    ImGui::SetNextWindowBgAlpha(0.7f);
    ImGui::SetNextWindowSizeConstraints(ImVec2(250, 0), ImVec2(FLT_MAX, FLT_MAX));

    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("Choose handle", nullptr, windowFlags);
    const Vec2 cursorPos = Input::GetCursorPosition();
    const bool isInsideWindow = ImGui::IsInsideWindow(cursorPos);
    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 2.0f);

    for (uint32 i = 0; i <= static_cast<uint32>(HandleType::Last); i++) {

        const HandleType handleType = static_cast<HandleType>(i);

        bool selected = weaponEditor.selectedHandleType == handleType;
        ImGui::Checkbox(GetHandleName(handleType), &selected);

        const bool isHovered = ImGui::IsInsidePreviousItem(cursorPos);
        if (isHovered) {
            const ImVec2 rectMin = ImGui::GetItemRectMin();
            const ImVec2 rectMax = ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddRectFilled(rectMin, rectMax, IM_COL32(255, 255, 255, 40));
        }

        if (mousePressed && isHovered) {
            if (!selected) {
                handleChanged = true;
                weaponEditor.selectedHandleType = handleType;
            }
            mousePressed = false; // Consume mouse press
        }
    }

    ImGui::PopFont();

    // Show module count and instructions
    ImGui::Separator();

    const ImVec2 windowSize = ImGui::GetWindowSize();
    ImGui::SetWindowPos(ImVec2(0, displaySize.y - windowSize.y));

    ImGui::End();

    if (isInsideWindow) {
        return handleChanged;
    }
#endif
    return handleChanged;
}

void UpdateWeaponEditorVisibility(Vault& vault, Entity weaponEditorEntity, WeaponEditor& weaponEditor, bool newVisibility) {

    // Update preview module visibility
    if (PrimitiveRenderer* primitiveRenderer = vault.TryGetComponent<PrimitiveRenderer>(weaponEditorEntity)) {
        primitiveRenderer->isVisible = newVisibility;
    }

    // Update attached modules visibility
    for (ModuleSlotEditor& module: weaponEditor.modules) {
        if (module.entity == ECS::InvalidEntity) {
            continue;
        }

        if (PrimitiveRenderer* primitiveRenderer = vault.TryGetComponent<PrimitiveRenderer>(module.entity)) {
            primitiveRenderer->isVisible = newVisibility;
        }
    }

    // Update handle visibility
    if (weaponEditor.handleEntity != ECS::InvalidEntity) {
        if (PrimitiveRenderer* primitiveRenderer = vault.TryGetComponent<PrimitiveRenderer>(weaponEditor.handleEntity)) {
            primitiveRenderer->isVisible = newVisibility;
        }
    }
}

void WeaponEditorSystem::Update(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }

    auto& [gameState, gameStateEntity] = *vault.GetComponents<GameState>().begin();
    auto& [weaponEditor, weaponEditorEntity] = *vault.GetComponents<WeaponEditor>().begin();

    if (!gameState.isBuildingWeapon) {
        UpdateWeaponEditorVisibility(vault, weaponEditorEntity, weaponEditor, false);
        return;
    }

    UpdateWeaponEditorVisibility(vault, weaponEditorEntity, weaponEditor, true);

    // Module selection via number keys
    if (Input::IsKeyPressed(KeyCode::One)) {
        weaponEditor.selectedModuleType = ModuleType::Air;
    }
    else if (Input::IsKeyPressed(KeyCode::Two)) {
        weaponEditor.selectedModuleType = ModuleType::Shoot;
    }
    else if (Input::IsKeyPressed(KeyCode::Three)) {
        weaponEditor.selectedModuleType = ModuleType::Rotator;
    }
    else if (Input::IsKeyPressed(KeyCode::Four)) {
        weaponEditor.selectedModuleType = ModuleType::Chain;
    }
    else if (Input::IsKeyPressed(KeyCode::Five)) {
        weaponEditor.selectedModuleType = ModuleType::Piston;
    }
    else if (Input::IsKeyPressed(KeyCode::Six)) {
        weaponEditor.selectedModuleType = ModuleType::Shape;
    }

    // Done editing press Enter to finalize weapon
    if (Input::IsKeyPressed(KeyCode::Return) && weaponEditor.numModules > 0) {
        SpawnWeapon(vault, weaponEditor);
        return;
    }

    if (Input::IsKeyDown(KeyCode::Q)) {
        weaponEditor.moduleScale -= 3.f * gTime.deltaTime;
    }
    else if (Input::IsKeyDown(KeyCode::W)) {
        weaponEditor.moduleScale += 3.f * gTime.deltaTime;
    }

    weaponEditor.moduleScale = std::clamp(weaponEditor.moduleScale, 0.3f, 3.f);

    bool mousePressed = Input::IsMouseButtonPressed(MouseButtonCode::Left);

    const bool handleTypeChanged = DrawHandleSelectionUI(weaponEditor, mousePressed);

    DrawModuleSelectionUI(weaponEditor, mousePressed);

    if (handleTypeChanged) {
        weaponEditor.RemoveModuleAt(vault, 0);
        vault.DestroyEntity(weaponEditor.handleEntity);
        SpawnHandlePreview(vault, weaponEditor);
    }

    const ModuleVisual moduleVisual = GetModuleVisuals(weaponEditor.selectedModuleType);
    const Vec3 moduleScale = GetModuleScale(weaponEditor.selectedModuleType) * weaponEditor.moduleScale;

    auto& [camera, weaponEditorCameraEntity] = *vault.GetComponents<WeaponEditorCamera>().begin();

    const bool pressedDelete = Input::IsKeyPressed(KeyCode::D);
    const Vec2 cursorPosition = Input::GetCursorPosition();
    const Vec3 rayDirection = camera.camera.DirectionTowardsScreen(cursorPosition);

    RayCastHit hit;
    if (Physics::RayCast(camera.camera.GetPosition(), rayDirection, 100000.f, hit)) {
        const Entity hitEntity = hit.entity;

        int32 hitModuleIndex = -1;

        // Check if we hit an already-placed module
        for (uint32 i = 0; i < weaponEditor.modules.GetSize(); i++) {
            if (weaponEditor.modules[i].entity == hitEntity) {
                hitModuleIndex = static_cast<int32>(i);
                break;
            }
        }

        if (pressedDelete && hitModuleIndex >= 0) {
            weaponEditor.RemoveModuleAt(vault, hitModuleIndex);
            return;
        }

        ModuleSlotEditor* parentModule = nullptr;
        if (hitModuleIndex >= 0) {
            parentModule = &weaponEditor.modules[hitModuleIndex];
        }

        // Cannot attach to shoot module :)
        const bool bHitValidParent =
            hitEntity != ECS::InvalidEntity
            && ((parentModule && parentModule->type != ModuleType::Shoot && parentModule->type != ModuleType::Air)
            || hitEntity == weaponEditor.handleEntity);

        // Snapping logic
        if (!Input::IsKeyHeld(KeyCode::LAlt)) {

            if (Transform* hitEntityTransform = vault.TryGetComponent<Transform>(hit.entity)) {
                // Here we cast a ray towards the already found target such that we hit the center of the face.
                Physics::RayCastSingleTarget(hitEntityTransform->location + hit.normal * 50.f, -hit.normal, 60.f, hit.actor, hit);
            }
        }

        const Vec3 up = hit.normal;

        Vec3 refVector = Vec3::UP;
        if (PxAbs(Dot(refVector, up)) > .99f) {
            refVector = Vec3::FORWARD;
        }
        else if (PxAbs(Dot(refVector, up)) > .99f) {
            refVector = Vec3::RIGHT;
        }

        const Vec3 right = Cross(up, refVector).Normalize();
        const Vec3 forward = Cross(right, up);
        const Quat rotation = Mat3(forward, right, up).ToQuaternion();

        // Update preview cursor
        Transform& previewTransform = vault.GetComponent<Transform>(weaponEditorEntity);
        previewTransform.location = hit.position + hit.normal * (moduleScale.x * .5f);
        previewTransform.rotation = rotation;
        previewTransform.scale = moduleScale;

        PrimitiveRenderer& previewRenderer = vault.GetComponent<PrimitiveRenderer>(weaponEditorEntity);
        previewRenderer.primitiveType = moduleVisual.primitiveType;
        previewRenderer.color = bHitValidParent ? moduleVisual.color : Vec3(1., 0., 0.);
        previewRenderer.alpha = .5f;
        previewRenderer.isVisible = true;

        if (mousePressed && bHitValidParent && weaponEditor.numModules < MAX_MODULES) {

            // Spawn preview entity with a static rigid body so future raycasts can hit it
            const Entity moduleEntity = vault.CreateEntity(GetModuleName(weaponEditor.selectedModuleType) + NameString("ModulePreview"), { "WeaponEditor" });

            const Transform& modulePreviewTransform = vault.GetComponent<Transform>(weaponEditorEntity);

            Transform& moduleTransform = vault.EmplaceComponent<Transform>(moduleEntity);
            moduleTransform.location = modulePreviewTransform.location;
            moduleTransform.rotation = modulePreviewTransform.rotation;
            moduleTransform.scale = modulePreviewTransform.scale;

            PrimitiveRenderer& primitiveRenderer = vault.EmplaceComponent<PrimitiveRenderer>(moduleEntity);
            primitiveRenderer.color = moduleVisual.color;
            primitiveRenderer.primitiveType = moduleVisual.primitiveType;

            // Add a static rigid body so this module can be a raycast target for future placements
            StaticRigidBody& staticBody = vault.EmplaceComponent<StaticRigidBody>(moduleEntity, ToPX(moduleTransform));

            switch (weaponEditor.selectedModuleType) {
                case ModuleType::Air : {
                    staticBody.AddExclusiveShape(PxSphereGeometry(moduleScale.x * .5f));
                    break;
                }
                case ModuleType::Shoot : {
                    staticBody.AddExclusiveShape(PxSphereGeometry(moduleScale.x * .5f));
                    break;
                }
                case ModuleType::Rotator : {
                    staticBody.AddExclusiveShape(PxBoxGeometry(moduleScale.x * .5f, moduleScale.y * .5f, moduleScale.z * .5f));
                    break;
                }
                case ModuleType::Chain : {
                    staticBody.AddExclusiveShape(PxSphereGeometry(moduleScale.x * .5f));
                    break;
                }
                case ModuleType::Piston : {
                    staticBody.AddExclusiveShape(PxBoxGeometry(moduleScale.x * .5f, moduleScale.y * .5f, moduleScale.z * .5f));
                    break;
                }
                case ModuleType::Shape : {
                    staticBody.AddExclusiveShape(PxBoxGeometry(moduleScale.x * .5f, moduleScale.y * .5f, moduleScale.z * .5f));
                    staticBody.AddExclusiveShape(PxBoxGeometry(moduleScale.x * .5f, moduleScale.y * .5f, moduleScale.z * .5f));
                    break;
                }
                default:;
            }

            const Transform& hitEntityTransform = vault.GetComponent<Transform>(hitEntity);
            const Transform localTransform = ToDS( ToPX(hitEntityTransform).getInverse() * ToPX(moduleTransform));
            weaponEditor.AddModule(moduleEntity, weaponEditor.selectedModuleType, localTransform, parentModule);
        }
    } else {
        PrimitiveRenderer& previewRenderer = vault.GetComponent<PrimitiveRenderer>(weaponEditorEntity);
        previewRenderer.isVisible = false;
    }
}