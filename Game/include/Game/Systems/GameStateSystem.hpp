#pragma once

#include <ECS/System.hpp>

class GameStateSystem final {

    GENERATE_SYSTEM_BODY(GameStateSystem)

public:
    void Start(ThreadContext& threadContext, Vault& vault);
    void Update(ThreadContext& threadContext, Vault& vault);
};
