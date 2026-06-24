#include <barrier>

#include <Core/Thread.hpp>
#include <Core/ThreadContext.hpp>
#include <Game/GameEntrypoint.hpp>
#include <Runtime/Engine.hpp>

#include "EngineSetup.hpp"

struct ThreadParameters {
    Engine& engine;
    ThreadContext threadContext;
};

static void RunEngine(Engine& engine, ThreadContext& threadContext) {
    engine.Start(threadContext);
    threadContext.Sync();

    while (engine.Update(threadContext)) { }
    threadContext.Sync();

    engine.End(threadContext);
    threadContext.Sync();
}

static void EntryPoint(Thread& thread) {
    ThreadParameters& threadData = thread.GetUserData<ThreadParameters>();
    gThreadContext = &threadData.threadContext;
    gThreadTempAllocator = &threadData.threadContext.tempAllocator;
    RunEngine(threadData.engine, threadData.threadContext);
}

int main(int argc, char *argv[]) {

    const EngineSettings engineSettings = GetEngineSettingsFromArgs(argc, argv);

    FreeListAllocator globalAllocator(engineSettings.maxMemory);
    gGlobalAllocator = &globalAllocator;
    gGameAllocator = gGlobalAllocator->Allocate<FreeListAllocator>(*gGlobalAllocator, engineSettings.maxGameMemory);
    // Main-thread temp allocator
    gThreadTempAllocator = gGlobalAllocator->Allocate<StackAllocator>(*gGlobalAllocator, engineSettings.maxTempMemoryPerThread);

    gNumThreads = engineSettings.maxThreads;

    Barrier barrier(static_cast<int64>(engineSettings.maxThreads));

#if WITH_EDITOR
    // Shared across all threads; each thread's Sync() stamps its own slot to validate barrier alignment.
    Array<SyncSite, FreeListAllocator> syncSites = Array<SyncSite, FreeListAllocator>(engineSettings.maxThreads, gGlobalAllocator);
    syncSites.EmplaceMany(engineSettings.maxThreads);
#endif

    ThreadContext mainThreadContext = {
        .allocator = *gGlobalAllocator,
        .tempAllocator = *gThreadTempAllocator,
        .barrier = barrier,
#if WITH_EDITOR
        .syncSites = syncSites,
#endif
        .broadcastMemory = nullptr, // TODO Decide what to pass here as default
        .count = static_cast<uint32>(engineSettings.maxThreads),
        .index = 0
    };
    gThreadContext = &mainThreadContext;

    Engine engine(engineSettings);

    engine.SetRegisterGameSystemProc(&RegisterSystems);

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

        ThreadParameters* threadData = gGlobalAllocator->Allocate<ThreadParameters>(engine, threadContext);

        threads.Emplace(gGlobalAllocator->Allocate<Thread>(EntryPoint, threadData));
    }

    // Run engine on main thread as well!
    RunEngine(engine, mainThreadContext);

    // -1 to exclude the main thread
    for (uint32 i = 0; i < engineSettings.maxThreads - 1; ++i) {
        threads[i]->Join();
        gGlobalAllocator->Free(threads[i]);
    }
}
