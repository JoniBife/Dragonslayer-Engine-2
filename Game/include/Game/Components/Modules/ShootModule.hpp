#pragma once

#include <ECS/Component.hpp>

#include "Game/StaticGameSettings.hpp"

DSTRUCT(ShootModule, .numComponents = MAX_MODULES, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DSTRUCT_BODY(ShootModule)

    float force = 150.f;
    float rate = .1f;
    float randomRateOffset = 0.f;
    float time = 0.f;

    void RandomizeIntervalOffset() {
        randomRateOffset = (RandomFloat(0.f, 1.f)*2.f - 1.f)*.2f*rate;
    }

#if WITH_EDITOR
    void OnHierarchy(){
        ImGui::InputFloat("Shoot Force", &force);
        ImGui::InputFloat("Shoot Interval", &rate);
    }
#endif

}; END_DSTRUCT(ShootModule)
