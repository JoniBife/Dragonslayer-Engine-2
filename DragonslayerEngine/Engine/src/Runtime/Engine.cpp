#include "Runtime/Engine.hpp"

#include "Runtime/Components/Transform.hpp"

#include "Editor/TimeScope.hpp"

#if WITH_EDITOR
#include "Editor/EditorGlobals.hpp"
#include "Editor/Systems/ECSDebugSystem.hpp"
#include "Editor/Systems/EditorCameraSystem.hpp"
#include "Editor/Systems/EditorStateSystem.hpp"
#include "Editor/Systems/EntityHierarchySystem.hpp"
#include "Editor/Systems/MemoryUsageSystem.hpp"
#include "Editor/Systems/OutputLogSystem.hpp"
#include "Editor/Systems/TimeCapturesSystem.hpp"
#include "Editor/Systems/TransformGizmosSystem.hpp"
#endif

#include <Core/ThreadContext.hpp>

#include "Runtime/Systems/LogSystem.hpp"

#include "Runtime/Components/PhysicsNPC.hpp"

#include "Core/EngineGlobals.hpp"
#include "Core/Time.hpp"
#include "Editor/Systems/PhysicsDebugSystem.hpp"
#include "Renderer/RenderingSystem.hpp"
#include "Runtime/Systems/TimeSystem.hpp"

Engine::Engine(const EngineSettings& settings) :
    settings(settings),
    editorVault(gGlobalAllocator->Allocate<Vault>(settings.vaultSettings, *gGlobalAllocator)),
    engineSystems(5, gGlobalAllocator),
    realEngineSystems(5, gGlobalAllocator),
    gameSystems(settings.maxGameSystems, gGlobalAllocator),
    realGameSystems(settings.maxGameSystems, gGlobalAllocator),
#if WITH_EDITOR
    editorSystems(settings.maxEditorSystems + 10, gGlobalAllocator),
    realEditorSystems(settings.maxEditorSystems + 10, gGlobalAllocator),
#endif
    window(settings.windowSettings, *gGlobalAllocator),
    input(window.GetGLFWWindow(), *gGlobalAllocator),
    log(*gGlobalAllocator)
{
    gEngine = this;
    gWindow = &window;

    // TODO Consider making this guy a global, also on release building this should not happen
    TimeCapturesManager::Initialize();

    AddEngineSystem<TimeSystem>();
    AddEngineSystem<PhysicsSystem>(settings.physicsSettings);
    AddEngineSystem<RenderingSystem>(settings.rendererSettings);
    AddEngineSystem<LogSystem>();

#if WITH_EDITOR
    AddEditorSystem<EntityHierarchySystem>();
    AddEditorSystem<ECSDebugSystem>();
    AddEditorSystem<EditorCameraSystem>();
    AddEditorSystem<EditorStateSystem>();
    AddEditorSystem<MemoryUsageSystem>();
    AddEditorSystem<TransformGizmosSystem>();
    AddEditorSystem<TimeCapturesSystem>();
    AddEditorSystem<PhysicsDebugSystem>();
    AddEditorSystem<OutputLogSystem>();
#endif
}

Engine::~Engine() {
    // TODO Maybe?
    // TODO Destroy rest of resources

    // Destroy engine systems that own resources living in separate DLLs while
    // those DLLs are still loaded. RenderingSystem owns the NRI device; its
    // destructor calls nriDestroyDevice(), which deactivates the D3D12 Agility
    // SDK activation context. Leaving this to process teardown unloads NRI.dll
    // with that context still on the loader's activation stack, which the OS
    // reports as STATUS_SXS_CORRUPT_ACTIVATION_STACK (0xC0150014) on exit.
    gGlobalAllocator->Free(&GetEngineSystem<RenderingSystem>());

    gGlobalAllocator->Free(&GetEngineSystem<PhysicsSystem>());
}

void Engine::SetRegisterGameSystemProc(const RegisterGameSystemsFunc& newRegisterGameSystemsProc) {
    this->registerGameSystemsProc = newRegisterGameSystemsProc;
}

void Engine::Start(ThreadContext& threadContext) {

    // Renderer needs to have a separate initialization where multiple threads are available
    GetEngineSystem<RenderingSystem>().Initialize(threadContext);

#if WITH_EDITOR
    Vault& currentVault = *editorVault;
    for (System* system : editorSystems)
    {
        system->Start(threadContext, currentVault);
    }
    threadContext.Sync();
#endif
}

FORCE_INLINE static void DisableInputIfDetached() {
#if WITH_EDITOR
    Input::ToggleInputEnabled(!gDetachedFromGame);
#endif
}

FORCE_INLINE static void EnableInputIfDetached() {
#if WITH_EDITOR
    Input::ToggleInputEnabled(true);
#endif
}

bool Engine::Update(ThreadContext& threadContext) {

    if (startedGame) {
        StartGame(threadContext);
        threadContext.Sync();

        if (threadContext.IsMainThread()) {
            startedGame = false;
        }
    } else if (endedGame) {
        EndGame(threadContext);
        threadContext.Sync();

        if (threadContext.IsMainThread()) {
            endedGame = false;
        }
    }

    if (threadContext.IsMainThread()) {
        input.PollInputEvents();
    }
    threadContext.Sync();

    static uint8 shouldClose = 0;
    if (threadContext.IsMainThread()) {
        shouldClose = 0;
        if (window.ShouldWindowClose()) {
            shouldClose = 1;
        }
        if (!window.IsInFocus() || window.IsMinimized()) {
            shouldClose = 2;
        }
        if (Input::IsKeyPressed(KeyCode::Escape)) {
            shouldClose = 1;
        }
    }
    threadContext.Sync();

    if (shouldClose == 1) {
        return false;
    }

    if (shouldClose == 2) {
        return true;
    }

    if (threadContext.IsMainThread()) {
#if WITH_EDITOR
        GetEngineSystem<RenderingSystem>().BeginImGuiFrame();
#endif
    }
    threadContext.Sync();

    Vault& currentVault = isPlayingGame ? *activeVault : *editorVault;

    GetEngineSystem<TimeSystem>().Update(threadContext, currentVault);
    threadContext.Sync();

    static bool parallelPhysicsWithMultipleFrames = false;
#if WITH_EDITOR
    //ImGui::Begin("Physics Options");
    //ImGui::Checkbox("Physics in Parallel with Multiple Frames", &parallelPhysicsWithMultipleFrames);
    //ImGui::End();
#endif

    // Physics only update when game is running
    if (isPlayingGame && gTime.physicsDeltaTime > 0.f) {
#if WITH_EDITOR
        for (System* system : editorSystems) {
            system->PrePhysicsUpdate(threadContext, currentVault);
        }
        threadContext.Sync();
#endif

        if (threadContext.IsMainThread()) {
            DisableInputIfDetached();
        }
        threadContext.Sync();

        for (System* system : gameSystems) {
            system->PrePhysicsUpdate(threadContext, currentVault);
        }
        threadContext.Sync();

        if (threadContext.IsMainThread()) {
            EnableInputIfDetached();
        }
        threadContext.Sync();

        GetEngineSystem<PhysicsSystem>().Update(threadContext, currentVault);
        threadContext.Sync();
    }

#if WITH_EDITOR
    for (System* system : editorSystems) {
        system->Update(threadContext, currentVault);
    }
    threadContext.Sync();
#endif

    if (isPlayingGame) {
        if (threadContext.IsMainThread()) {
            DisableInputIfDetached();
        }
        threadContext.Sync();

        for (System* system : gameSystems) {
            system->Update(threadContext, currentVault);
        }
        threadContext.Sync();

        if (threadContext.IsMainThread()) {
            EnableInputIfDetached();
        }
        threadContext.Sync();
    }

    GetEngineSystem<RenderingSystem>().Update(threadContext, currentVault);
    threadContext.Sync();

    // Now that the expensive part of the frame is over we block to wait for physics
    // which most likely are already done :)
    if (isPlayingGame && gTime.physicsDeltaTime > 0.f) {

        if (parallelPhysicsWithMultipleFrames) {

            static bool physicsUpdateDone = false;
            if (threadContext.IsMainThread()) {
                physicsUpdateDone = gPhysicsScene->checkResults(false);
            }
            threadContext.Sync();

            if (physicsUpdateDone) {
                GetEngineSystem<PhysicsSystem>().PostPhysicsUpdate(threadContext, currentVault);
                threadContext.Sync();
#if WITH_EDITOR
                for (System* system : editorSystems) {
                    system->PostPhysicsUpdate(threadContext, currentVault);
                }
                threadContext.Sync();
#endif
                if (threadContext.IsMainThread()) {
                    DisableInputIfDetached();
                }
                threadContext.Sync();

                for (System* system : gameSystems) {
                    system->PostPhysicsUpdate(threadContext, currentVault);
                }
                threadContext.Sync();

                if (threadContext.IsMainThread()) {
                    EnableInputIfDetached();
                }
                threadContext.Sync();
            }

        } else {
            GetEngineSystem<PhysicsSystem>().PostPhysicsUpdate(threadContext, currentVault);
            threadContext.Sync();

#if WITH_EDITOR
            for (System* system : editorSystems) {
                system->PostPhysicsUpdate(threadContext, currentVault);
            }
            threadContext.Sync();
#endif

            if (threadContext.IsMainThread()) {
                DisableInputIfDetached();
            }
            threadContext.Sync();

            for (System* system : gameSystems) {
                system->PostPhysicsUpdate(threadContext, currentVault);
            }
            threadContext.Sync();

            if (threadContext.IsMainThread()) {
                EnableInputIfDetached();
            }
            threadContext.Sync();
        }
    }

#if WITH_EDITOR
    for (System* system : editorSystems) {
        system->LateUpdate(threadContext, currentVault);
    }
    threadContext.Sync();
#endif

    if (isPlayingGame) {
        if (threadContext.IsMainThread()) {
            DisableInputIfDetached();
        }
        threadContext.Sync();

        for (System* system : gameSystems) {
            system->LateUpdate(threadContext, currentVault);
        }
        threadContext.Sync();

        if (threadContext.IsMainThread()) {
            EnableInputIfDetached();
        }
        threadContext.Sync();

        GetEngineSystem<PhysicsSystem>().LateUpdate(threadContext, currentVault);
        threadContext.Sync();
    }

    GetEngineSystem<LogSystem>().Update(threadContext, currentVault);
    threadContext.Sync();

    GetThreadTempAllocator().Clear();

    return true;
}

void Engine::End(ThreadContext& threadContext) {

    // Call end in reverse order as there may be dependencies
#if WITH_EDITOR
    Vault& currentVault = isPlayingGame ? *activeVault : *editorVault;
    for (int32 i = static_cast<int32>(editorSystems.GetSize()) - 1; i >= 0; --i)
    {
        editorSystems[i]->End(threadContext, currentVault);
    }
    threadContext.Sync();
#endif

    if (isPlayingGame) {
        EndGame(threadContext);
    }
}

void Engine::PlayGame() {
    ASSERT(!isPlayingGame && !startedGame && !endedGame);
    startedGame = true;
}

void Engine::StopGame() {
    ASSERT(isPlayingGame && !endedGame && !startedGame);
    endedGame = true;
}

void Engine::StartGame(ThreadContext& threadContext) {

    TIME_SCOPE("Engine_StartGame");

    if (threadContext.IsMainThread()) {
        isPlayingGame = true;
        activeVault = gGameAllocator->Allocate<Vault>(settings.vaultSettings, *gGameAllocator);
        editorVault->CopyTo(*activeVault);
    }
    threadContext.Sync();

    Vault& vault = *activeVault;

    if (registerGameSystemsProc) {

        if (threadContext.IsMainThread()) {
            registerGameSystemsProc(*this);
        }
        threadContext.Sync();

        for (System* system : engineSystems)
        {
            system->Start(threadContext, vault);
        }
        threadContext.Sync();

        for (System* system : gameSystems)
        {
            system->Start(threadContext, vault);
        }
        threadContext.Sync();
    }

    if (threadContext.IsMainThread()) {
        // This fixes issues like calling AddForce in bodies not yet in the scene.
        GetEngineSystem<PhysicsSystem>().AddNewBodiesToPhysicsScene(vault);
    }
    threadContext.Sync();
}

void Engine::EndGame(ThreadContext& threadContext) {

    TIME_SCOPE("Engine_EndGame");

    Vault& vault = *activeVault;

    // Call end in reverse order as there may be dependencies
    for (int32 i = static_cast<int32>(gameSystems.GetSize()) - 1; i >= 0; --i)
    {
        gameSystems[i]->End(threadContext, vault);
    }
    threadContext.Sync();

    for (int32 i = static_cast<int32>(engineSystems.GetSize()) - 1; i >= 0; --i)
    {
        engineSystems[i]->End(threadContext, vault);
    }
    threadContext.Sync();

    if (threadContext.IsMainThread()) {
        gameSystems.Reset();
        gGameAllocator->Clear();
        activeVault = nullptr;
        isPlayingGame = false;
    }
    threadContext.Sync();
}
