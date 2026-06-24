#pragma once

#include "ECS/System.hpp"

class LogSystem final {

    GENERATE_SYSTEM_BODY(LogSystem)

public:
    void Update(ThreadContext& threadContext, Vault& vault);
    void End(ThreadContext& threadContext, Vault& vault);
};
