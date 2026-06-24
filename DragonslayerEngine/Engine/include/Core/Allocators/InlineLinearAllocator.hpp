#pragma once

#include <cstddef>

#include "Core/Assert.hpp"
#include "Core/Memory.hpp"
#include "Core/NotCopyable.hpp"
#include "Core/Types.hpp"

/*
 * Inline Linear allocator
 * - Memory allocated 100% on the stack
 * - Can be used in containers
 */
template<uint32 TotalMemoryBytes, size_t Alignment = alignof(std::max_align_t)>
class InlineLinearAllocator : public NotCopyable {

private:
    alignas(Alignment) uint8 baseAddress[TotalMemoryBytes];
    uint8* topAddress;

public:
    InlineLinearAllocator() : baseAddress{}, topAddress(baseAddress) {}

    NO_DISCARD uint8* Allocate(size_t size, size_t alignment) {

        ASSERT(size > 0 && alignment > 0);
        ASSERT(IsPowerOf2(alignment));
        ASSERT(size % alignment == 0);

        uint8* targetAddress = reinterpret_cast<uint8*>(AlignUp(reinterpret_cast<uint64>(topAddress), alignment));

        const size_t newUsedMemory = (targetAddress - baseAddress) + size;

        ASSERT(newUsedMemory <= TotalMemoryBytes);

        if (newUsedMemory > TotalMemoryBytes) {
            FAIL("Allocation request would exceed max capacity.");
            return nullptr;
        }

        topAddress = targetAddress + size;

        return targetAddress;
    }

    template<typename Type, typename... Args>
    NO_DISCARD Type* Allocate(Args&&... args) {
        Type* objectMemory = reinterpret_cast<Type*>(Allocate(sizeof(Type), alignof(Type)));
        new (objectMemory) Type(std::forward<Args>(args)...);
        return objectMemory;
    }

    NO_DISCARD bool TryExtend(uint8* blockAddress, size_t newSize) { return false; }

    // Does absolutely nothing, here purely for use in containers!
    void Free(uint8*) {
        // TODO We could validate the address perhaps?
    }

    template<typename Type>
    void Free(Type* address) {
        if (address) {
            address->~Type();
            // Pointless! -> Free(reinterpret_cast<uint8*>(address));
        }
    }

    void Clear() { topAddress = baseAddress; }

    NO_DISCARD FORCE_INLINE size_t GetUsedMemory() const { return topAddress - baseAddress; }
    NO_DISCARD FORCE_INLINE size_t GetFreeMemory() const { return TotalMemoryBytes - GetUsedMemory(); }
    NO_DISCARD FORCE_INLINE size_t GetTotalMemory() const { return TotalMemoryBytes; }
};