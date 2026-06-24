#include "Runtime/Systems/TimeSystem.hpp"

#include <algorithm> // TODO For clamp, replace with my own implementation
#include <Core/ThreadContext.hpp>

#include "Core/EngineGlobals.hpp"
#include "Core/Time.hpp"

using namespace std::chrono;

void TimeSystem::Start(ThreadContext &threadContext, Vault &vault) {

    if (!threadContext.IsMainThread()) {
        return;
    }

    gTime = Time();
    gTime.lastFrameTime = high_resolution_clock::now();
}

void TimeSystem::Update(ThreadContext &threadContext, Vault &vault) {

    if (!threadContext.IsMainThread()) {
        return;
    }

    Time time = gTime;
    const high_resolution_clock::time_point currentFrameTime = high_resolution_clock::now();
    const microseconds delta = duration_cast<microseconds>(currentFrameTime - time.lastFrameTime);
    time.lastFrameTime = currentFrameTime;

    const float clockDeltaTime = static_cast<float>(delta.count()) * 1.0e-6f; // Converting from microseconds to seconds

    if (time.paused) {
        time.deltaTime = 0.f;
        time.physicsDeltaTime = 0.f;
        time.unscaledDeltaTime = clockDeltaTime;
    } else {
        // Calculate how much time has passed since the last render
        float worldDeltaTime = clockDeltaTime + time.residualDeltaTime;

        // TODO Decide if clamp stays, it will extrapolate which might be good!
        time.physicsUpdateInterpolationFactor = std::clamp(worldDeltaTime / (time.physicsUpdateFrequency), 0.f, 1.f);

        if (worldDeltaTime < time.physicsUpdateFrequency) {
            // Too soon, set the residual time and don't update
            time.residualDeltaTime = worldDeltaTime;
            worldDeltaTime = 0.0f;
        } else {
            time.lastFrameExtraPhysicsTime = std::max(0.f, time.residualDeltaTime - time.physicsDeltaTime);

            // Update and clamp the residual time to a full update to avoid spiral of death
            time.residualDeltaTime = std::min(time.physicsUpdateFrequency,
                                              worldDeltaTime - time.physicsUpdateFrequency);
            worldDeltaTime = time.physicsUpdateFrequency;
        }

        time.deltaTime = clockDeltaTime;
        time.physicsDeltaTime = worldDeltaTime;
        time.unscaledDeltaTime = clockDeltaTime;
    }

    time.totalTime += clockDeltaTime;
    gTime = time;
}
