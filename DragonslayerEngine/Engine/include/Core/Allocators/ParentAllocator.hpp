#pragma once

#include "Core/Macros.hpp"
#include "Core/Types.hpp"

/*
 * Helper type that stores a typeless parent allocator
 * and its respective free function.
 * This is used by allocators when freeing their memory on destruction
 */
struct ParentAllocator {

    void* parentAllocator = nullptr;
    void (*freeFunction)(void*, uint8*) = nullptr;

    ParentAllocator() = default;

    template<typename ParentType>
    explicit ParentAllocator(ParentType& parentAllocator) : parentAllocator(&parentAllocator) {
        struct FreeFunction {
            static void Free(void* parentAllocator, uint8* blockAddress) {
                static_cast<ParentType*>(parentAllocator)->Free(blockAddress);
            }
        };
        freeFunction = FreeFunction::Free;
    }

    void Free(uint8* blockAddress) {
        freeFunction(parentAllocator, blockAddress);
    }

    NO_DISCARD bool Exists() const {
        return parentAllocator != nullptr;
    }
};
