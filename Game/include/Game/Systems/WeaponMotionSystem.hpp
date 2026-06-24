#pragma once

#include <ECS/System.hpp>

class WeaponMotionSystem final {

    GENERATE_SYSTEM_BODY(WeaponMotionSystem)

public:
    void Start(ThreadContext& threadContext, Vault& vault);
    void Update(ThreadContext& threadContext, Vault& vault);
    void PrePhysicsUpdate(ThreadContext& threadContext, Vault& vault);
};
