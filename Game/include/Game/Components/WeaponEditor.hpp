#pragma once

#include <ECS/Component.hpp>

#include <Runtime/Components/Transform.hpp>

#include "Game/Components/WeaponBlueprint.hpp"

inline const Vec3 WEAPON_EDITOR_LOCATION = Vec3(0.f, -500.f, 0.f);

struct ModuleSlotEditor {
    Entity entity = ECS::InvalidEntity;
    ModuleType type = ModuleType::Shape;
    Transform transform;
    float scale = 1.f;
    ModuleSlotEditor* parentModule = nullptr;
    InlineArray<ModuleSlotEditor*, MAX_MODULES> childModuleNodes;

    bool operator==(const ModuleSlotEditor& other) const {
        return entity == other.entity;
    }
};

DSTRUCT(WeaponEditor, .numComponents = 1, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DSTRUCT_BODY(WeaponEditor)

    InlineArray<Entity, 3> playerPreviewEntities = { ECS::InvalidEntity, ECS::InvalidEntity, ECS::InvalidEntity };
    InlineArray<ModuleSlotEditor, MAX_MODULES> modules;
    uint32 numModules = 0;
    Entity handleEntity = ECS::InvalidEntity;

    HandleType selectedHandleType = HandleType::ShootV1;
    ModuleType selectedModuleType = ModuleType::Shape;

    float moduleScale = 1.f;

    ModuleSlotEditor* AddModule(const Entity& entity, ModuleType type, const Transform& transform, ModuleSlotEditor* parentModule) {
        for (ModuleSlotEditor& moduleSlot : modules) {
            if (moduleSlot.entity == ECS::InvalidEntity) {
                moduleSlot.entity = entity;
                moduleSlot.type = type;
                moduleSlot.transform = transform;
                moduleSlot.scale = moduleScale;

                if (parentModule) {
                    moduleSlot.parentModule = parentModule;
                    moduleSlot.parentModule->childModuleNodes.Add(&moduleSlot);
                } else {
                    moduleSlot.parentModule = nullptr;
                }

                ++numModules;
                return &moduleSlot;
            }
        }

        return nullptr;
    }

    void InvalidateModule(ModuleSlotEditor& module, Vault& vault) {
        vault.DestroyEntity(module.entity);
        module.parentModule = nullptr;
        module.childModuleNodes.Reset();
        module.entity = ECS::InvalidEntity;
        --numModules;
    }

    void RemoveChildModules(ModuleSlotEditor& moduleSlotEditor, Vault& vault) {

        for (ModuleSlotEditor* childModuleSlot : moduleSlotEditor.childModuleNodes) {
            RemoveChildModules(*childModuleSlot, vault);
            InvalidateModule(*childModuleSlot, vault);
        }
    }

    void RemoveModuleAt(Vault& vault, uint32 index) {

        if (numModules == 0) {
            return;
        }

        ModuleSlotEditor& module = modules[index];

        // Remove the module from parent list if it has a parent
        if (module.parentModule) {
            module.parentModule->childModuleNodes.Remove(&module);
        }

        // Remove child modules
        RemoveChildModules(module, vault);

        // Remove current module
        InvalidateModule(module, vault);
    }


#if WITH_EDITOR
    void OnHierarchy() {
        ImGui::Text("Selected Module: %d", selectedModuleType);
        ImGui::Text("NumModules: %d", numModules);
    }
#endif

}; END_DSTRUCT(WeaponEditor)
