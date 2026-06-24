#pragma once

#include <ECS/System.hpp>

class MapSystem final {

    GENERATE_SYSTEM_BODY(MapSystem)

public:
    void Start(ThreadContext& threadContext, Vault& vault);
    void Update(ThreadContext& threadContext, Vault& vault);
};
