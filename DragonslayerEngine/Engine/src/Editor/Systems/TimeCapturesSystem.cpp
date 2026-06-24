#include "Editor/Systems/TimeCapturesSystem.hpp"

#include <Core/Containers/ArrayHelper.hpp>

#include "Core/Time.hpp"
#include "Editor/TimeCapturesManager.hpp"
#include "Runtime/Input.hpp"

void TimeCapturesSystem::Update(ThreadContext& threadContext, Vault& vault)
{
    TimeCapturesManager& timeCapturesManager = TimeCapturesManager::Instance();

    static bool showPerformanceData = false;
    static bool showFrameGraph = false;

    if (threadContext.IsMainThread()) {
        if (Input::IsKeyHeld(KeyCode::LControl) && Input::IsKeyPressed(KeyCode::One)) {
            showPerformanceData = !showPerformanceData;
        }

        if (Input::IsKeyHeld(KeyCode::LControl) && Input::IsKeyPressed(KeyCode::Six)) {
            showFrameGraph = !showFrameGraph;
        }
    }
    threadContext.Sync();

    if (showPerformanceData) {
        // We sort the captures stored per thread
        timeCapturesManager.SortThreadCaptures();
    }
    threadContext.Sync();

    if (threadContext.IsMainThread()) {

        if (ImGui::MainMenuBarItem("Stats", "Timing Captures", "CTRL+1", &showPerformanceData)) {

            ImGui::Begin("Timing Captures", &showPerformanceData);

            static int32 threadIndexFilter = -1;
            ImGui::InputInt("Thread Index Filter", &threadIndexFilter);

            const float smoothingFactor = timeCapturesManager.GetSmoothingFactor();
            float newSmoothingFactor = smoothingFactor;
            if (ImGui::SliderFloat("SmoothingFactor", &newSmoothingFactor, 0.f, 1.f)) {
                timeCapturesManager.SetSmoothingFactor(newSmoothingFactor);
            }

            // Entity Name Filter
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F, ImGuiInputFlags_Tooltip);
            ImGui::PushItemFlag(ImGuiItemFlags_NoNavDefaultFocus, true);
            static ImGuiTextFilter textFilter;
            if (ImGui::InputTextWithHint("##Filter", "incl,-excl", textFilter.InputBuf, IM_ARRAYSIZE(textFilter.InputBuf), ImGuiInputTextFlags_EscapeClearsAll))
                textFilter.Build();
            ImGui::PopItemFlag();
            ImGui::Separator();

            if (ImGui::BeginTable("Performance Table", 3, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersV))
            {
                const float totalFrameMs = gTime.deltaTime * 1000.f;

                ImGui::TableSetupColumn("Thread", ImGuiTableColumnFlags_WidthFixed, 80.f);
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Duration");
                ImGui::TableHeadersRow();

                const Array<Array<TimeCapture, FreeListAllocator>, FreeListAllocator>& capturesPerThread = timeCapturesManager.GetCapturesPerThread();
                for (uint32 i = 0; i < capturesPerThread.GetSize(); ++i) {

                    if (threadIndexFilter > -1 && i != threadIndexFilter) {
                        continue;
                    }

                    for (const auto& [name, duration, threadIndex] : capturesPerThread[i])
                    {
                        // Captures which pass the text filter
                        if (!textFilter.PassFilter(name.CStr())) {
                            continue;
                        }

                        const float performanceRatio = fmin(duration / 1.f, 1.f);

                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%d", threadIndex);

                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s", name.CStr());

                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%f ms", duration);
                    }
                }

                constexpr float targetFramerate = 120.f;
                constexpr float targetMs = 1000.f / targetFramerate;
                // We go from green at or below target MS to red at a quarter larger than target ms
                const float totalPerformanceRatio = fmax(0.f, totalFrameMs - targetMs) / (targetMs * .25f);

                const ImVec4* colors = ImGui::GetStyle().Colors;
                const ImU32 bgColor = ImGui::GetColorU32(colors[ImGuiCol_TableRowBgAlt]);

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Total");
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, bgColor);

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("");
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, bgColor);

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%f ms", totalFrameMs);
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, bgColor);

                ImGui::EndTable();
            }

            ImGui::End();
        }

        if (ImGui::MainMenuBarItem("Stats", "Frame Graph", "CTRL+6", &showFrameGraph)) {

            ImGui::Begin("Frame Graph", &showFrameGraph);

            static bool updateGraph = true;

            static float timings[512] = {};
            static int offset = 0;

            if (updateGraph) {
                timings[offset] = gTime.deltaTime * 1000.f;
                offset = (offset + 1) % IM_ARRAYSIZE(timings);
            }

            ImGui::PlotLines("Delta Time (ms) ", timings, IM_ARRAYSIZE(timings), offset, "", 0.f, 33.f, ImVec2(0, 100.0f));
            ImGui::Checkbox("Update Graph", &updateGraph);

            ImGui::End();
        }
    }
    threadContext.Sync();
}
