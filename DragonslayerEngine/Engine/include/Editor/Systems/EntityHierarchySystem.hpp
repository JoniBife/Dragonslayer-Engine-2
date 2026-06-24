#pragma once

#include "ECS/System.hpp"

class EntityHierarchySystem final {

    GENERATE_SYSTEM_BODY(EntityHierarchySystem)

public:
    void Update(ThreadContext& threadContext, Vault& vault);
};
