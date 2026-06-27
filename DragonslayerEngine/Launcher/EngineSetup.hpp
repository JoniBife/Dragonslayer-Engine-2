#pragma once

#include <Runtime/Engine.hpp>
#include <Editor/TimeScope.hpp>
#include <Core/Thread.hpp>
#include <Core/ThreadContext.hpp>

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
#include "NRIAgilitySDK.h" // This MUST be included here in order for the agility SDK to work
#endif

using EngineLoopFunc = void(*)(Engine& engine, ThreadContext& threadContext);

struct ThreadParameters {
    Engine& engine;
    ThreadContext threadContext;
    EngineLoopFunc engineLoopFunc;
};

void RunEngineLoop(Engine& engine, ThreadContext& threadContext, const EngineLoopFunc engineLoopFunc) {
    engine.Start(threadContext);
    threadContext.Sync();

    engineLoopFunc(engine, threadContext);
    threadContext.Sync(); // Potentially redundant

    engine.End(threadContext);
    threadContext.Sync();
}

void ThreadEntryPoint(Thread& thread) {
    ThreadParameters& threadData = thread.GetUserData<ThreadParameters>();
    SetThreadContext(&threadData.threadContext);
    RunEngineLoop(threadData.engine, threadData.threadContext, threadData.engineLoopFunc);
}

FORCE_INLINE void RunEngine(
    int argc,
    char *argv[],
    const Engine::RegisterGameSystemsFunc registerGameSystemsFunc,
    const EngineLoopFunc engineLoopFunc) {

    nri::GraphicsAPI api = nri::GraphicsAPI::D3D12;

    if (argc > 1) {
        const char* arg = argv[1];
        if (strcmp(arg, "VK") == 0) {
            api = nri::GraphicsAPI::VK;
        }
    }

    const EngineSettings engineSettings = {
        .maxMemory = Gb(3),
        .maxGameMemory = Mb(256),
        .maxMemoryPerThread = Mb(8),
        .maxTempMemoryPerThread = Mb(16),
        .maxThreads = std::thread::hardware_concurrency() - 1, // Minus 1 to give some breathing room for the OS
        .maxGameSystems = 64,
#if WITH_EDITOR
        .maxEditorSystems = 32,
#endif

        .vaultSettings = {
            .maxMemory = Mb(128),
            .maxEntities = 65536,
            .maxComponentTypes = 64,
        },

        .windowSettings = {
            .title = "WackyWorks",
            .maxMemory = Kb(256), // GLFW Seems to only use 89 Kb
            .width = 2560,
            .height = 1600,
        },

        .rendererSettings = {
            .usedMemory = Mb(32),
            .graphicsAPI = api,
            .enableVSync = true,
#if DS_RELEASE_BUILD
            .debugAPI = false,
            .debugNRI = false
#else
            .debugAPI = true,
            .debugNRI = true
#endif
        },

        .physicsSettings = {
            .usedMemory = Mb(256),
            .scratchMemory = Mb(16), // PhysX requires a multiple of 16
            .enablePhysicsVisualDebugger = false
        }
    };

    FreeListAllocator globalAllocator(engineSettings.maxMemory);
    gGlobalAllocator = &globalAllocator;
    gGameAllocator = gGlobalAllocator->Allocate<FreeListAllocator>(*gGlobalAllocator, engineSettings.maxGameMemory);
    // Main-thread temp allocator
    StackAllocator* mainTempAllocator = gGlobalAllocator->Allocate<StackAllocator>(*gGlobalAllocator, engineSettings.maxTempMemoryPerThread);

    gNumThreads = engineSettings.maxThreads;

    Barrier barrier(static_cast<int64>(engineSettings.maxThreads));

#if WITH_EDITOR
    // Shared across all threads; each thread's Sync() stamps its own slot to validate barrier alignment.
    Array<SyncSite, FreeListAllocator> syncSites = Array<SyncSite, FreeListAllocator>(engineSettings.maxThreads, gGlobalAllocator);
    syncSites.EmplaceMany(engineSettings.maxThreads);
#endif

    ThreadContext mainThreadContext = {
        .allocator = *gGlobalAllocator,
        .tempAllocator = *mainTempAllocator,
        .barrier = barrier,
#if WITH_EDITOR
        .syncSites = syncSites,
#endif
        .broadcastMemory = nullptr, // TODO Decide what to pass here as default
        .count = static_cast<uint32>(engineSettings.maxThreads),
        .index = 0
    };
    SetThreadContext(&mainThreadContext);

    Engine engine(engineSettings);

    engine.SetRegisterGameSystemProc(registerGameSystemsFunc);

    Array<Thread*, FreeListAllocator> threads = Array<Thread*, FreeListAllocator>(engineSettings.maxThreads, gGlobalAllocator);

    // Loop starts at 1 to exclude the main thread
    for (uint32 i = 1; i < engineSettings.maxThreads; ++i) {

        FreeListAllocator* threadAllocator = gGlobalAllocator->Allocate<FreeListAllocator>(*gGlobalAllocator, engineSettings.maxMemoryPerThread);
        StackAllocator* threadTempAllocator = gGlobalAllocator->Allocate<StackAllocator>(*gGlobalAllocator, engineSettings.maxTempMemoryPerThread);

        const ThreadContext threadContext = {
            .allocator = *threadAllocator,
            .tempAllocator = *threadTempAllocator,
            .barrier = barrier,
#if WITH_EDITOR
            .syncSites = syncSites,
#endif
            .broadcastMemory = nullptr, // TODO Decide what to pass here as default
            .count =  static_cast<uint32>(engineSettings.maxThreads),
            .index = i
        };

        ThreadParameters* threadData = gGlobalAllocator->Allocate<ThreadParameters>(engine, threadContext, engineLoopFunc);

        threads.Emplace(gGlobalAllocator->Allocate<Thread>(ThreadEntryPoint, threadData));
    }

    // Run engine on main thread as well!
    RunEngineLoop(engine, mainThreadContext, engineLoopFunc);

    // -1 to exclude the main thread
    for (uint32 i = 0; i < engineSettings.maxThreads - 1; ++i) {
        threads[i]->Join();
        gGlobalAllocator->Free(threads[i]);
    }
}

