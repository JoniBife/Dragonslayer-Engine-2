#pragma once

#include "Core/Export.hpp"
#include "Editor/TimeScope.hpp"
#include "Core/Log.hpp"
#include "Core/Allocators/LinearAllocator.hpp"
#include "Core/Window.hpp"
#include "ECS/System.hpp"
#include "ECS/Vault.hpp"
#include "Engine.hpp"
#include "Input.hpp"
#include "Physics/Systems/PhysicsSystem.hpp"
#include "Renderer/RenderingSystem.hpp"

struct ENGINE_API EngineSettings {

    // Maximum memory used by all the engine
    size_t maxMemory;

    // Maximum memory used by the game (subset of maxMemory)
    size_t maxGameMemory;

    // Maximum memory used by each thread's allocator (subset of maxMemory)
    size_t maxMemoryPerThread;

    // Maximum memory used by each thread's temp allocator (subset of maxMemory)
    size_t maxTempMemoryPerThread;

    // Maximum threads created for the engine (does not represent actual total)
    size_t maxThreads;

    // Maximum number of game systems
    size_t maxGameSystems;

#if WITH_EDITOR
    // Maximum number of editor systems
    size_t maxEditorSystems;
#endif

    VaultSettings vaultSettings;

    WindowSettings windowSettings;

    RendererSettings rendererSettings;

    PhysicsSettings physicsSettings;
};

class ENGINE_API Engine final : public NotCopyable {

public:
    using RegisterGameSystemsProc = void(*)(Engine&);

private:
    EngineSettings settings;

    // Stores the current map's entities and components
    Vault* editorVault = nullptr;

    // Stores all the data from the editor vault plus anything spawned at runtime
    // On play the editor vault is copied over to the active. On stop the active is reset.
    Vault* activeVault = nullptr;

    // TODO Consider switching to InlineArray
    Array<System*, FreeListAllocator> engineSystems;
    HashMap<SystemID, void*, FreeListAllocator, true> realEngineSystems;

    Array<System*, FreeListAllocator> gameSystems;
    HashMap<SystemID, void*, FreeListAllocator, true> realGameSystems;

#if WITH_EDITOR
    Array<System*, FreeListAllocator> editorSystems;
    HashMap<SystemID, void*, FreeListAllocator, true> realEditorSystems;
#endif

    Window window;

    Input input;

    Log log;

    RegisterGameSystemsProc registerGameSystemsProc = nullptr;

    bool startedGame = false;
    bool endedGame = false;
    bool isPlayingGame = false;

public:
    explicit Engine(const EngineSettings& settings);
    ~Engine();

    void SetRegisterGameSystemProc(const RegisterGameSystemsProc& newRegisterGameSystemsProc);

    template<typename SystemType, typename... Args>
    void AddGameSystem(Args&&... args) {
        TIME_SCOPE("AddSystem_" + NameString(TypeName<SystemType>()));
        constexpr SystemID systemID = TypeHash<SystemType>();
        SystemType* system = nullptr;
        if (void** existingSystem = realGameSystems.Find(systemID)) {
            system = static_cast<SystemType*>(*existingSystem);
        } else {
            system = gGlobalAllocator->Allocate<SystemType>(std::forward<Args>(args)...);
            realGameSystems.Emplace(systemID, system);
        }
        // Necessary so the compiler recognizes the wrapper system as a valid type
        using WrapperSystemType = SystemType::WrapperSystem;
        WrapperSystemType* wrapperSystem = gGlobalAllocator->Allocate<WrapperSystemType>(*system);
        gameSystems.Add(wrapperSystem);
    }

#if WITH_EDITOR
    template<typename SystemType, typename... Args>
    void AddEditorSystem(Args&&... args) {
        TIME_SCOPE("AddSystem_" + NameString(TypeName<SystemType>()));
        constexpr SystemID systemID = TypeHash<SystemType>();
        SystemType* system = nullptr;
        if (void** existingSystem = realEditorSystems.Find(systemID)) {
            system = static_cast<SystemType*>(*existingSystem);
        } else {
            system = gGlobalAllocator->Allocate<SystemType>(std::forward<Args>(args)...);
            realEditorSystems.Emplace(systemID, system);
        }
        // Necessary so the compiler recognizes the wrapper system as a valid type
        using WrapperSystemType = SystemType::WrapperSystem;
        WrapperSystemType* wrapperSystem = gGlobalAllocator->Allocate<WrapperSystemType>(*system);
        editorSystems.Add(wrapperSystem);
    }
#endif

    template<typename SystemType>
    SystemType& GetGameSystem() {
        constexpr SystemID systemID = TypeHash<SystemType>();
        return *static_cast<SystemType*>(realGameSystems[systemID]);
    }

    template<typename SystemType>
    SystemType& GetEngineSystem() {
        constexpr SystemID systemID = TypeHash<SystemType>();
        return *static_cast<SystemType*>(realEngineSystems[systemID]);
    }

#if WITH_EDITOR
    template<typename SystemType>
    SystemType& GetEditorSystem() {
        constexpr SystemID systemID = TypeHash<SystemType>();
        return *static_cast<SystemType*>(realEditorSystems[systemID]);
    }
#endif

    // TODO These should not be part of the public API
    void Start(struct ThreadContext& threadContext);
    bool Update(ThreadContext& threadContext);
    void End(ThreadContext& threadContext);

    void PlayGame();
    void StopGame();

private:
    template<typename SystemType, typename... Args>
    void AddEngineSystem(Args&&... args) {
        TIME_SCOPE("AddSystem_" + NameString(TypeName<SystemType>()));
        constexpr SystemID systemID = TypeHash<SystemType>();
        SystemType* system = nullptr;
        if (void** existingSystem = realEngineSystems.Find(systemID)) {
            system = static_cast<SystemType*>(*existingSystem);
        } else {
            system = gGlobalAllocator->Allocate<SystemType>(std::forward<Args>(args)...);
            realEngineSystems.Emplace(systemID, system);
        }
        // Necessary so the compiler recognizes the wrapper system as a valid type
        using WrapperSystemType = SystemType::WrapperSystem;
        WrapperSystemType* wrapperSystem = gGlobalAllocator->Allocate<WrapperSystemType>(*system);
        engineSystems.Add(wrapperSystem);
    }

    void StartGame(ThreadContext& threadContext);
    void EndGame(ThreadContext& threadContext);
};
