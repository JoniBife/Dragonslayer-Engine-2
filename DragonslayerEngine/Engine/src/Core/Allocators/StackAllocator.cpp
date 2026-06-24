#include "Core/Allocators/StackAllocator.hpp"

#include <cstring>

#include "Core/Assert.hpp"
#include "Core/Memory.hpp"

StackAllocator::StackAllocator(size_t size, size_t alignment) : totalMemory(size) {

    ASSERT(size > 0 && alignment > 0);

    baseAddress = static_cast<uint8*>(AlignedMalloc(size, alignment));

    ASSERT(baseAddress != nullptr);

    topAddress = baseAddress;
    topHeader = nullptr;
}

uint8* StackAllocator::Allocate(size_t size, size_t alignment) {

    ASSERT(size > 0 && alignment > 0);
    ASSERT(IsPowerOf2(alignment));

    uint8* headerPos = reinterpret_cast<uint8*>(AlignUp(reinterpret_cast<uint64>(topAddress), alignof(Header)));
    uint8* afterHeader = headerPos + sizeof(Header);
    uint8* userAddress = reinterpret_cast<uint8*>(AlignUp(reinterpret_cast<uint64>(afterHeader), alignment));
    uint8* newTop = userAddress + size;

    const size_t newUsedMemory = (userAddress - baseAddress) + size;

    if (newUsedMemory > totalMemory) {
        FAIL("Allocation request would exceed max capacity.");
        return nullptr;
    }

    Header* header = reinterpret_cast<Header*>(headerPos);
    header->userAddress = userAddress;
    header->previousTop = topAddress;
    header->previousHeader = topHeader;

    topHeader = header;
    topAddress = newTop;

    return userAddress;
}

bool StackAllocator::TryExtend(uint8* blockAddress, size_t newSize) {

    if (topHeader == nullptr) {
        return false;
    }

    if (topHeader->userAddress != blockAddress) {
        return false;
    }

    const size_t oldSize = static_cast<size_t>(topAddress - blockAddress);

    if (newSize < oldSize) {
        FAIL("New allocation size is smaller than original!");
        return false;
    }

    if (newSize == oldSize) {
        return true; // No extension needed
    }

    const size_t newUsedMemory = (topHeader->userAddress - baseAddress) + newSize;

    if (newUsedMemory > totalMemory) {
        FAIL("Allocation request would exceed max capacity.");
        return false;
    }

    topAddress = topHeader->userAddress + newSize;

    return true;
}

void StackAllocator::Free() {
    if (parentAllocator.Exists()) {
        parentAllocator.Free(baseAddress);
    } else {
        AlignedFree(baseAddress);
    }
}

void StackAllocator::Free(uint8* blockAddress) {

    if (topHeader == nullptr) {
        return;
    }

    if (topHeader->userAddress != blockAddress) {
        // Per spec: Free only operates on the top allocation
        return;
    }

    topAddress = topHeader->previousTop;
    topHeader = topHeader->previousHeader;
}

void StackAllocator::Clear() {
    topAddress = baseAddress;
    topHeader = nullptr;
}
