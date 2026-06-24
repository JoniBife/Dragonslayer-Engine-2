#pragma once

#include "ECS/System.hpp"

class TimeSystem final {

    GENERATE_SYSTEM_BODY(TimeSystem)

public:
    void Start(ThreadContext& threadContext, Vault& vault);
    void Update(ThreadContext& threadContext, Vault& vault);
};
