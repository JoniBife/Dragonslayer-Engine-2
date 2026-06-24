
#include "Editor/Systems/ECSDebugSystem.hpp"

#include <imgui.h>

#include "Runtime/Input.hpp"

void ECSDebugSystem::Update(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }

    static bool showECSData = false;
    if (Input::IsKeyHeld(KeyCode::LControl) && Input::IsKeyPressed(KeyCode::Two)) {
        showECSData = !showECSData;
    }

    if (ImGui::MainMenuBarItem("Stats", "ECS", "CTRL+2", &showECSData)) {

        ImGui::Begin("ECS Data", &showECSData);

        uint32 numberOfEntities = 0;
        uint32 numberOfComponents = 0;
        for (Entity entity : vault.GetAllEntities()) {
            ++numberOfEntities;
            numberOfComponents += vault.GetNumberOfComponents(entity);
        }
        ImGui::Text("Num Entities: %d", numberOfEntities);
        ImGui::Text("Num Components: %d", numberOfComponents);
        //ImGui::Text("Num Systems: %d", SystemMetadata::numSystems);
        ImGui::Text("Memory Used: %zu mb", vault.GetAllocator().GetUsedMemory() / (1024 * 1024));

        ImGui::End();
    }
}