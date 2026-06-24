#pragma once

#include <ECS/Component.hpp>

#include "Game/StaticGameSettings.hpp"

DSTRUCT(ChainModule, .numComponents = MAX_MODULES, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DSTRUCT_BODY(ChainModule)

    float length = 1.f;

#if WITH_EDITOR
    void OnHierarchy() {
        ImGui::InputFloat("Length", &length);
    }
#endif

}; END_DSTRUCT(ChainModule)
