#pragma once

#include <cstddef>

#include "Core/Assert.hpp"
#include "Core/Export.hpp"
#include "Core/Macros.hpp"
#include "Core/NotCopyable.hpp"
#include "Core/Types.hpp"
#include "ParentAllocator.hpp"

/*
 * Stack allocator
 * - Memory DOES not get freed on destruction (Free needs to be called explicitly)
 * - Free and TryExtend only act on the top (most recent) allocation
 */
class ENGINE_API StackAllocator final : public NotCopyable {

private:
    ParentAllocator parentAllocator;

    struct Header {
        uint8* userAddress;       // address returned to the user for this frame
        uint8* previousTop;       // topAddress before this allocation (rewind point)
        Header* previousHeader;   // previous Header pointer, or nullptr
    };

    uint8* baseAddress;
    uint8* topAddress; // The address at the end of the last allocation
    Header* topHeader; // Header of the top frame, or nullptr if empty
    size_t totalMemory;

public:
    explicit StackAllocator(size_t size, size_t alignment = alignof(std::max_align_t));

    template<typename ParentAllocator>
    StackAllocator(ParentAllocator& parentAllocator, size_t size, size_t alignment = alignof(std::max_align_t)) :
        parentAllocator(parentAllocator),
        baseAddress(parentAllocator.Allocate(size, alignment)),
        topAddress(baseAddress),
        topHeader(nullptr),
        totalMemory(size) {
        ASSERT(size > 0);
    }

    // TODO Decide if we add a destructor
    //~StackAllocator() = delete;

    NO_DISCARD uint8* Allocate(size_t size, size_t alignment = alignof(std::max_align_t));

    NO_DISCARD bool TryExtend(uint8* baseAddress, size_t newSize);

    void Free();

    void Free(uint8*);

    void Clear();

    NO_DISCARD FORCE_INLINE size_t GetUsedMemory() const { return topAddress - baseAddress; }
    NO_DISCARD FORCE_INLINE size_t GetFreeMemory() const { return totalMemory - GetUsedMemory(); }
    NO_DISCARD FORCE_INLINE size_t GetTotalMemory() const { return totalMemory; }
};


