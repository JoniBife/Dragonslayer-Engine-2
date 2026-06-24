#pragma once

#include "ECS/System.hpp"

class TransformGizmosSystem final {

    GENERATE_SYSTEM_BODY(TransformGizmosSystem)

public:
    void Start(ThreadContext& threadContext, Vault& vault);
    void Update(ThreadContext& threadContext, Vault& vault);
};
