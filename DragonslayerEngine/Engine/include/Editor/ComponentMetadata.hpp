#pragma once

#include <concepts>

#include "Core/Containers/Array.hpp"
#include "Core/String.hpp"
#include "ECS/Entity.hpp"

template<typename Component>
concept HasDisplayFunction = requires(Component& component) {
    { component.OnHierarchy() } -> std::same_as<void>;
};

#define VALIDATE_HIERARCHY_DISPLAY(Component) \
    static_assert(!IsDisplayable<Component> || HasDisplayFunction<Component>, \
    #Component" needs to implement: void OnHierarchy()");

struct ComponentMetadata {
    NameString componentName;
    void (*onEditorUI)(void* component);
    void* (*getComponent)(class Vault& vault, Entity entity);
    struct ComponentFlags (*getComponentFlags)();
};

// TODO Figure out how this can be moved to editor globals
// Declared as a function instead of variable because c++ standard does not guarantee
// construction order across translation units, with a static variable inside a function
// we can enforce the initialization when the function gets called the first time
ENGINE_API InlineArray<ComponentMetadata, 1024>& GetComponentsMetadata();

// NOTE: In hot-reloading mode the initialization will be called multiple times,
// once for the Runtime and every time Game.dll gets loaded, this is fine since
// we have a loop removing existing metadata, this ensures no duplicates exist.
// It could be fixed by declaring the metadata initializer variable as extern
// but then we would be forced to have a .cpp to declare them which is not ideal
#define END_METADATA_COMPONENT(Component) \
    VALIDATE_HIERARCHY_DISPLAY(Component) \
    struct Component##MetadataInitializer { \
        FORCE_INLINE static void OnEditorUI(void* component) { \
            static_cast<Component*>(component)->OnEditorUI(); \
        } \
        NO_DISCARD FORCE_INLINE static void* GetComponent(Vault& vault, Entity entity) { \
            if (!vault.ContainsComponentPool<Component>()) { return nullptr; } \
            return reinterpret_cast<void*>(vault.TryGetComponent<Component>(entity)); \
        } \
        Component##MetadataInitializer() { \
            if constexpr (HasMetadata<Component>) { \
                uint32 idx = 0; \
                auto& componentsMetadata = GetComponentsMetadata(); \
                for (const ComponentMetadata& componentMetadata : componentsMetadata) { \
                    if (componentMetadata.componentName == Component::GetComponentName()) { \
                        componentsMetadata.RemoveAndSwapAt(idx); \
                        break; \
                    } \
                    ++idx; \
                } \
                ComponentMetadata& metadata = componentsMetadata.Emplace(); \
                metadata.componentName = NameString(Component::GetComponentName()); \
                if constexpr (IsDisplayable<Component>) { \
                    metadata.onEditorUI = &OnEditorUI; \
                } else { \
                    metadata.onEditorUI  = nullptr; \
                } \
                metadata.getComponent = &GetComponent; \
                metadata.getComponentFlags = &Component::GetComponentFlags; \
            } \
        } \
    }; \
    static const Component##MetadataInitializer Component##MetadataInitializer;

#define GENERATE_METADATA_BODY(Component) \
public: \
    friend struct Component##MetadataInitializer; \
    /* Helper to delay name lookup of OnHierarchy from Preprocessing to Template instantiation */ \
    template<typename T = Component> \
    void CallOnHierarchy(T* instance) { \
        instance->OnHierarchy(); \
    } \
    void OnEditorUI() { \
        if constexpr (IsDisplayable<Component>) { \
            static bool isOpen = true; \
            ImGui::SetNextItemOpen(isOpen); \
            if (ImGui::TreeNodeEx(#Component, ImGuiTreeNodeFlags_Framed)) { \
                isOpen = true; \
                CallOnHierarchy(this); \
                ImGui::TreePop(); \
            } else { \
                isOpen = false; \
            } \
        } \
    } \
    NO_DISCARD FORCE_INLINE static const char* GetComponentName() { \
        return #Component; \
    }
