#pragma once

#include "Core/NotCopyable.hpp"
#include "Core/ThreadContext.hpp"
#include "Vault.hpp"

#include "Editor/TimeScope.hpp"

// To support hot-reloading, system state and logic must be managed by the engine,
// not the transient game DLL. If a system contains virtual methods, its vtable
// pointers become invalid upon unloading the DLL and must be re-created.
//
// Normally, this requires separating state into a separate struct (e.g., `state.field`),
// which clutters the code with redundant prefixes. To keep state directly as fields
// in the system class while retaining polymorphism, we do the following:
//
// 1. The Concrete System (e.g. PlayerSystem): Contains the actual fields and logic,
//    but does NOT inherit from the 'System' base class thus it has no vtable.
// 2. The Wrapper System: A macro-generated class that inherits from 'System' and
//    holds a reference to the concrete system, forwarding all virtual calls.
//
// During a hot-reload, the engine deletes and re-creates only the Wrapper. The
// concrete system's state persists completely untouched in engine memory, provided
// no fields were added or removed during the recompile.

// Traits to help in not having to declare all system callbacks unless necessary

template<typename SystemType>
concept HasStart = requires(SystemType system, ThreadContext& threadContext, Vault& vault)
{
    system.Start(threadContext, vault);
};
template<typename SystemType>
concept HasPrePhysicsUpdate = requires(SystemType system, ThreadContext& threadContext, Vault& vault)
{
    system.PrePhysicsUpdate(threadContext, vault);
};
template<typename SystemType>
concept HasPostPhysicsUpdate = requires(SystemType system, ThreadContext& threadContext, Vault& vault)
{
    system.PostPhysicsUpdate(threadContext, vault);
};
template<typename SystemType>
concept HasUpdate = requires(SystemType system, ThreadContext& threadContext, Vault& vault)
{
    system.Update(threadContext, vault);
};
template<typename SystemType>
concept HasLateUpdate = requires(SystemType system, ThreadContext& threadContext, Vault& vault)
{
    system.LateUpdate(threadContext, vault);
};
template<typename SystemType>
concept HasEnd = requires(SystemType system, ThreadContext& threadContext, Vault& vault)
{
    system.End(threadContext, vault);
};

template<typename SystemType>
void CallStartIfExists(SystemType& system, ThreadContext& threadContext, Vault& vault) {
    if constexpr (HasStart<SystemType>) {
        TIME_SCOPE("Start_" + NameString(TypeName<SystemType>()));
        system.Start(threadContext, vault);
    }
}
template<typename SystemType>
void CallPrePhysicsIfExists(SystemType& system, ThreadContext& threadContext, Vault& vault) {
    if constexpr (HasPrePhysicsUpdate<SystemType>) {
        TIME_SCOPE("PrePhysicsUpdate_" + NameString(TypeName<SystemType>()));
        system.PrePhysicsUpdate(threadContext, vault);
    }
}
template<typename SystemType>
void CallPostPhysicsIfExists(SystemType& system, ThreadContext& threadContext, Vault& vault) {
    if constexpr (HasPostPhysicsUpdate<SystemType>) {
        TIME_SCOPE("PostPhysicsUpdate_" + NameString(TypeName<SystemType>()));
        system.PostPhysicsUpdate(threadContext, vault);
    }
}
template<typename SystemType>
void CallUpdateIfExists(SystemType& system, ThreadContext& threadContext, Vault& vault) {
    if constexpr (HasUpdate<SystemType>) {
        TIME_SCOPE("Update_" + NameString(TypeName<SystemType>()));
        system.Update(threadContext, vault);
    }
}
template<typename SystemType>
void CallLateUpdateIfExists(SystemType& system, ThreadContext& threadContext, Vault& vault) {
    if constexpr (HasLateUpdate<SystemType>) {
        TIME_SCOPE("LateUpdate_" + NameString(TypeName<SystemType>()));
        system.LateUpdate(threadContext, vault);
    }
}
template<typename SystemType>
void CallEndIfExists(SystemType& system, ThreadContext& threadContext, Vault& vault) {
    if constexpr (HasEnd<SystemType>) {
        TIME_SCOPE("End_" + NameString(TypeName<SystemType>()));
        system.End(threadContext, vault);
    }
}

#define GENERATE_SYSTEM_WRAPPER(SystemName) \
public: \
    class WrapperSystem final : public System { \
    private: \
        SystemName& realSystem; \
    public: \
        explicit WrapperSystem(SystemName& instance) : realSystem(instance) {} \
        void Start(ThreadContext& threadContext, Vault& vault) override { CallStartIfExists(realSystem, threadContext, vault); } \
        void PrePhysicsUpdate(ThreadContext& threadContext, Vault& vault) override { CallPrePhysicsIfExists(realSystem, threadContext, vault); } \
        void PostPhysicsUpdate(ThreadContext& threadContext, Vault& vault) override { CallPostPhysicsIfExists(realSystem, threadContext, vault); } \
        void Update(ThreadContext& threadContext, Vault& vault) override { CallUpdateIfExists(realSystem, threadContext, vault); } \
        void LateUpdate(ThreadContext& threadContext, Vault& vault) override { CallLateUpdateIfExists(realSystem, threadContext, vault); } \
        void End(ThreadContext& threadContext, Vault& vault) override { CallEndIfExists(realSystem, threadContext, vault); } \
    }; \
    private: // Defaulting back to private since this is mostly used with classes

#define GENERATE_SYSTEM_BODY(SystemName) \
    GENERATE_SYSTEM_WRAPPER(SystemName)

using SystemID = uint32;

// TODO Update callbacks to create a clear separation between engine and game callbacks
// E.g. Start -> BeginPlay , End -> EndPlay, Update -> UpdateGame (maybe StartGame, EndGame, UpdateGame)
// Add StartEngine, EndEngine, UpdateEngine
class ENGINE_API System : public NotCopyable {

public:
    virtual ~System() = default;

    /* Called at engine start */
    virtual void Start(ThreadContext& threadContext, Vault& vault) {}

    /* Called before a physics update */
    virtual void PrePhysicsUpdate(ThreadContext& threadContext, Vault& vault) {}

    /* Called after a physics update */
    virtual void PostPhysicsUpdate(ThreadContext& threadContext, Vault& vault) {}

    /* Called every frame */
    virtual void Update(ThreadContext& threadContext, Vault& vault) {}

    /* Called at the end of the frame right before rendering */
    virtual void LateUpdate(ThreadContext& threadContext, Vault& vault) {}

    /* Called when engine is closed */
    virtual void End(ThreadContext& threadContext, Vault& vault) {}
};