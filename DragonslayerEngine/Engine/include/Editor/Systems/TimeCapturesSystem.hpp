#pragma once

#include "ECS/System.hpp"

class TimeCapturesSystem final {

    GENERATE_SYSTEM_BODY(TimeCapturesSystem)

public:
    void Update(ThreadContext& threadContext, Vault& vault);
};
