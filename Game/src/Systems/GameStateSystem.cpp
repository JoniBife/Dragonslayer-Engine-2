#include "Game/Systems/GameStateSystem.hpp"

#include <Runtime/Input.hpp>

#include "Game/Components/GameState.hpp"

void GameStateSystem::Start(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }
    const Entity gameStateEntity = vault.CreateEntity("Game State");
    vault.EmplaceComponent<GameState>(gameStateEntity);
}

void GameStateSystem::Update(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }

    auto& [gameState, gameStateEntity] = *vault.GetComponents<GameState>().begin();

    // We give game a single frame to react to game state changes
    if (gameState.updatedIsBuildingWeapon) {
        gameState.updatedIsBuildingWeapon = false;
    }

    if (Input::IsKeyPressed(KeyCode::E)) {
        gameState.isBuildingWeapon = !gameState.isBuildingWeapon;
        gameState.updatedIsBuildingWeapon = true;
    }
}
