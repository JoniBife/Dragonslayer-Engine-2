#include "Core/Allocators/LinearAllocator.hpp"

#include "Core/Memory.hpp"

LinearAllocator::LinearAllocator(size_t size, size_t alignment) : totalMemory(size) {

    // USE 128 BYTE ALIGNMENT FOR GLOBAL ALLOCATOR TO POTENTIALLY SUPPORT AVX512 SIMD

    ASSERT(size > 0 && alignment > 0);

    baseAddress = static_cast<uint8*>(AlignedMalloc(size, alignment));

    ASSERT(baseAddress != nullptr);

    topAddress = baseAddress;
}

uint8* LinearAllocator::Allocate(size_t size, size_t alignment) {

    ASSERT(size > 0 && alignment > 0);
    ASSERT(IsPowerOf2(alignment));
    //ASSERT(size % alignment == 0); Sadly cannot have this because GLFW does not follow this rule!

    uint8* targetAddress = reinterpret_cast<uint8*>(AlignUp(reinterpret_cast<uint64>(topAddress), alignment));
    const size_t newUsedMemory = (targetAddress - baseAddress) + size;

    if (newUsedMemory > totalMemory) {
        FAIL("Allocation request would exceed max capacity.");
        return nullptr;
    }

    topAddress = targetAddress + size;

    return targetAddress;
}

void LinearAllocator::Free() {
    if (parentAllocator.Exists()) {
        parentAllocator.Free(baseAddress);
    } else {
        AlignedFree(baseAddress);
    }
}
