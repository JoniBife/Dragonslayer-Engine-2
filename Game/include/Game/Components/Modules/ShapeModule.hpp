#pragma once

#include <ECS/Component.hpp>

#include "Game/StaticGameSettings.hpp"

DSTRUCT(ShapeModule, .numComponents = MAX_MODULES, .hasMetadata = true, .isDisplayable = true) final {
    GENERATE_DSTRUCT_BODY(ShapeModule)

    uint32 shapeType = 0;

#if WITH_EDITOR
    void OnHierarchy() {
        ImGui::InputInt("ShapeType", reinterpret_cast<int*>(&shapeType));
    }
#endif

}; END_DSTRUCT(ShapeModule)
