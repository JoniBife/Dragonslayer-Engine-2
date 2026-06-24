#include "Editor/ImGuiState.hpp"

void ImGui::GetState(State& imGuiState) {
    imGuiState.context = ImGui::GetCurrentContext();
    ImGui::GetAllocatorFunctions(&imGuiState.allocFunc, &imGuiState.freeFunc, &imGuiState.userData);
}
