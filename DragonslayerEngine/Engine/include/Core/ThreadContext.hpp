#pragma once

#include <barrier>

#include "Containers/Array.hpp"
#include "Core/Allocators/FreeListAllocator.hpp"
#include "Core/Allocators/StackAllocator.hpp"

#if WITH_EDITOR
#include <source_location>
#include "Core/Assert.hpp"
#endif

// TODO Replace with custom type, we don't need a completion function
using Barrier = std::barrier<>;

struct Range {
    uint32 min; /*inclusive*/
    uint32 max; /*exclusive*/
};

struct SyncSite {
    const char* file;
    uint32 line;
};

/**
 * Represents what is available per-thread within the context of a thread group
 * TODO The debug syncSites should be behind a macro that is not WITH_EDITOR
 */
struct ThreadContext {
    FreeListAllocator& allocator;
    StackAllocator& tempAllocator;
    Barrier& barrier;
#if WITH_EDITOR
    Array<SyncSite, FreeListAllocator>& syncSites;
#endif
    void* broadcastMemory; // A pointer to broadcast data from on to many
    const uint32 count; // Total number of threads running in parallel with this one
    const uint32 index;

#if WITH_EDITOR
    // Every thread stores its call site, then after the barrier asserts that the
    // whole group arrived at the same site. Catches a thread taking a divergent number of Sync() calls
    // (e.g. an early return before a Sync)
    FORCE_INLINE void Sync(const std::source_location& location = std::source_location::current()) {
        // A thread-group of 1 does not actually need to wait
        if (count == 1) {
            return;
        }

        syncSites[index] = { location.file_name(), location.line() };
        barrier.arrive_and_wait();

        for (uint32 i = 0; i < count; ++i) {
            ASSERT(
                syncSites[i].line == syncSites[index].line &&
                std::strcmp(syncSites[i].file, syncSites[index].file) == 0,
                "Thread Desync detected! Thread %u at %s:%u, thread %u at %s:%u",
                index, syncSites[index].file, syncSites[index].line,
                i, syncSites[i].file, syncSites[i].line);
        }

        barrier.arrive_and_wait();
    }
#else
    FORCE_INLINE void Sync() {
        // A thread-group of 1 does not actually need to wait
        if (count == 1) {
            return;
        }
        barrier.arrive_and_wait();
    }
#endif

    NO_DISCARD FORCE_INLINE bool IsMainThread() const {
        return index == 0;
    }

    // Uniformly distributes the total for each thread
    NO_DISCARD Range UniformRange(uint32 total) const {

        const uint32 rangeSize = total / count;
        const uint32 remainder = total % count;

        const bool hasRemainder = index < remainder;

        uint32 remainderFromPreviousThread = remainder;
        if (hasRemainder) {
            remainderFromPreviousThread = index;
        }

        const uint32 rangeMin = rangeSize * index + remainderFromPreviousThread;
        uint32 rangeMax = rangeMin + rangeSize;

        if (hasRemainder) {
            rangeMax += 1;
        }

        return { rangeMin, rangeMax };
    }

};