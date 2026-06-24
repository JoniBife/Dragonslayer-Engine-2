#include "Editor/Systems/RendererDebugSystem.hpp"
#include "Editor/ImGuiExtensions.hpp"
#include <Renderer/Renderer.h>
#include <imgui.h>
#include <Input/Input.hpp>
#include <Core/Time.hpp>
#include <Core/Components/MeshRenderer.hpp>

using namespace DragonslayerEngine;

void RendererDebugSystem::Update()
{
    static bool showRenderingData = false;
    const Input& input = GetGlobal<const Input>();

    if (input.IsKeyHeld(KeyCode::LEFT_CTRL) && input.IsKeyPressed(KeyCode::THREE)) {
        showRenderingData = !showRenderingData;
    }

    ImGui::OnMainSubMenu(registry, "Stats", [&](){
        ImGui::MenuItem("Rendering", "CTRL+3", &showRenderingData);
    });

    if (showRenderingData)
    {
        const Time& time = GetGlobal<const Time>();
        auto& renderer = GetGlobal<Shared<Renderer>>();
        const double geometryTime =  renderer->getFrameTime(RenderPass::GEOMETRY);
        const double lightTime =  renderer->getFrameTime(RenderPass::LIGHT);
        const double shadowTime =  renderer->getFrameTime(RenderPass::SHADOW);
        const double postProcessingTime =  renderer->getFrameTime(RenderPass::POSTPROCESSING);
        const double ssaoTime =  renderer->getFrameTime(RenderPass::SSAO);

        const double deltaTimeSeconds = time.deltaTime * 1000.;
        const double gpuTimeSeconds = geometryTime + lightTime + shadowTime + postProcessingTime + ssaoTime;
        const double cpuTimeSeconds = deltaTimeSeconds - gpuTimeSeconds;

        const auto meshRenderers = FindEntitiesWithComponents<MeshRenderer>();
        int meshRenderersCount = 0;
        for (auto [_, meshRenderer] : meshRenderers.each()) {
            ++meshRenderersCount;
        }

        ImGui::Begin("Rendering Data", &showRenderingData);
        ImGui::Text("Num MeshRenderers: %d", meshRenderersCount);
        ImGui::TextPerformance(static_cast<float>(geometryTime / 1000), "Geometry pass: %f", geometryTime);
        ImGui::TextPerformance(static_cast<float>(lightTime / 1000), "Light pass: %f", lightTime);
        ImGui::TextPerformance(static_cast<float>(shadowTime / 1000), "Shadow pass: %f", shadowTime);
        ImGui::TextPerformance(static_cast<float>(postProcessingTime / 1000), "Post-processing pass: %f", postProcessingTime);
        ImGui::TextPerformance(static_cast<float>(ssaoTime / 1000), "Ssao pass: %f", ssaoTime);

        ImGui::TextPerformance(static_cast<float>(cpuTimeSeconds / 1000.), "Delta time (CPU) : %f", cpuTimeSeconds);
        ImGui::TextPerformance(static_cast<float>(gpuTimeSeconds / 1000.), "Delta time (GPU) : %f", gpuTimeSeconds);
        ImGui::TextPerformance(static_cast<float>(time.deltaTime / 16.), "Delta time (GPU + CPU) : %f", deltaTimeSeconds);

        ImGui::Text("Framerate : %d", static_cast<unsigned int>(ImGui::GetIO().Framerate));
        ImGui::End();
    }

    ImGui::OnMainMenu(registry, []() {
        // FPS text
        const std::string fpsText = std::to_string(static_cast<int>(ImGui::GetIO().Framerate)) + " FPS";
        const ImVec2 textSize = ImGui::CalcTextSize(fpsText.c_str());

        // Get menu bar full width and current cursor position
        const float menuBarWidth = ImGui::GetWindowWidth();
        const float padding = ImGui::GetStyle().WindowPadding.x;

        // Position cursor to right-aligned position
        const float prevCursorPos = ImGui::GetCursorPosX();
        ImGui::SetCursorPosX(menuBarWidth - textSize.x - padding);
        ImGui::Text(fpsText.c_str());
        ImGui::SetCursorPosX(prevCursorPos);
    });
}
