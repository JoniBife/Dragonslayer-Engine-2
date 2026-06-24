#pragma once

#include "Component.hpp"
#include "Core/Allocators/LinearAllocator.hpp"
#include "Core/Containers/Array.hpp"
#include "Core/Containers/ContainerView.hpp"
#include "Core/Containers/HashMap.hpp"
#include "Core/Export.hpp"
#include "Core/IO/File.hpp"
#include "Core/Macros.hpp"
#include "Core/NotCopyable.hpp"
#include "Core/String.hpp"
#include "Core/TypeInfo.hpp"
#include "Entity.hpp"

struct ComponentCallbacks {
    Array<OnCreateFunc, LinearAllocator> onCreateCallbacks;
    Array<OnDestroyFunc, LinearAllocator> onDestroyCallbacks;

    explicit ComponentCallbacks(LinearAllocator& allocator) :
        onCreateCallbacks(8, &allocator),
        onDestroyCallbacks(8, &allocator)  {}
};

/*
 * Fixed size component pool:
 * - Is typeless, does not know the component type only bytes
 * - Only one Component per Entity
 * - Does not grow, memory gets allocated up front
 * - Remove swaps therefore it can break references
 * TODO Consider adding remove that does not swap
 */
class ENGINE_API FixedComponentPool /*: TODO public NotCopyable */ {

private:
    LinearAllocator* allocator;

    ComponentFunctions componentFunctions;
    uint32* entitiesToComponentIdx;
    uint8* componentEntityPairs; // Stored Contiguously

    // TODO Consider switching all these types to uint32
    size_t componentSize;
    size_t componentEntityPairSize;
    size_t componentEntityPairAlignment;
    size_t entityOffset;
    size_t maxComponents;
    size_t activeComponents;
    size_t maxEntities;
    bool isSerializable;

public:
    FixedComponentPool() = default;

    FixedComponentPool(
        LinearAllocator* allocator,
        const ComponentFunctions& componentFunctions,
        size_t maxEntities,
        size_t componentSize,
        size_t componentEntityPairSize,
        size_t componentEntityPairAlignment,
        size_t entityOffset,
        size_t totalNumberComponents,
        bool isSerializable);

    // Does nothing, it's the vault that frees up the memory (componentEntityPairs)
    ~FixedComponentPool() = default;

    NO_DISCARD uint8* CreateComponent(Entity entity);

    NO_DISCARD uint8* GetOrCreateComponent(Entity entity, bool& created);

    NO_DISCARD uint8* TryGetComponent(Entity entity, size_t& index);

    bool RemoveAndSwapComponent(Entity entity);

    bool RemoveAndSwapComponentAt(size_t index);

    bool SerializeComponent(Entity entity, VaultFile& file);

    bool DeserializeComponent(Entity entity, VaultFile& file);

    bool Serialize(VaultFile& file);

    bool Deserialize(VaultFile& file);

    void Reset();

    NO_DISCARD FORCE_INLINE uint8* GetComponentEntityPairs() {
        return componentEntityPairs;
    }

    NO_DISCARD FORCE_INLINE size_t GetNumComponentEntityPairs() const {
        return activeComponents;
    }

    NO_DISCARD FORCE_INLINE uint8* GetPairAt(size_t idx) {
        return componentEntityPairs + idx * componentEntityPairSize;
    }

    NO_DISCARD FORCE_INLINE Entity& GetEntityFromPair(uint8* pair) {
        return *reinterpret_cast<Entity*>(pair + entityOffset);
    }

    NO_DISCARD FORCE_INLINE Entity& GetEntityAt(size_t idx) {
        return GetEntityFromPair(GetPairAt(idx));
    }

    NO_DISCARD FORCE_INLINE bool IsSerializable() const {
        return isSerializable;
    }

    void CopyTo(FixedComponentPool& destinationPool) const;

    friend class Vault;
};

/* Entity Iterator:
 * - Iterates over active entities
 */
class ENGINE_API EntityIterator {

private:
    const class Vault& vault;
    Entity currentEntity;

public:
    explicit EntityIterator(const Vault& vault)
        : vault(vault), currentEntity(ECS::FirstEntity) {
        SkipInvalidEntities();
    }

    NO_DISCARD Entity operator*() const {
        return currentEntity;
    }

    EntityIterator& operator++() {
        ++currentEntity;
        SkipInvalidEntities();
        return *this;
    }

    NO_DISCARD bool operator!=(const EntityIterator& other) const {
        return currentEntity != other.currentEntity;
    }

    NO_DISCARD EntityIterator begin() const { return EntityIterator(vault, ECS::FirstEntity); }
    NO_DISCARD EntityIterator end() const;

private:
    EntityIterator(const Vault& vault, Entity currentEntity)
        : vault(vault), currentEntity(currentEntity) {
        SkipInvalidEntities();
    }

    void SkipInvalidEntities();
};

struct ENGINE_API VaultSettings {

    // Maximum memory used by the Vault (subset of maxMemory)
    size_t maxMemory;

    // Maximum number of entities
    size_t maxEntities;

    // Maximum types of components
    size_t maxComponentTypes;
};

/*
 * A Vault is the storage where ECS related data is stored
 * - Entity IDs get re-used
 * - Each Entity can only have one Component of each type
 * TODO Decide how the validation should be made (Either fallbacks, only asserts or hybrid??)
 * TODO Come to a CONSISTENT decision on the whole get/tryget etc.. topic
 * TODO Future improvement: We can potentially speed up component look-up by switching component ID
 * from a typehash to an incremented id that is automatically defined by component macros (DSTRUCT etc...)
 * Because its incremented it can serve as a simple index to an array instead of relying an on an hashmap!
 */
class ENGINE_API Vault final : public NotCopyable {
private:
    enum EntityState : uint8 {
        Free = 1 << 0,
        Active = 1 << 1,
        Serializable = 1 << 2
    };

    // Once set they never change!
    const VaultSettings settings;

    // Using a linear allocator because all the Vault's containers pre-allocate for the worse case,
    // that is, max amount of entities and components per entity. As such they never grow
    // and therefore never have to reallocate which would lead to free and allocate.
    // This means that all the Vault's memory is stored contiguously with minimal gaps.
    // The only remaining gaps are unavoidable since they come from the memory alignment of the components.
    LinearAllocator allocator;

    // TODO Replace this array by a single contiguous one
    Array<Array<ComponentID, LinearAllocator>, LinearAllocator> componentsPerEntity;
    HashMap<ComponentID, FixedComponentPool, LinearAllocator, true> componentPoolsMap;
    HashMap<ComponentID, ComponentCallbacks, LinearAllocator, true> componentCallbacks;
    Array<EntityState, LinearAllocator> entitiesStates; // State of each entity
    Array<Entity, LinearAllocator> freeEntities; // Entities available to be reused

    Entity lastEntity; // Last entity

public:
    template<typename AllocatorType>
    Vault(const VaultSettings& settings, AllocatorType& parentAllocator) :
        settings(settings),
        allocator(parentAllocator, settings.maxMemory),
        componentsPerEntity(settings.maxEntities, &allocator),
        componentPoolsMap(settings.maxComponentTypes, &allocator),
        componentCallbacks(settings.maxComponentTypes, &allocator),
        entitiesStates(settings.maxEntities, &allocator, Free),
        freeEntities(settings.maxEntities, &allocator),
        lastEntity(ECS::FirstEntity) {

        ASSERT(settings.maxEntities > 0, "Cannot have 0 maximum entities!");
        ASSERT(settings.maxComponentTypes > 0, "Cannot have 0 component types!");

        componentsPerEntity.EmplaceMany(settings.maxEntities, settings.maxComponentTypes, &allocator);

        CreateComponentPools();
    }

    // The vault can be destroyed by simply releasing its memory
    // This bypasses the need to call destructors on all its components
    // thus is a much faster destruction path
    void Destroy();

    ~Vault() = default;

    NO_DISCARD Entity CreateEntity();
    NO_DISCARD Entity CreateEntity(const NameString& name);
    NO_DISCARD Entity CreateEntity(const NameString& name, std::initializer_list<NameString> directoryPath);
    NO_DISCARD Entity CreateEntity(std::initializer_list<NameString> directoryPath);

    bool DestroyEntity(Entity entity);

    NO_DISCARD bool IsEntityValid(Entity entity) const;

    friend class EntityIterator;
    NO_DISCARD FORCE_INLINE EntityIterator GetAllEntities() const {
        return EntityIterator(*this);
    }

    template<typename Component>
    FORCE_INLINE bool DestroyEntitiesWithComponent() {
        constexpr ComponentID componentID = TypeHash<Component>();
        return DestroyEntitiesWithComponent(componentID);
    }

    template<typename Component>
    FORCE_INLINE void DestroyComponentPool() {
        constexpr ComponentID componentID = TypeHash<Component>();
        DestroyComponentPool(componentID);
    }

    template<typename Component>
    NO_DISCARD FORCE_INLINE bool ContainsComponentPool() const {
        constexpr ComponentID componentID = TypeHash<Component>();
        return componentPoolsMap.Contains(componentID);
    }

    template<typename Component, typename... Args>
    FORCE_INLINE Component& EmplaceComponent(Entity entity, Args&&... args) {
        constexpr ComponentID componentID = TypeHash<Component>();
        Component* component = reinterpret_cast<Component*>(CreateComponent(entity, componentID));
        new (component) Component(std::forward<Args>(args)...);
        NotifyOnCreate(componentID, entity, component);
        return *component;
    }

    template<typename Component>
    FORCE_INLINE bool RemoveAndSwapComponent(Entity entity) {
        constexpr ComponentID componentID = TypeHash<Component>();
        return RemoveAndSwapComponent(entity, componentID);
    }

    template<typename Component>
    FORCE_INLINE void DestroyComponents() {
        constexpr ComponentID componentID = TypeHash<Component>();
        DestroyComponents(componentID);
    }

    template<typename Component>
    NO_DISCARD FORCE_INLINE bool HasComponent(Entity entity) const {
        constexpr ComponentID componentID = TypeHash<Component>();
        return HasComponent(entity, componentID);
    }

    template<typename Component>
    NO_DISCARD FORCE_INLINE decltype(auto) GetComponent(Entity entity) {
        constexpr ComponentID componentID = TypeHash<Component>();
        size_t dummyIndex;
        return *reinterpret_cast<Component*>(TryGetComponent(entity, dummyIndex, componentID));
    }

    template<typename Component>
    NO_DISCARD FORCE_INLINE auto TryGetComponent(Entity entity) {
        constexpr ComponentID componentID = TypeHash<Component>();
        size_t dummyIndex;
        return reinterpret_cast<Component*>(TryGetComponent(entity, dummyIndex, componentID));
    }

    template<typename Component, typename... Args>
    NO_DISCARD FORCE_INLINE decltype(auto) GetOrEmplaceComponent(Entity entity, Args&&... args) {
        constexpr ComponentID componentID = TypeHash<Component>();
        bool createdComponent = false;
        Component* component = reinterpret_cast<Component*>(TryGetOrCreateComponent(entity, componentID, createdComponent));
        if (createdComponent) {
            new (component) Component(std::forward<Args>(args)...); // Using placement new to explicitly call the constructor on the element
            NotifyOnCreate(componentID, entity, component);
        }
        return *component;
    }

    template<typename Component>
    FORCE_INLINE auto GetComponents() {
        constexpr ComponentID componentID = TypeHash<Component>();
        FixedComponentPool& componentPool = GetComponentPool(componentID);
        ComponentEntityPair<Component>* componentEntityPairs = reinterpret_cast<ComponentEntityPair<Component>*>(componentPool.GetComponentEntityPairs());
        return ContainerView<ComponentEntityPair<Component>>(componentEntityPairs, componentPool.GetNumComponentEntityPairs());
    }

    template<typename Component>
    NO_DISCARD FORCE_INLINE uint32 GetNumberOfComponents() {
        constexpr ComponentID componentID = TypeHash<Component>();
        const FixedComponentPool& componentPool = GetComponentPool(componentID);
        return componentPool.GetNumComponentEntityPairs();
    }

    NO_DISCARD uint32 GetNumberOfComponents(Entity entity) const;

    template<typename Component, void (*Callback)(Entity, Component&, Vault&)>
    FORCE_INLINE void RegisterOnCreate() {
        constexpr ComponentID componentID = TypeHash<Component>();
        struct OnCreate {
            static void Func(Entity entity, void* component, Vault& vault) {
                Callback(entity, *static_cast<Component*>(component), vault);
            }
        };
        RegisterOnCreate(componentID, &OnCreate::Func);
    }

    template<typename Component, void (*Callback)(Entity, Component&, Vault&)>
    FORCE_INLINE void RegisterOnDestroy() {
        constexpr ComponentID componentID = TypeHash<Component>();
        struct OnDestroy {
            static void Func(Entity entity, void* component, Vault& vault) {
                Callback(entity, *static_cast<Component*>(component), vault);
            }
        };
        RegisterOnDestroy(componentID, &OnDestroy::Func);
    }

    NO_DISCARD FORCE_INLINE uint32 GetMaxEntities() const {
        return settings.maxEntities;
    }

    NO_DISCARD FORCE_INLINE const LinearAllocator& GetAllocator() const {
        return allocator;
    }

    // Resets vault to its initial state (no entities nor components)
    // Component Pools remain in the vault but empty!
    void Reset();

    bool Serialize(VaultFile& file);

    bool Deserialize(VaultFile& file);

private:
    void CreateComponentPool(const ComponentConstructionData& componentConstructionData);

    void DestroyComponentPool(ComponentID componentID);

    void CreateComponentPools();

    void DestroyComponents(ComponentID componentID);

    bool DestroyEntitiesWithComponent(ComponentID componentID);

    NO_DISCARD uint8* CreateComponent(Entity entity, ComponentID componentID);

    bool RemoveAndSwapComponent(Entity entity, ComponentID componentID);

    bool RemoveAndSwapComponentAt(Entity entity, size_t index, ComponentID componentID);

    NO_DISCARD bool HasComponent(Entity entity, ComponentID componentID) const;

    NO_DISCARD uint8* TryGetComponent(Entity entity, size_t& index, ComponentID componentID);

    NO_DISCARD uint8* TryGetOrCreateComponent(Entity entity, ComponentID componentID, bool& created);

    NO_DISCARD FixedComponentPool& GetComponentPool(ComponentID componentID);

    void RegisterOnCreate(ComponentID componentID, OnCreateFunc onCreate);

    void RegisterOnDestroy(ComponentID componentID, OnDestroyFunc onDestroy);

    FORCE_INLINE void NotifyOnCreate(ComponentID componentID, Entity entity, void* component) {
        for (OnCreateFunc& onCreate : componentCallbacks[componentID].onCreateCallbacks) {
            onCreate(entity, component, *this);
        }
    }

    FORCE_INLINE void NotifyOnDestroy(ComponentID componentID, Entity entity, void* component) {
        for (OnDestroyFunc& onDestroy : componentCallbacks[componentID].onDestroyCallbacks) {
            onDestroy(entity, component, *this);
        }
    }

    /*
     * Generally the vault should NEVER be copied!
     * Except when starting to play the game where
     * only the Engine can do it!
     */
    friend class Engine;
    void CopyTo(Vault& destinationVault) const;
};