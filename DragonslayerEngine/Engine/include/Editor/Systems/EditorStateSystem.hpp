#pragma once

#include "ECS/System.hpp"

class EditorStateSystem final {

    GENERATE_SYSTEM_BODY(EditorStateSystem)

public:
    void Update(ThreadContext& threadContext, Vault& vault);
};
