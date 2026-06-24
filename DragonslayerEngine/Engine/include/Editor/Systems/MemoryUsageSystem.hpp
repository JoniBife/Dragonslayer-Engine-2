#pragma once

#include "ECS/System.hpp"

class MemoryUsageSystem final {

    GENERATE_SYSTEM_BODY(MemoryUsageSystem)

public:
    void Update(ThreadContext& threadContext, Vault& vault);
};
