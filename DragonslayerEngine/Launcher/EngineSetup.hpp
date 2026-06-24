#pragma once

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
#include "NRIAgilitySDK.h" // This MUST be included here in order for the agility SDK to work
#endif

#include <Editor/TimeScope.hpp>

#include "Runtime/Engine.hpp"

NO_DISCARD FORCE_INLINE static EngineSettings GetEngineSettingsFromArgs(int argc, char *argv[]) {

    nri::GraphicsAPI api = nri::GraphicsAPI::D3D12;

    if (argc > 1) {
        const char* arg = argv[1];
        if (strcmp(arg, "VK") == 0) {
            api = nri::GraphicsAPI::VK;
        }
    }

    return {
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
            .debugAPI = true,
            .debugNRI = true
        },

        .physicsSettings = {
            .usedMemory = Mb(256),
            .scratchMemory = Mb(16), // PhysX requires a multiple of 16
            .enablePhysicsVisualDebugger = false
        }
    };
}

