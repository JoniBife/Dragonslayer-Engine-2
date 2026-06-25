#include "EngineSetup.hpp"
#include <Game/GameEntrypoint.hpp>

static void EngineLoop(Engine& engine, ThreadContext& threadContext) {
    while (engine.Update(threadContext)) { }
    threadContext.Sync();
}

int main(int argc, char *argv[]) {
    RunEngine(argc, argv, RegisterSystems, EngineLoop);
}
