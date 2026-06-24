#pragma once

#include <ECS/System.hpp>

class WeaponEditorCameraSystem final {

    GENERATE_SYSTEM_BODY(WeaponEditorCameraSystem)

public:
    void Start(ThreadContext& threadContext, Vault& vault);
    void Update(ThreadContext& threadContext, Vault& vault);
};
