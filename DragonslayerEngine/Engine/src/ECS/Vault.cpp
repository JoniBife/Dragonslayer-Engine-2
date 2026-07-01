#include "ECS/Vault.hpp"

#if WITH_EDITOR
#include "ECS/EntityName.hpp"
#include "ECS/HierarchyDirectory.hpp"
#endif

#include "Editor/TimeScope.hpp"
#include "Core/Log.hpp"
#include "Core/Containers/ArrayHelper.hpp"

InlineArray<ComponentConstructionData, MAX_COMPONENT_TYPES>& GetComponentsConstructionData() {
    static InlineArray<ComponentConstructionData, MAX_COMPONENT_TYPES> componentConstructionData = {};
    return componentConstructionData;
}

FixedComponentPool::FixedComponentPool(
    LinearAllocator* allocator,
    const ComponentFunctions& componentFunctions,
    size_t maxEntities,
    size_t componentSize,
    size_t componentEntityPairSize,
    size_t componentEntityPairAlignment,
    size_t entityOffset,
    size_t totalNumberComponents,
    bool isSerializable) :
    allocator(allocator),
    componentFunctions(componentFunctions),
    entitiesToComponentIdx(reinterpret_cast<uint32*>(allocator->Allocate(maxEntities * sizeof(uint32), alignof(uint32)))),
    componentEntityPairs(allocator->Allocate(componentEntityPairSize * totalNumberComponents, componentEntityPairAlignment)),
    componentSize(componentSize),
    componentEntityPairSize(componentEntityPairSize),
    componentEntityPairAlignment(componentEntityPairAlignment),
    entityOffset(entityOffset),
    maxComponents(totalNumberComponents),
    activeComponents(0),
    maxEntities(maxEntities),
    isSerializable(isSerializable)
{
    for (uint32 i = 0; i < maxEntities; i++) {
        entitiesToComponentIdx[i] = 0;
    }

    for (uint32 i = 0; i < maxComponents; i++) {
        GetEntityFromPair(GetPairAt(i)) = ECS::InvalidEntity;
    }
}

uint8* FixedComponentPool::CreateComponent(Entity entity) {
    ASSERT(activeComponents < maxComponents, "Fixed Component pool is full!");
    ++activeComponents;
    uint8* componentEntityPair = GetPairAt(activeComponents - 1);
    GetEntityFromPair(componentEntityPair) = entity;
    entitiesToComponentIdx[entity - 1] = activeComponents;
    return componentEntityPair;
}

uint8* FixedComponentPool::GetOrCreateComponent(Entity entity, bool& created) {
    size_t dummyIndex;
    if (uint8* componentEntityPair = TryGetComponent(entity, dummyIndex)) {
        created = false;
        return componentEntityPair;
    }
    created = true;
    return CreateComponent(entity);
}

uint8* FixedComponentPool::TryGetComponent(Entity entity, size_t& index) {
    const uint32 componentIndex = entitiesToComponentIdx[entity - 1];
    if (componentIndex == 0) {
        return nullptr;
    }
    index = componentIndex - 1;
    return GetPairAt(componentIndex - 1);
}

bool FixedComponentPool::RemoveAndSwapComponent(Entity entity) {
    const uint32 componentIndex = entitiesToComponentIdx[entity - 1];
    if (componentIndex == 0) {
        return false;
    }
    return RemoveAndSwapComponentAt(componentIndex - 1);
}

bool FixedComponentPool::RemoveAndSwapComponentAt(size_t index) {

    ASSERT(index < activeComponents, "Index out of bounds!");

    uint8* pairAtIndex = GetPairAt(index);
    const Entity entityAtIndex = GetEntityFromPair(pairAtIndex);
    entitiesToComponentIdx[entityAtIndex - 1] = 0;

    componentFunctions.destructor(pairAtIndex);

    --activeComponents;

    if (index < activeComponents) {
        uint8* lastPair = GetPairAt(activeComponents);
        const Entity lastPairEntity = GetEntityFromPair(lastPair);
        entitiesToComponentIdx[lastPairEntity - 1] = index + 1;
        componentFunctions.moveConstructor(pairAtIndex, lastPair);
        componentFunctions.destructor(lastPair);
    }

    return true;
}

bool FixedComponentPool::SerializeComponent(Entity entity, VaultFile& file) {
    if (!isSerializable) {
        return true;
    }

    size_t dummyIndex;
    uint8* component = TryGetComponent(entity, dummyIndex);
    if (!component) {
        return false;
    }

    if (componentFunctions.serialize) {
        componentFunctions.serialize(component, file);
    } else {
        file.WriteBytes(component, componentSize);
    }
    return true;
}

bool FixedComponentPool::DeserializeComponent(Entity entity, VaultFile& file) {

    ASSERT(isSerializable);

    uint8* component = CreateComponent(entity);

    if (componentFunctions.deserialize) {
        componentFunctions.deserialize(component, file);
    } else {
        componentFunctions.defaultConstructor(component);
        uint32 bytesRead;
        file.ReadBytes(component, componentSize, bytesRead);
    }
    return true;
}

bool FixedComponentPool::Serialize(VaultFile& file) {

    ASSERT(isSerializable);

    if (activeComponents > 0) {
        file.Write(static_cast<uint32>(activeComponents));
        if (componentFunctions.serialize) {
            for (size_t i = 0; i < activeComponents; ++i) {
                componentFunctions.serialize(GetPairAt(i), file);
            }
        } else {
            // Bulk serialize trivially copyable components
            file.WriteBytes(componentEntityPairs, componentEntityPairSize*activeComponents);
        }
        return true;
    }
    return false;
}

bool FixedComponentPool::Deserialize(VaultFile& file) {

    if (!isSerializable) {
        return true;
    }

    if (!file.Read(activeComponents)) {
        return false;
    }
    if (componentFunctions.deserialize) {
        for (size_t i = 0; i < activeComponents; ++i) {
            componentFunctions.deserialize(GetPairAt(i), file);
        }
    } else {

        for (size_t i = 0; i < activeComponents; ++i) {
            componentFunctions.defaultConstructor(GetPairAt(i));
        }

        // Bulk deserialize trivially copyable components
        uint32 bytesRead;
        file.ReadBytes(componentEntityPairs, componentEntityPairSize*activeComponents, bytesRead);
    }
    return true;
}

void FixedComponentPool::Reset() {

    for (int32 i = 0; i < maxEntities; i++) {
        entitiesToComponentIdx[i] = 0;
    }

    memset(componentEntityPairs, 0, componentEntityPairSize*maxComponents);

    activeComponents = 0;
}

void FixedComponentPool::CopyTo(FixedComponentPool& destinationPool) const {

    // This assumes both pools are exactly equal!
    // That is, their entitiesToComponentIdx and componentEntityPairs have exactly the same size

    ASSERT(destinationPool.componentEntityPairSize == componentEntityPairSize);
    ASSERT(destinationPool.componentEntityPairAlignment == componentEntityPairAlignment);
    ASSERT(destinationPool.maxComponents == maxComponents);
    ASSERT(destinationPool.maxEntities == maxEntities);

    destinationPool.activeComponents = activeComponents;
    destinationPool.componentFunctions = componentFunctions;
    destinationPool.componentEntityPairSize = componentEntityPairSize;
    destinationPool.componentEntityPairAlignment = componentEntityPairAlignment;
    destinationPool.entityOffset = entityOffset;
    destinationPool.maxComponents = maxComponents;
    destinationPool.maxEntities = maxEntities;
    destinationPool.isSerializable = isSerializable;
    memcpy(destinationPool.entitiesToComponentIdx, entitiesToComponentIdx, maxEntities * sizeof(uint32));
    memcpy(destinationPool.componentEntityPairs, componentEntityPairs, componentEntityPairSize * maxComponents);
}

EntityIterator EntityIterator::end() const {
    return { vault, vault.lastEntity };
}

void EntityIterator::SkipInvalidEntities() {
    while (currentEntity < vault.lastEntity && !vault.IsEntityValid(currentEntity)) {
        ++currentEntity;
    }
}

void Vault::Destroy() {
    allocator.Free();
}

Entity Vault::CreateEntity() {

    Entity newEntity = ECS::InvalidEntity;

    if (!freeEntities.IsEmpty()) {
        newEntity = freeEntities.GetLast();
        freeEntities.RemoveLast();
    } else {

        // TODO Is assert the right move here?
        ASSERT(lastEntity != ECS::InvalidEntity, "Cannot create more Entities!");
        newEntity = lastEntity;
        ++lastEntity;
    }

    entitiesStates[newEntity - 1] = Active;

    return newEntity;
}

Entity Vault::CreateEntity(const NameString& name) {
    const Entity newEntity = CreateEntity();
#if WITH_EDITOR
    EmplaceComponent<EntityName>(newEntity, name);
#endif
    return newEntity;
}

Entity Vault::CreateEntity(const NameString& name, std::initializer_list<NameString> directoryPath) {
    const Entity newEntity = CreateEntity();
#if WITH_EDITOR
    EmplaceComponent<EntityName>(newEntity, name);
    EmplaceComponent<HierarchyDirectory>(newEntity, directoryPath);
#endif
    return newEntity;
}

Entity Vault::CreateEntity(std::initializer_list<NameString> directoryPath) {
    const Entity newEntity = CreateEntity();
#if WITH_EDITOR
    EmplaceComponent<HierarchyDirectory>(newEntity, directoryPath);
#endif
    return newEntity;
}

bool Vault::DestroyEntity(Entity entity) {

    if (!IsEntityValid(entity)) {
        FAIL("Tried destroying invalid entity!");
        return false;
    }

    for (const ComponentID componentId: componentsPerEntity[entity - 1]) {
        FixedComponentPool& pool = componentPoolsMap[componentId];
        size_t index;
        if (uint8* component = pool.TryGetComponent(entity, index)) {
            NotifyOnDestroy(componentId, entity, component);
            pool.RemoveAndSwapComponentAt(index);
        }
    }

    componentsPerEntity[entity - 1].Reset();

    freeEntities.Add(entity);

    entitiesStates[entity - 1] = Free;

    return true;
}

bool Vault::IsEntityValid(Entity entity) const {
    return entity != ECS::InvalidEntity && entity < lastEntity && (entitiesStates[entity - 1] & Active) == Active;
}

uint32 Vault::GetNumberOfComponents(Entity entity) const {

    if (!IsEntityValid(entity)) {
        FAIL("Cannot get number of components from an Invalid Entity!");
        return 0;
    }

    return componentsPerEntity[entity - 1].GetSize();
}

void Vault::Reset() {

    // This function assumes the vault settings have not been changed!
    // As well as that no component pools have been added nor removed

    for (auto& components : componentsPerEntity) {
        components.Reset();
    }

    for (auto& pair : componentPoolsMap) {
        pair.value.Reset();
    }

    for (auto& pair : componentCallbacks) {
        pair.value.onCreateCallbacks.Reset();
        pair.value.onDestroyCallbacks.Reset();
    }

    entitiesStates.Reset();

    entitiesStates.EmplaceMany(settings.maxEntities, Free);

    freeEntities.Reset();

    lastEntity = ECS::FirstEntity;
}

bool Vault::Serialize(VaultFile& file) {

    TIME_SCOPE("Vault_Serialize");

    bool success = true;

    // We start by writing a dummy (once we know the amount of arrays we update the value by jumping back to this file pointer)
    int64 filePointerNumEntities;
    success &= file.GetFilePointer(filePointerNumEntities);
    success &= file.Write(static_cast<uint32>(0));

    uint32 numEntities = 0;
    for (Entity entity : GetAllEntities()) {

        // TODO Only serialize if entity is serializable
        //EntityState& entityState = entitiesStates[entity - 1];

        success &= file.Write(entity);

        uint32 numComponents = 0;
        for (ComponentID& componentID : componentsPerEntity[entity - 1]) {
            FixedComponentPool& componentPool = componentPoolsMap[componentID];
            if (!componentPool.IsSerializable()) {
                continue;
            }
            ++numComponents;
        }

        success &= file.Write(numComponents);

        for (ComponentID& componentID : componentsPerEntity[entity - 1]) {
            FixedComponentPool& componentPool = componentPoolsMap[componentID];
            if (!componentPool.IsSerializable()) {
                continue;
            }
            success &= file.Write(componentID);
            success &= componentPool.SerializeComponent(entity, file);
        }

        if (!success) {
            break;
        }

        ++numEntities;
    }

    // Jump back to NumEntities, write the actual count, then restore EOF
    int64 filePointerEnd;
    success &= file.GetFilePointer(filePointerEnd);

    success &= file.SetFilePointerTo(filePointerNumEntities);
    success &= file.Write(numEntities);

    success &= file.SetFilePointerTo(filePointerEnd);

    if (success) {
        file.FlushWrites();
    } else {
        file.DiscardWrites();
    }

    return success;
}

bool Vault::Deserialize(VaultFile& file) {

    TIME_SCOPE("Vault_Deserialize");

    // Reset vault
    Reset();

    uint32 numEntities = 0;
    bool success = true;
    success &= file.Read(numEntities);

    if (!success) {
        return false;
    }

    // nothing to deserialize, just return!
    if (numEntities == 0) {
        return true;
    }

    TempArray<Entity> deserializedEntities(numEntities);
    for (int32 i = 0; i < numEntities; ++i) {
        Entity entity = ECS::InvalidEntity;
        success &= file.Read(entity);

        if (entity == ECS::InvalidEntity || entity > settings.maxEntities) {
            Log::Error("[ECS] : Found invalid entity during Vault deserialization!");
            success = false;
            break;
        }

        deserializedEntities.Add(entity);
        entitiesStates[entity - 1] = Active;

        uint32 numComponents = 0;
        success &= file.Read(numComponents);

        for (uint32 j = 0; j < numComponents; ++j) {
            ComponentID componentID = 0;
            success &= file.Read(componentID);

            FixedComponentPool* componentPool = componentPoolsMap.Find(componentID);

            if (!componentPool) {
                Log::Error("[ECS] : Failed to find component pool during Vault deserialization!");
                success = false;
                break;
            }

            if (componentPool->IsSerializable()) {
                success &= componentPool->DeserializeComponent(entity, file);
                componentsPerEntity[entity - 1].Add(componentID);
            }
        }

        if (!success) {
            break;
        }
    }

    deserializedEntities.Sort();

    // Adds free entities that exist in between the deserialized entities
    uint32 previousEntity = ECS::InvalidEntity;
    for (Entity entity : deserializedEntities) {
        for (uint32 i = 1; i < entity - previousEntity; ++i) {
            freeEntities.Emplace(previousEntity + i);
        }
        previousEntity = entity;
    }

    lastEntity = deserializedEntities.GetLast() + 1;

    if (!success) {
        Reset();
    }

    return success;
}

void Vault::CreateComponentPool(const ComponentConstructionData& componentConstructionData) {

    ASSERT(componentPoolsMap.GetSize() + 1 <= componentPoolsMap.GetCapacity(), "Cannot create more component pools!");

    componentPoolsMap.Emplace(
        componentConstructionData.componentID,
        &allocator,
        componentConstructionData.componentFunctions,
        settings.maxEntities,
        componentConstructionData.componentSize,
        componentConstructionData.componentEntityPairSize,
        componentConstructionData.componentEntityPairAlignment,
        componentConstructionData.entityOffset,
        componentConstructionData.numComponents > 0 ? componentConstructionData.numComponents : settings.maxEntities,
        componentConstructionData.isSerializable);

    componentCallbacks.Emplace(
        componentConstructionData.componentID,
        allocator);
}

void Vault::DestroyComponentPool(ComponentID componentID) {

    FixedComponentPool& componentPool = GetComponentPool(componentID);

    // Destroy components
    DestroyComponents(componentID);

    // Finally free underlying memory
    allocator.Free(componentPool.GetComponentEntityPairs());

    componentPoolsMap.Remove(componentID);

    componentCallbacks.Remove(componentID);
}

void Vault::CreateComponentPools() {
    // This is creating pools for components that might not even be in use
    // Generally this is okay unless we have a ridiculous amount of components
    // which is not the case.
    // TODO Consider separating pools per map
    for (ComponentConstructionData& componentConstructionData: GetComponentsConstructionData()) {
        CreateComponentPool(componentConstructionData);
    }
}

void Vault::DestroyComponents(ComponentID componentID) {

    FixedComponentPool& componentPool = GetComponentPool(componentID);

    // Remove components from entity lists
    for (uint32 i = 0; i < componentPool.GetNumComponentEntityPairs(); ++i) {
        uint8* pair = componentPool.GetPairAt(i);
        const Entity entity = componentPool.GetEntityFromPair(pair);
        NotifyOnDestroy(componentID, entity, pair);
        componentsPerEntity[entity -1].RemoveAndSwap(componentID);
    }

    // Destroy components
    componentPool.Reset();
}

bool Vault::DestroyEntitiesWithComponent(ComponentID componentID) {

    FixedComponentPool& componentPool = GetComponentPool(componentID);

    // TODO Optimize, we could rely on component pool clear instead to clear the target component pool

    const uint32 numEntities = componentPool.GetNumComponentEntityPairs();

    if (numEntities == 0) {
        return false;
    }

    for (int32 i = static_cast<int32>(numEntities) - 1; i >= 0; --i) {
        DestroyEntity(componentPool.GetEntityAt(i));
    }

    return true;
}

uint8* Vault::CreateComponent(Entity entity, ComponentID componentID) {

    if (!IsEntityValid(entity)) {
        FAIL("Cannot add component to an invalid Entity!");
        return nullptr;
    }

    if (!componentsPerEntity[entity - 1].IsEmpty()) {
        for (ComponentID existingComponentID: componentsPerEntity[entity - 1]) {
            if (existingComponentID == componentID) {
                FAIL("Entity already has this component!");
                return nullptr;
            }
        }
    }

    FixedComponentPool& componentPool = GetComponentPool(componentID);

    if (uint8* component = componentPool.CreateComponent(entity)) {
        componentsPerEntity[entity - 1].Add(componentID);
        return component;
    }

    return nullptr;
}

bool Vault::RemoveAndSwapComponent(Entity entity, ComponentID componentID) {

    // TODO Should this validation be in?
    if (!IsEntityValid(entity)) {
        FAIL("Cannot remove component from an invalid Entity!");
        return false;
    }

    bool hasComponent = false;
    if (componentsPerEntity[entity - 1].RemoveAndSwap(componentID))
    {
        hasComponent = true;
    }

    if (!hasComponent) {
        FAIL("Cannot remove a component the Entity does not have!");
        return false;
    }

    FixedComponentPool& componentPool = GetComponentPool(componentID);

    size_t index;
    if (uint8* component = componentPool.TryGetComponent(entity, index)) {
        NotifyOnDestroy(componentID, entity, component);
        return componentPool.RemoveAndSwapComponentAt(index);
    }

    return false;
}

bool Vault::RemoveAndSwapComponentAt(Entity entity, size_t index, ComponentID componentID) {

    // TODO Should this validation be in?
    if (!IsEntityValid(entity)) {
        FAIL("Cannot remove component from an invalid Entity!");
        return false;
    }

    bool hasComponent = false;
    if (componentsPerEntity[entity - 1].RemoveAndSwap(componentID))
    {
        hasComponent = true;
    }

    if (!hasComponent) {
        FAIL("Cannot remove a component the Entity does not have!");
        return false;
    }

    FixedComponentPool& componentPool = GetComponentPool(componentID);

    NotifyOnDestroy(componentID, entity, componentPool.GetPairAt(index));

    return componentPool.RemoveAndSwapComponentAt(index);
}

bool Vault::HasComponent(Entity entity, ComponentID componentID) const {

    // TODO Should this validation be in?
    if (!IsEntityValid(entity)) {
        FAIL("Cannot check for component on an invalid Entity!");
        return false;
    }

    for (const ComponentID existingComponentID : componentsPerEntity[entity - 1]) {
        if (existingComponentID == componentID) {
            return true;
        }
    }

    return false;
}

uint8* Vault::TryGetComponent(Entity entity, size_t& index, ComponentID componentID) {

    // TODO Should this validation be in?
    if (!IsEntityValid(entity)) {
        //FAIL("Cannot get component from an invalid Entity!");
        return nullptr;
    }

    // It's generally faster to go over the components per entity list
    // so we do that instead of just rely on TryGetComponent from pool
    bool hasComponent = false;
    for (const ComponentID existingComponentID: componentsPerEntity[entity - 1]) {
        if (existingComponentID == componentID) {
            hasComponent = true;
            break;
        }
    }

    if (!hasComponent) { return nullptr; }

    FixedComponentPool& componentPool = GetComponentPool(componentID);

    return componentPool.TryGetComponent(entity, index);
}

uint8* Vault::TryGetOrCreateComponent(Entity entity, ComponentID componentID, bool& created) {

    // TODO Should this validation be in?
    if (!IsEntityValid(entity)) {
        FAIL("Cannot get component from an invalid Entity!");
        return nullptr;
    }

    FixedComponentPool& componentPool = GetComponentPool(componentID);

    uint8* component = componentPool.GetOrCreateComponent(entity, created);
    if (created) {
        componentsPerEntity[entity - 1].Add(componentID);
    }
    return component;
}

FixedComponentPool& Vault::GetComponentPool(ComponentID componentID) {

    FixedComponentPool* componentPool = componentPoolsMap.Find(componentID);

    ASSERT(componentPool != nullptr, "Component does not have pool!");

    return *componentPool;
}

void Vault::RegisterOnCreate(ComponentID componentID, OnCreateFunc onCreate) {

    ComponentCallbacks* callbacks = componentCallbacks.Find(componentID);

    ASSERT(callbacks != nullptr, "Component does not have pool!");

    callbacks->onCreateCallbacks.Add(onCreate);
}

void Vault::RegisterOnDestroy(ComponentID componentID, OnDestroyFunc onDestroy) {

    ComponentCallbacks* callbacks = componentCallbacks.Find(componentID);

    ASSERT(callbacks != nullptr, "Component does not have pool!");

    callbacks->onDestroyCallbacks.Add(onDestroy);
}

void Vault::CopyTo(Vault& destinationVault) const {

    // Manual copy otherwise Array's copy constructor will be invoked
    // which will copy allocator as well!
    uint32 index = 0;
    for (const Array<ComponentID, LinearAllocator>& components: componentsPerEntity) {
        destinationVault.componentsPerEntity[index] = components;
        ++index;
    }

    for (auto& keyValuePair : componentPoolsMap) {
        const FixedComponentPool& sourceComponentPool = keyValuePair.value;
        FixedComponentPool& destinationPool = destinationVault.GetComponentPool(keyValuePair.key);
        sourceComponentPool.CopyTo(destinationPool);
    }

    destinationVault.componentCallbacks = componentCallbacks;
    destinationVault.entitiesStates = entitiesStates;
    destinationVault.freeEntities = freeEntities;
    destinationVault.lastEntity = lastEntity;
}





