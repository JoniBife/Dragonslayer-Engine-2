#pragma once

#include <ECS/Component.hpp>

DSTRUCT(Enemy, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DSTRUCT_BODY(Enemy)

    float maxHealth = 100.f;
    float currentHealth = 0.f;

#if WITH_EDITOR
    void OnHierarchy() {
        ImGui::InputFloat("Max Health", &maxHealth);
        ImGui::Text("Current Health %f", currentHealth);
    }
#endif

}; END_DSTRUCT(Enemy)

