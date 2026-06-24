
#include "Editor/Systems/EditorStateSystem.hpp"

#include <Runtime/Engine.hpp>
#include <imgui.h>
#include <imgui_internal.h>

#include "Core/EngineGlobals.hpp"
#include "Core/Time.hpp"
#include "Editor/EditorGlobals.hpp"
#include "Runtime/Input.hpp"

static void HandleAttachDetach() {

    static bool previousCursorVisibility = Input::IsCursorVisible();

    if (gDetachedFromGame) {
        previousCursorVisibility = Input::IsCursorVisible();
        Input::SetCursorVisibility(true);
    } else {
        // We suppress the next delta so the game does not update its state based on the editor cursor's movements
        Input::SuppressNextCursorDelta();
        Input::SetCursorVisibility(previousCursorVisibility);

        // Attaching to game will prevent ImGui from receiving input, as such,
        // when attaching via button press ImGui's last stored cursor position will become the button location (e.g. Attach/Play buttons).
        // These buttons have hover state on them thus they will be stuck in hover state resulting
        // in a persistent pop-up for example. We solve this issue by telling ImGui that their last cursor location
        // is instead the botton left of the screen where nothing exists.
        ImGui::GetIO().AddMousePosEvent(0.f, 0.f);
    }

    if (gDetachedFromGame && gEditorCamera && gCamera) {
        gEditorCamera->Assign(*gCamera);
    }
}

namespace ImGui {
    static bool PlayStopButton(float buttonSize, bool& isPlaying) {
        ImDrawList* dl = ImGui::GetWindowDrawList();

        bool pressedButton = false;

        if (ImGui::InvisibleButton("##playStop", ImVec2(buttonSize, buttonSize))) {
            isPlaying = !isPlaying;
            pressedButton = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(isPlaying ? "Stop" : "Play");
        }

        // Draw Icon
        {
            const ImVec2 mn = ImGui::GetItemRectMin();
            const ImVec2 mx = ImGui::GetItemRectMax();

            ImVec4 iconColor;
            if (ImGui::IsItemActive()) {
                iconColor = isPlaying ? ImVec4(1.f, 0.f, 0.f, 1.f) : ImVec4(0.f, 1.f, 0.f, 1.f);
            } else if (gDetachedFromGame && ImGui::IsItemHovered()) {
                iconColor = isPlaying ? ImVec4(.9f, 0.f, 0.f, 1.f) : ImVec4(0.f, .9f, 0.f, 1.f);
            } else if (ImGui::IsItemDisabled()) {
                iconColor = ImVec4(1.f, 1.f, 1.f, ImGui::GetStyle().DisabledAlpha);
            } else{
                iconColor = isPlaying ? ImVec4(.8f, 0.f, 0.f, 1.f) : ImVec4(0.f, .8f, 0.f, 1.f);
            }

            const ImU32 col = ImGui::ColorConvertFloat4ToU32(iconColor);

            const float cx = (mn.x + mx.x) * .5f;
            const float cy = (mn.y + mx.y) * .5f;
            const float h  = (mx.y - mn.y) * .65f;

            if (!isPlaying) {
                const float w = h * .866f;
                dl->AddTriangleFilled(
                    ImVec2(cx - w * .5f, cy - h * .5f),
                    ImVec2(cx - w * .5f, cy + h * .5f),
                    ImVec2(cx + w * .5f, cy), col);
            } else {
                const float s = h;
                dl->AddRectFilled(
                    ImVec2(cx - s * .5f, cy - s * .5f),
                    ImVec2(cx + s * .5f, cy + s * .5f), col);
            }
        }

        return pressedButton;
    }

    static bool PauseResumeButton(float buttonSize, bool& isPaused) {

        ImDrawList* dl = ImGui::GetWindowDrawList();

        bool pressedButton = false;

        if (ImGui::InvisibleButton("##resumePause", ImVec2(buttonSize, buttonSize))) {
            isPaused = !isPaused;
            pressedButton = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(isPaused ? "Resume" : "Pause");
        }

        // Draw Icon
        {
            const ImVec2 mn = ImGui::GetItemRectMin();
            const ImVec2 mx = ImGui::GetItemRectMax();

            ImVec4 iconColor;
            if (ImGui::IsItemActive()) {
                iconColor = ImVec4(1.f, 1.f, 1.f, 1.f);
            } else if (ImGui::IsItemHovered()) {
                iconColor = ImVec4(0.9f, 0.9f, 0.9f, 1.f);
            } else if (ImGui::IsItemDisabled()) {
                iconColor = ImVec4(0.8f, 0.8f, 0.8f, ImGui::GetStyle().DisabledAlpha);
            } else {
                iconColor = ImVec4(0.8f, 0.8f, 0.8f, 1.f);
            }

            const ImU32 col = ImGui::ColorConvertFloat4ToU32(iconColor);

            const float cx = (mn.x + mx.x) * .5f;
            const float cy = (mn.y + mx.y) * .5f;
            const float h  = (mx.y - mn.y) * .65f;

            if (isPaused) {
                const float w = h * .866f;
                dl->AddTriangleFilled(
                    ImVec2(cx - w * .5f, cy - h * .5f),
                    ImVec2(cx - w * .5f, cy + h * .5f),
                    ImVec2(cx + w * .5f, cy), col);
            } else {
                const float barW = h * .22f;
                const float gap  = h * .2f;
                dl->AddRectFilled(ImVec2(cx - gap * .5f - barW, cy - h * .5f),
                                  ImVec2(cx - gap * .5f,        cy + h * .5f), col);
                dl->AddRectFilled(ImVec2(cx + gap * .5f,        cy - h * .5f),
                                  ImVec2(cx + gap * .5f + barW, cy + h * .5f), col);
            }
        }

        return pressedButton;
    }

    static bool DetachAttachButton(float buttonSize, bool& isDetached) {

        ImDrawList* dl = ImGui::GetWindowDrawList();

        bool pressedButton = false;
        if (ImGui::InvisibleButton("##detachAttach", ImVec2(buttonSize, buttonSize))) {
            isDetached = !isDetached;
            pressedButton = true;
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(isDetached ? "Attach" : "Detach");
        }

        // Draw Icon
        {
            const ImVec2 mn = ImGui::GetItemRectMin();
            const ImVec2 mx = ImGui::GetItemRectMax();

            ImVec4 iconColor;
            if (ImGui::IsItemActive()) {
                iconColor = ImVec4(1.f, 1.f, 1.f, 1.f);
            } else if (ImGui::IsItemHovered()) {
                iconColor = ImVec4(0.9f, 0.9f, 0.9f, 1.f);
            } else if (ImGui::IsItemDisabled()) {
                iconColor = ImVec4(0.8f, 0.8f, 0.8f, ImGui::GetStyle().DisabledAlpha);
            } else {
                iconColor = ImVec4(0.8f, 0.8f, 0.8f, 1.f);
            }

            const ImU32 col = ImGui::ColorConvertFloat4ToU32(iconColor);

            const float cx = (mn.x + mx.x) * .5f;
            const float cy = (mn.y + mx.y) * .5f;
            const float h  = (mx.y - mn.y) * .65f;
            const float w  = h * .866f;

            const float barH = h * .18f;
            const float gap  = h * .2f;
            const float triH = h - barH - gap;

            // Eject icon: triangle + bar, total height = h
            if (!isDetached) {
                const float top = cy - h * .5f;
                dl->AddTriangleFilled(
                    ImVec2(cx - w * .5f, top + triH),
                    ImVec2(cx + w * .5f, top + triH),
                    ImVec2(cx,           top), col);
                dl->AddRectFilled(
                    ImVec2(cx - w * .5f, top + triH + gap),
                    ImVec2(cx + w * .5f, top + h), col);
            } else {
                const float top = cy - h * .5f;
                dl->AddTriangleFilled(
                    ImVec2(cx - w * .5f, top),
                    ImVec2(cx + w * .5f, top),
                    ImVec2(cx,           top + triH), col);
                dl->AddRectFilled(
                    ImVec2(cx - w * .5f, top + triH + gap),
                    ImVec2(cx + w * .5f, top + h), col);
            }
        }

        return pressedButton;
    }
}

void EditorStateSystem::Update(ThreadContext& threadContext, Vault& vault) {

    if (!threadContext.IsMainThread()) {
        return;
    }

    bool changedPlayingState = false;
    bool changedPausedState = false;
    bool changedDetachedState = false;

    // Only accept Pause/Resume or Attach/Detach inputs when actually playing the game
    if (Input::IsKeyDown(KeyCode::LAlt) && Input::IsKeyPressed(KeyCode::P)) {
        gIsPlayingGame = !gIsPlayingGame;
        changedPlayingState = true;

    } else if (gIsPlayingGame) {
        if (Input::IsKeyPressed(KeyCode::P)) {
            gTime.paused = !gTime.paused;
            changedPausedState = true;
        }

        if (Input::IsKeyPressed(KeyCode::F8)) {
            gDetachedFromGame = !gDetachedFromGame;
            changedDetachedState = true;
        }
    }

    if (ImGui::BeginMainMenuBar()) {

        const float menuBarWidth = ImGui::GetWindowWidth();
        const float buttonSize = ImGui::GetFrameHeight();
        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        const float totalWidth  = buttonSize * 3.f + spacing * 2.f;

        const float prevCursorX = ImGui::GetCursorPosX();
        ImGui::SetCursorPosX((menuBarWidth - totalWidth) * .5f);

        if (ImGui::PlayStopButton(buttonSize, gIsPlayingGame)) {
            changedPlayingState = true;
        }

        ImGui::BeginDisabled(!gIsPlayingGame);

        if (ImGui::PauseResumeButton(buttonSize, gTime.paused)) {
            changedPausedState = true;
        }

        if (ImGui::DetachAttachButton(buttonSize, gDetachedFromGame)) {
            changedDetachedState = true;
        }
        ImGui::EndDisabled();

        ImGui::SetCursorPosX(prevCursorX);

        ImGui::EndMainMenuBar();
    }

    // Playing state takes priority over all the rest
    if (changedPlayingState) {

        if (gIsPlayingGame) {
            gEngine->PlayGame();
            gDetachedFromGame = false;
        } else {
            gEngine->StopGame();
            gDetachedFromGame = true;
        }
        HandleAttachDetach();

        gTime.paused = false;

    } else {

        if (changedPausedState) {

            if (gTime.paused) {
                gDetachedFromGame = true;
                HandleAttachDetach();
            }

        } else if (changedDetachedState) {

            HandleAttachDetach();

            // When paused and attaching we unpause game
            if (gTime.paused && !gDetachedFromGame) {
                gTime.paused = false;
            }
        }
    }
}
