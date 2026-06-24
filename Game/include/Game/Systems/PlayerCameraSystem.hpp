#pragma once

#include <ECS/System.hpp>

class PlayerCameraSystem final {

    GENERATE_SYSTEM_BODY(PlayerCameraSystem)

public:
    void Start(ThreadContext& threadContext, Vault& vault);
    void Update(ThreadContext& threadContext, Vault& vault);
};
