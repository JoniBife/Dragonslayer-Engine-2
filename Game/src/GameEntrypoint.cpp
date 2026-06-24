#include "Game/GameExports.hpp"
#include "Game/Systems/EnemyAISystem.hpp"
#include "Game/Systems/GameStateSystem.hpp"
#include "Game/Systems/MapSystem.hpp"
#include "Game/Systems/PlayerCameraSystem.hpp"
#include "Game/Systems/PlayerSystem.hpp"
#include "Game/Systems/WeaponEditorCameraSystem.hpp"
#include "Game/Systems/WeaponEditorSystem.hpp"
#include "Game/Systems/WeaponMotionSystem.hpp"

#include <Runtime/Engine.hpp>

#if WITH_EDITOR
#include "Editor/ImGuiState.hpp"
#endif

#if HOT_RELOAD_ON
// GameEntry.cpp (inside game project)
// TODO Make this a macro
extern "C" {
#endif

    GAME_API void RegisterSystems(Engine& engine) {
        engine.AddGameSystem<GameStateSystem>();
        engine.AddGameSystem<WeaponEditorSystem>();
        engine.AddGameSystem<WeaponEditorCameraSystem>();
        engine.AddGameSystem<WeaponMotionSystem>();
        engine.AddGameSystem<MapSystem>();
        engine.AddGameSystem<PlayerSystem>();
        engine.AddGameSystem<PlayerCameraSystem>();
        engine.AddGameSystem<EnemyAISystem>();
    }

#if WITH_EDITOR
    // Game.dll and Runtime.dll each have a copy of ImGui thus the state (context, allocfuncs etc...) is not shared
    // cross dll boundaries as such it needs to be passed explicitly to the game. We could make ImGui a dll
    // BUT it's not recommended as per their docs.
    GAME_API void SetImGuiState(const ImGui::State& imGuiState)
    {
        ImGui::SetAllocatorFunctions(imGuiState.allocFunc, imGuiState.freeFunc, imGuiState.userData);
        ImGui::SetCurrentContext(imGuiState.context);
    }
#endif

#if HOT_RELOAD_ON
}
#endif

