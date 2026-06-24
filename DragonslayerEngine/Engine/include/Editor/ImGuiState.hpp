#pragma once

#include <imgui.h>

#include "Core/Export.hpp"

namespace ImGui {

    struct ENGINE_API State {
        ImGuiContext* context;
        ImGuiMemAllocFunc allocFunc;
        ImGuiMemFreeFunc freeFunc;
        void* userData;
    };

    ENGINE_API void GetState(State& state);
}
