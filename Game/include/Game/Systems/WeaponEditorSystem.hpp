#pragma once

#include <Core/Allocators/FreeListAllocator.hpp>
#include <ECS/System.hpp>

class WeaponEditorSystem final {

    GENERATE_SYSTEM_BODY(WeaponEditorSystem)

public:
    void Start(ThreadContext& threadContext, Vault& vault);
    void Update(ThreadContext& threadContext, Vault& vault);
};
