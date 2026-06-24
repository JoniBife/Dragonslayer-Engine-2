#pragma once

#include <cstddef>
#include <mutex>
#include <utility>

#include "Core/Assert.hpp"
#include "Core/Export.hpp"
#include "Core/Macros.hpp"
#include "Core/NotCopyable.hpp"
#include "Core/Types.hpp"
#include "ParentAllocator.hpp"

/*
 * Linear allocator
 * - Memory DOES not get freed on destruction (Free needs to be called explicitly)
 * TODO Should it be copyable?
 */
class ENGINE_API LinearAllocator final : public NotCopyable {

private:
    ParentAllocator parentAllocator;

    uint8* baseAddress;
    uint8* topAddress; // The address at the end of the last allocation
    size_t totalMemory;

public:
    explicit LinearAllocator(size_t size, size_t alignment = alignof(std::max_align_t));

    template<typename ParentAllocator>
    LinearAllocator(ParentAllocator& parentAllocator, size_t size, size_t alignment = alignof(std::max_align_t)) :
        parentAllocator(parentAllocator),
        baseAddress(parentAllocator.Allocate(size, alignment)),
        topAddress(baseAddress),
        totalMemory(size) {
        ASSERT(size > 0);
    }

    // TODO Decide if we add a destructor
    //~LinearAllocator() = delete;

    NO_DISCARD uint8* Allocate(size_t size, size_t alignment = alignof(std::max_align_t));

    template<typename Type, typename... Args>
    Type* Allocate(Args&&... args) {
        Type* objectMemory = reinterpret_cast<Type*>(Allocate(sizeof(Type), alignof(Type)));
        new (objectMemory) Type(std::forward<Args>(args)...);
        return objectMemory;
    }

    // Does absolutely nothing, here purely for use in containers!
    NO_DISCARD bool TryExtend(uint8* blockAddress, size_t newSize) { return false; }

    void Free();

    // Does absolutely nothing, here purely for use in containers!
    FORCE_INLINE void Free(uint8* blockAddress) {}

    template<typename Type>
    void Free(Type* address) {
        if (address) {
            address->~Type();
            // Pointless! -> Free(reinterpret_cast<uint8*>(address));
        }
    }

    FORCE_INLINE void Clear() { topAddress = baseAddress; }

    NO_DISCARD FORCE_INLINE size_t GetUsedMemory() const { return topAddress - baseAddress; }
    NO_DISCARD FORCE_INLINE size_t GetFreeMemory() const { return totalMemory - GetUsedMemory(); }
    NO_DISCARD FORCE_INLINE size_t GetTotalMemory() const { return totalMemory; }
};


