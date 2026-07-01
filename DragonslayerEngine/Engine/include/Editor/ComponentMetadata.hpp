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
    static_assert(!IsDisplayable<Component> || \
    (std::is_default_constructible_v<Component> && HasDisplayFunction<Component>), \
    #Component" must be default constructible and needs to implement: void OnHierarchy()");

struct ComponentMetadata {
    NameString componentName; // NOTE: This implies components MUST have a name of less than 64 characters (incl null terminator)
    bool (*onEditorUI)(void* component);
    void (*createComponent)(class Vault& vault, Entity entity);
    void (*destroyComponent)(class Vault& vault, Entity entity);
    void* (*getComponent)(class Vault& vault, Entity entity);
    struct ComponentFlags (*getComponentFlags)();
};

// TODO Figure out how this can be moved to editor globals
// TODO Figure out a way of passing the MAX_COMPONENTS instead of 1024
// Declared as a function instead of variable because c++ standard does not guarantee
// construction order across translation units, with a static variable inside a function
// we can enforce the initialization when the function gets called the first time
ENGINE_API InlineArray<ComponentMetadata, 1024>& GetComponentsMetadata();

#define GENERATE_METADATA_BODY(Component) \
public: \
    friend struct Component##MetadataInitializer; \
    /* Helper to delay name lookup of OnHierarchy from Preprocessing to Template instantiation */ \
    template<typename T = Component> \
    void CallOnHierarchy(T* instance) { \
        instance->OnHierarchy(); \
    } \
    bool OnEditorUI() { \
        if constexpr (IsDisplayable<Component>) { \
            static bool isOpen = true; \
            ImGui::PushID(TypeHash<Component>()); \
            ImGui::SetNextItemOpen(isOpen); \
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowOverlap; \
            bool nodeOpen = ImGui::TreeNodeEx(#Component, flags); \
            ImGui::SameLine(); \
            const float buttonWidth = 30.0f; \
            const float posX = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - buttonWidth; \
            ImGui::SetCursorPosX(posX); \
            bool removedComponent = false; \
            if (ImGui::Button("X##RemoveButton", ImVec2(buttonWidth, 0))) { \
                removedComponent = true; \
            } \
            if (nodeOpen) { \
                isOpen = true; \
                CallOnHierarchy(this); \
                ImGui::TreePop(); \
            } else { \
                isOpen = false; \
            } \
            ImGui::PopID(); \
            return removedComponent; \
        } \
        return false; \
    } \
    NO_DISCARD FORCE_INLINE static const char* GetComponentName() { \
        return #Component; \
    }

// NOTE: In hot-reloading mode the initialization will be called multiple times,
// once for the Runtime and every time Game.dll gets loaded, this is fine since
// we have a loop removing existing metadata, this ensures no duplicates exist.
// It could be fixed by declaring the metadata initializer variable as extern
// but then we would be forced to have a .cpp to declare them which is not ideal
#define END_METADATA_COMPONENT(Component) \
    VALIDATE_HIERARCHY_DISPLAY(Component) \
    struct Component##MetadataInitializer { \
        FORCE_INLINE static bool OnEditorUI(void* component) { \
            return static_cast<Component*>(component)->OnEditorUI(); \
        } \
        /* Use of template here to delay name lookup of EmplaceComponent<Component> from Preprocessing to Template instantiation */ \
        template<typename T = Component> \
        FORCE_INLINE static void CreateComponent(Vault& vault, Entity entity) { \
            vault.EmplaceComponent<T>(entity); \
        } \
        template<typename T = Component> \
        FORCE_INLINE static void DestroyComponent(Vault& vault, Entity entity) { \
            vault.RemoveAndSwapComponent<T>(entity); \
        } \
        NO_DISCARD FORCE_INLINE static void* GetComponent(Vault& vault, Entity entity) { \
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
                    metadata.createComponent = &CreateComponent; \
                    metadata.destroyComponent = &DestroyComponent; \
                    metadata.onEditorUI = &OnEditorUI; \
                } else { \
                    metadata.createComponent = nullptr; \
                    metadata.destroyComponent = nullptr; \
                    metadata.onEditorUI  = nullptr; \
                } \
                metadata.getComponent = &GetComponent; \
                metadata.getComponentFlags = &Component::GetComponentFlags; \
            } \
        } \
    }; \
    static const Component##MetadataInitializer Component##MetadataInitializer;
