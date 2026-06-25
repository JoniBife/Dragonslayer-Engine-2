#pragma once

// TODO Move out of editor!

#include <Core/ThreadContext.hpp>

#include "Core/EngineGlobals.hpp"
#include "Core/NotCopyable.hpp"
#include "Core/String.hpp"
#include "Core/Containers/Array.hpp"
#include "Core/Allocators/FreeListAllocator.hpp"

struct TimeCapture {
    NameString name;
    float duration;
    uint32 threadIndex;

    bool operator<(const TimeCapture& other) const {
        if (duration < other.duration) {
            return true;
        }
        return false;
    }

    bool operator>(const TimeCapture& other) const {
        if (duration > other.duration) {
            return true;
        }
        return false;
    }
};

/*
 * Responsible for storing timing captures
 * Each thread gets a separate list for its captures
 */
class ENGINE_API TimeCapturesManager final : public NotCopyable {

private:
    Array<Array<TimeCapture, FreeListAllocator>, FreeListAllocator> capturesPerThread;
    float smoothingFactor = .85f;

    static TimeCapturesManager* instance;

    friend class FreeListAllocator; // So the allocator can use this private constructor
    TimeCapturesManager();

public:
    static void Initialize();

    static TimeCapturesManager& Instance();

    void AddCapture(const NameString& name, float duration);

    void SetSmoothingFactor(float newSmoothingFactor);

    NO_DISCARD float GetSmoothingFactor() const;

    FORCE_INLINE void SortThreadCaptures() {
        capturesPerThread[GetThreadContext().index].Sort([](const TimeCapture& a, const TimeCapture& b) {
            return a > b;
        });
    }

    void ClearThreadCaptures();

    NO_DISCARD FORCE_INLINE const Array<Array<TimeCapture, FreeListAllocator>, FreeListAllocator>& GetCapturesPerThread() const {
        return capturesPerThread;
    }
};