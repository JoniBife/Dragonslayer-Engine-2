#pragma once

#include <ECS/System.hpp>

// Try changing update order
// First, move player, but don't update transform nor rotation
// then update transform only when physics end because it happens
// after the rendering
class PlayerSystem final {

    GENERATE_SYSTEM_BODY(PlayerSystem)

public:
    void Start(ThreadContext& threadContext, Vault& vault);
    void Update(ThreadContext& threadContext, Vault& vault);
    void PrePhysicsUpdate(ThreadContext& threadContext, Vault& vault);
    void PostPhysicsUpdate(ThreadContext& threadContext, Vault& vault);
};
