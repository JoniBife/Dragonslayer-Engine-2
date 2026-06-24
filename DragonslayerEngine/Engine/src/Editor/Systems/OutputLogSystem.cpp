#include "Editor/Systems/OutputLogSystem.hpp"

#include "Core/Log.hpp"
#include "Core/Containers/ArrayHelper.hpp"

static bool autoScroll = true;
static bool onlyErrors = false;
static bool onlyWarnings = false;

void OutputLogSystem::Update(ThreadContext& threadContext, Vault& vault) {

    // TODO Its missing Thread Index as part of the output!

    if (!threadContext.IsMainThread()) {
        return;
    }

    static bool showOutputLog = true;
    if (!ImGui::MainMenuBarItem("Window", "Output Log", "", &showOutputLog)) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Output Log", &showOutputLog)) {
        // As a specific feature guaranteed by the library, after calling Begin() the last Item represent the title bar.
        // So e.g. IsItemHovered() will return true when hovering the title bar.
        // Here we create a context menu only available from the title bar.
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Close Console")) {
                showOutputLog = false;
            }
            ImGui::EndPopup();
        }

        // Options menu
        if (ImGui::BeginPopup("Options"))
        {
            ImGui::Checkbox("Auto-scroll", &autoScroll);
            ImGui::Checkbox("Errors only", &onlyErrors);
            ImGui::Checkbox("Warnings only", &onlyWarnings);
            ImGui::EndPopup();
        }
        if (ImGui::Button("Options")) {
            ImGui::OpenPopup("Options");
        }

        // Clear Logs button
        ImGui::SameLine();
        if (ImGui::Button("Clear Logs")) {
            Log::ClearLogs();
        }

        // Option Filter
        ImGui::SameLine();
        static ImGuiTextFilter textFilter;
        if (ImGui::InputTextWithHint("##Filter", "incl,-excl", textFilter.InputBuf, IM_ARRAYSIZE(textFilter.InputBuf), ImGuiInputTextFlags_EscapeClearsAll)) {
            textFilter.Build();
        }

        ImGui::Separator();

        // Reserve enough left-over height for 1 separator + 1 input text
        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_HorizontalScrollbar))
        {
            if (ImGui::BeginPopupContextWindow())
            {
                if (ImGui::Selectable("Clear")) {
                    Log::ClearLogs();
                }
                ImGui::EndPopup();
            }

            const auto& logs = Log::GetLogs();

            if (logs.GetSize() > 0) {
                // Build list of valid indices
                TempArray<uint32> filteredIndices(logs.GetSize());

                for (uint32 i = 0; i < logs.GetSize(); ++i) {
                    const LogEntry& logEntry = logs[i];

                    if (onlyErrors && logEntry.type != LogType::Error) {
                        continue;
                    }
                    if (onlyWarnings && logEntry.type != LogType::Warning) {
                        continue;
                    }
                    if (!textFilter.PassFilter(logEntry.text.CStr())) {
                        continue;
                    }

                    filteredIndices.Add(i);
                }

                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

                // Calculate wrap width based on the window
                const float warpWidth = ImGui::GetContentRegionAvail().x;
                ImGui::PushTextWrapPos(warpWidth);

                const float itemSpacingY = ImGui::GetStyle().ItemSpacing.y;
                const uint32 itemCount = filteredIndices.GetSize();

                // Manual virtualization: ImGuiListClipper requires uniform item height,
                // but wrapped text varies per item. Precompute heights, compute the
                // visible range from scroll position, render only that range, and pad
                // before/after with Dummy to keep total content size correct.
                TempArray<float> rowHeights(itemCount);
                float totalHeight = 0.0f;
                for (uint32 i = 0; i < itemCount; ++i) {
                    const LogEntry& logEntry = logs[filteredIndices[i]];
                    const float h = ImGui::CalcTextSize(logEntry.text.CStr(), nullptr, false, warpWidth).y;
                    rowHeights.Add(h);
                    totalHeight += h + itemSpacingY;
                }

                const float scrollY = ImGui::GetScrollY();
                const float viewHeight = ImGui::GetWindowSize().y;

                uint32 firstVisible = 0;
                float skippedHeight = 0.0f;
                {
                    float y = 0.0f;
                    for (uint32 i = 0; i < itemCount; ++i) {
                        const float rowAdvance = rowHeights[i] + itemSpacingY;
                        if (y + rowAdvance > scrollY) {
                            firstVisible = i;
                            skippedHeight = y;
                            break;
                        }
                        y += rowAdvance;
                        firstVisible = i + 1;
                        skippedHeight = y;
                    }
                }

                if (skippedHeight > 0.0f) {
                    ImGui::Dummy(ImVec2(0.0f, skippedHeight));
                }

                float renderedHeight = 0.0f;
                const float visibleBudget = viewHeight + (scrollY - skippedHeight);
                uint32 lastRendered = firstVisible;
                for (uint32 i = firstVisible; i < itemCount; ++i) {
                    const LogEntry& logEntry = logs[filteredIndices[i]];
                    const char* textStr = logEntry.text.CStr();

                    ImGui::PushID(static_cast<int>(i));

                    const float textSizeY = rowHeights[i];

                    ImGui::Selectable("##row", false, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, textSizeY));

                    if (ImGui::BeginPopupContextItem("LogMenu")) {
                        if (ImGui::MenuItem("Copy Message")) {
                            ImGui::SetClipboardText(textStr);
                        }
                        ImGui::EndPopup();
                    }

                    ImGui::SameLine(0, 0);

                    bool pushedColor = false;
                    if (logEntry.type == LogType::Warning) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                        pushedColor = true;
                    } else if (logEntry.type == LogType::Error) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                        pushedColor = true;
                    }

                    ImGui::TextUnformatted(textStr);

                    if (pushedColor) {
                        ImGui::PopStyleColor();
                    }

                    ImGui::PopID();

                    renderedHeight += textSizeY + itemSpacingY;
                    lastRendered = i + 1;
                    if (renderedHeight >= visibleBudget) {
                        break;
                    }
                }

                // Pad remaining items so scrollbar reflects full content height
                float trailing = 0.0f;
                for (uint32 i = lastRendered; i < itemCount; ++i) {
                    trailing += rowHeights[i] + itemSpacingY;
                }
                if (trailing > 0.0f) {
                    ImGui::Dummy(ImVec2(0.0f, trailing));
                }

                ImGui::PopTextWrapPos();

                // When auto-scrolling we jump to the end of the window's Y axis
                if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                    ImGui::SetScrollHereY(1.0);
                }

                ImGui::PopStyleVar();
            }
        }
        ImGui::EndChild();
        ImGui::Separator();
    }
    ImGui::End();
}