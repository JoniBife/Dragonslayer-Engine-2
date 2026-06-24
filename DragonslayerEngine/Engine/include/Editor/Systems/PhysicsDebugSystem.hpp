#pragma once

#include "ECS/System.hpp"

class PhysicsDebugSystem final {

    GENERATE_SYSTEM_BODY(PhysicsDebugSystem)

public:
    void PostPhysicsUpdate(ThreadContext& threadContext, Vault& vault);

    void Update(ThreadContext& threadContext, Vault& vault);
};
