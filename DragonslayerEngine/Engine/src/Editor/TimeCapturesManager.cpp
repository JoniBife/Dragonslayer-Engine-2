#include "Editor/TimeCapturesManager.hpp"

#include <Core/ThreadContext.hpp>

#include "Core/Log.hpp"

TimeCapturesManager* TimeCapturesManager::instance = nullptr;

TimeCapturesManager::TimeCapturesManager() : capturesPerThread(gNumThreads, gGlobalAllocator)
{
    capturesPerThread.EmplaceMany(gNumThreads, 64, gGlobalAllocator);
}

void TimeCapturesManager::Initialize() {
    ASSERT(GetThreadContext().IsMainThread()); // TODO For now only main thread can set this!
    ASSERT(instance == nullptr);
    instance = gGlobalAllocator->Allocate<TimeCapturesManager>();
}

TimeCapturesManager& TimeCapturesManager::Instance() {
    ASSERT(instance != nullptr);
    return *instance;
}

void TimeCapturesManager::AddCapture(const NameString& name, float duration) {

    if (duration > 100.f) {
        Log::Warning("[PERF][SPIKE] " + LogString(name) + " took " + LogString(duration) + " ms!");
    }

    const uint32 threadIndex = GetThreadContext().index;

    Array<TimeCapture, FreeListAllocator>& captures = capturesPerThread[threadIndex];

    bool foundCapture = false;
    for (TimeCapture& capture : captures) {
        if (capture.name == name && capture.threadIndex == threadIndex) {
            // Exponential moving average
            // Factor is inverted (when compared to reference) because its more intuitive this way given its name
            capture.duration = (1.f - smoothingFactor)*duration + smoothingFactor*capture.duration;
            foundCapture = true;
            break;
        }
    }

    if (!foundCapture) {
        captures.Emplace(name, duration, threadIndex);
    }
}

void TimeCapturesManager::ClearThreadCaptures() {
    capturesPerThread[GetThreadContext().index].Reset();
}

void TimeCapturesManager::SetSmoothingFactor(float newSmoothingFactor) {
    ASSERT(GetThreadContext().IsMainThread()); // TODO For now only main thread can set this!
    smoothingFactor = std::clamp(newSmoothingFactor, 0.f, 1.f);
}

float TimeCapturesManager::GetSmoothingFactor() const {
    ASSERT(GetThreadContext().IsMainThread()); // TODO For now only main thread can set this!
    return smoothingFactor;
}
