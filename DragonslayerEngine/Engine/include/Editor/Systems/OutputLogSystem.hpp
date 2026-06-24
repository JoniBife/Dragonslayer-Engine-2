#pragma once

#include "ECS/System.hpp"

class OutputLogSystem final {

    GENERATE_SYSTEM_BODY(OutputLogSystem)

public:
    void Update(ThreadContext& threadContext, Vault& vault);
};
