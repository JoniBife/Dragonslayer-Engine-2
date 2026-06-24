#pragma once

#include <ECS/System.hpp>

class EnemyAISystem final {

    GENERATE_SYSTEM_BODY(EnemyAISystem)

public:
    void Start(ThreadContext& threadContext, Vault& vault);
    void PrePhysicsUpdate(ThreadContext& threadContext, Vault& vault);
    void Update(ThreadContext& threadContext, Vault& vault);
};

