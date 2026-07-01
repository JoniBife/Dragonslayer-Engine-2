#pragma once

#include <concepts>

#include "Core/Containers/Array.hpp" // TODO Maybe forward declare here?
#include "Core/IO/BufferedFile.hpp"
#include "Core/Types.hpp"
#include "Entity.hpp"

constexpr uint32 MAX_COMPONENT_TYPES = 1024; // TODO Should be defined by the engine

using ComponentID = uint32;

struct ComponentFlags {
    size_t numComponents = 0; // TODO Consider switching to uint32, size_t is super large!
    bool isSerializable : 1 = false;
    bool hasMetadata : 1 = false;
    bool isDisplayable : 1 = false;
};

template<typename Component>
concept HasFlags = requires() {
    { Component::GetComponentFlags() } -> std::same_as<ComponentFlags>;
};

template<typename Component>
concept IsSerializable = HasFlags<Component> && (Component::GetComponentFlags().isSerializable == true);

template<typename Component>
concept HasMetadata = HasFlags<Component> && (Component::GetComponentFlags().hasMetadata == true);

template<typename Component>
concept IsDisplayable = HasFlags<Component> && (Component::GetComponentFlags().isDisplayable == true);

#define GENERATE_GLOBAL_FLAGS(Component, ...) \
    static constexpr ComponentFlags Component##Flags = { __VA_ARGS__ }; \

#define GENERATE_LOCAL_FLAGS(Component) \
public: \
    static constexpr ComponentFlags GetComponentFlags() { return Component##Flags; }

#define VALIDATE_FLAGS(Component, Kind) \
    static_assert(HasFlags<Component>, \
    #Component" is missing: GENERATE_D"#Kind"_BODY("#Component")");

template<typename Component>
concept IsTriviallySerializable = std::is_trivially_copyable_v<Component> && std::is_default_constructible_v<Component>;

using VaultFile = BufferedFile<FreeListAllocator>;

template<typename Component>
concept HasCustomSerialization = requires(Component& component, void* obj, VaultFile& file) {
    { component.Serialize(file) } -> std::same_as<void>;
    { Component::Deserialize(obj, file) } -> std::same_as<void>;
};

#define VALIDATE_SERIALIZATION(Component) \
    static_assert(!IsSerializable<Component> || \
        (IsTriviallySerializable<Component> || HasCustomSerialization<Component>), \
        #Component" is not trivially copyable nor default constructible thus needs to implement: void Serialize(VaultFile&)" \
        "and static void Deserialize(void*, VaultFile&)");

// Helper type for component and entities
template<typename Component>
struct ComponentEntityPair {
    Component component;
    Entity entity;
};

// Templated call to default destructor
template<typename Component>
void PairDefaultConstructor(void* obj) {
    using PairType = ComponentEntityPair<Component>;
    PairType* pair = static_cast<PairType*>(obj);
    new (&pair->component) Component();
}

// Templated call to move constructor
template<typename Component>
void PairMoveConstructor(void* dst, void* src) {
    using PairType = ComponentEntityPair<Component>;
    if constexpr (std::is_trivially_copyable_v<Component>) {
        memcpy(dst, src, sizeof(PairType));
    } else {
        PairType* dstPair = static_cast<PairType*>(dst);
        PairType* srcPair = static_cast<PairType*>(src);
        new (&dstPair->component) Component(std::move(srcPair->component));
        dstPair->entity = srcPair->entity;
    }
}

// Templated call to destructor
template<typename Component>
void PairDestructor(void* obj) {
    using PairType = ComponentEntityPair<Component>;
    PairType* pair = static_cast<PairType*>(obj);
    if constexpr (!std::is_trivially_destructible_v<Component>) {
        pair->component.~Component();
    }
    pair->entity = ECS::InvalidEntity;
}

// Templated call to serialize
template<typename Component>
void PairSerialize(void* obj, VaultFile& file) {
    using PairType = ComponentEntityPair<Component>;
    PairType* pair = static_cast<PairType*>(obj);
    file.Write(pair->entity);
    pair->component.Serialize(file);
}

// Templated call to deserialize
template<typename Component>
void PairDeserialize(void* obj, VaultFile& file) {
    using PairType = ComponentEntityPair<Component>;
    PairType* pair = static_cast<PairType*>(obj);
    file.Read(pair->entity);
    return Component::Deserialize(&pair->component, file);
}

struct ComponentFunctions {
    void (*defaultConstructor)(void* obj);
    void (*moveConstructor)(void* dst, void* src);
    void (*destructor)(void* obj);
    void (*serialize)(void* obj, VaultFile& file);
    void (*deserialize)(void* obj, VaultFile& file);

    template<typename Component>
    static ComponentFunctions Create() {

        ComponentFunctions functions = {
            .defaultConstructor = nullptr,
            .moveConstructor = &PairMoveConstructor<Component>,
            .destructor = &PairDestructor<Component>,
            .serialize = nullptr,
            .deserialize = nullptr
        };
        
        if constexpr (std::is_default_constructible_v<Component>)
        {
            functions.defaultConstructor = &PairDefaultConstructor<Component>;
        }

        if constexpr (HasCustomSerialization<Component>)
        {
            functions.serialize = &PairSerialize<Component>;
            functions.deserialize = &PairDeserialize<Component>;
        }

        return functions;
    }
};

using OnCreateFunc = void(*)(Entity, void*, class Vault&);
using OnDestroyFunc = void(*)(Entity, void*, Vault&);

// Represents the necessary data to initialize a component pool
struct ComponentConstructionData {
    ComponentID componentID;
    ComponentFunctions componentFunctions;
    size_t componentSize;
    size_t componentEntityPairSize;
    size_t componentEntityPairAlignment;
    size_t entityOffset;
    size_t numComponents; // Zero means use max possible which is max entities
    bool isSerializable;
    // TODO Consider adding storage type (.e.g FixedPool, Singleton, perhaps dynamic ?)
};

// Declared as a function instead of variable because c++ standard does not guarantee
// construction order across translation units, with a static variable inside a function
// we can enforce the initialization when the function gets called the first time
ENGINE_API InlineArray<ComponentConstructionData, MAX_COMPONENT_TYPES>& GetComponentsConstructionData();

// NOTE 1: The custom OffsetOf is not constexpr, but that is ok since CreateFixedComponentPool only happens once!

// NOTE 2: In hot-reloading mode the initialization will be called multiple times,
// once for the Runtime and every time Game.dll gets loaded, this is fine since
// we have a loop removing existing metadata, this ensures no duplicates exist.
// It could be fixed by declaring the metadata initializer variable as extern
// but then we would be forced to have a .cpp to declare them which is not ideal
#define INITIALIZE_COMPONENT_CONSTRUCTION_DATA(Component) \
    struct Component##ConstructionDataInitializer { \
        Component##ConstructionDataInitializer() { \
            using PairType = ComponentEntityPair<Component>; \
            constexpr ComponentID componentID = TypeHash<Component>(); \
            size_t entityOffset; \
            if constexpr (std::is_standard_layout_v<PairType>) { \
                entityOffset = offsetof(PairType,entity); \
            } else { \
                entityOffset = OffsetOf(&PairType::entity); \
            } \
            const ComponentFlags componentFlags = Component::GetComponentFlags(); \
            \
            uint32 idx = 0; \
            auto& componentsConstructionData = GetComponentsConstructionData(); \
            for (const ComponentConstructionData& componentConstructionData : componentsConstructionData) { \
                if (componentConstructionData.componentID == componentID) { \
                    componentsConstructionData.RemoveAndSwapAt(idx); \
                    break; \
                } \
                ++idx; \
            } \
            \
            componentsConstructionData.Emplace( \
                componentID, \
                ComponentFunctions::Create<Component>(), \
                sizeof(Component), \
                sizeof(PairType), \
                alignof(PairType), \
                entityOffset, \
                componentFlags.numComponents, \
                componentFlags.isSerializable \
            ); \
        } \
    }; \
    static const Component##ConstructionDataInitializer Component##ConstructionDataInitializer;

#if WITH_EDITOR
#include <imgui.h>
#include "Editor/ComponentMetadata.hpp"
#include "Editor/ImGuiExtensions.hpp"

#define DSTRUCT(Component, ...) \
    GENERATE_GLOBAL_FLAGS(Component, __VA_ARGS__) \
    struct Component

// Used when some members need to be exported
#define ENGINE_DSTRUCT(Component, ...) \
    GENERATE_GLOBAL_FLAGS(Component, __VA_ARGS__) \
    struct ENGINE_API Component

#define GENERATE_DSTRUCT_BODY(Component) \
    GENERATE_LOCAL_FLAGS(Component) \
    GENERATE_METADATA_BODY(Component) \
public:

#define END_DSTRUCT(Component) \
    VALIDATE_FLAGS(Component, STRUCT) \
    VALIDATE_SERIALIZATION(Component) \
    INITIALIZE_COMPONENT_CONSTRUCTION_DATA(Component) \
    END_METADATA_COMPONENT(Component)


#define DCLASS(Component, ...) \
    GENERATE_GLOBAL_FLAGS(Component, __VA_ARGS__) \
    class Component

#define GENERATE_DCLASS_BODY(Component) \
    GENERATE_LOCAL_FLAGS(Component) \
    GENERATE_METADATA_BODY(Component) \
private:

#define END_DCLASS(Component) \
    VALIDATE_FLAGS(Component, CLASS) \
    VALIDATE_SERIALIZATION(Component) \
    INITIALIZE_COMPONENT_CONSTRUCTION_DATA(Component) \
    END_METADATA_COMPONENT(Component)

#else

#define DSTRUCT(Component, ...) \
    GENERATE_GLOBAL_FLAGS(Component, __VA_ARGS__) \
    struct Component

// Used when some members need to be exported
#define ENGINE_DSTRUCT(Component, ...) \
    GENERATE_GLOBAL_FLAGS(Component, __VA_ARGS__) \
    struct ENGINE_API Component

#define GENERATE_DSTRUCT_BODY(Component) \
    GENERATE_LOCAL_FLAGS(Component) \
public:

#define END_DSTRUCT(Component) \
    VALIDATE_FLAGS(Component, STRUCT) \
    VALIDATE_SERIALIZATION(Component) \
    INITIALIZE_COMPONENT_CONSTRUCTION_DATA(Component)

#define DCLASS(Component, ...) \
    GENERATE_GLOBAL_FLAGS(Component, __VA_ARGS__) \
    class Component

#define GENERATE_DCLASS_BODY(Component) \
    GENERATE_LOCAL_FLAGS(Component) \
private:

#define END_DCLASS(Component) \
    VALIDATE_FLAGS(Component, CLASS) \
    VALIDATE_SERIALIZATION(Component) \
    INITIALIZE_COMPONENT_CONSTRUCTION_DATA(Component)

#endif