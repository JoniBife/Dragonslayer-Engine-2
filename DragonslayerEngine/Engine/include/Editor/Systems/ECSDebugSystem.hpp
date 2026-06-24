#pragma once

#include "ECS/System.hpp"

class ECSDebugSystem final {

    GENERATE_SYSTEM_BODY(ECSDebugSystem)

public:
    void Update(ThreadContext& threadContext, Vault& vault);
};
