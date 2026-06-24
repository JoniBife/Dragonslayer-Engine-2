#pragma once

#include <Core/Export.hpp>
#include <chrono>

struct ENGINE_API Time /* Seconds */ {

    // Time passed since last frame (affected by pause)
    float deltaTime = 0.f;

    // Time passed since last frame (not affected by pause)
    float unscaledDeltaTime = 0.f;

    // Time since the start of the game
    float totalTime = 0.f;

    // Last frame's high_resolution_clock::now()
    std::chrono::high_resolution_clock::time_point lastFrameTime;

    // Time that passed last frame but still not enough to trigger a physics update 
    float residualDeltaTime = 0.f;

    // Physics delta time (If 0.f then no physics update happened this frame, affected by pause)
    float physicsDeltaTime = 0.f;

    // Physics fixed update frequency
    float physicsUpdateFrequency = 1.f / 60.f; // 60hz

    float lastFrameExtraPhysicsTime = 0.f;

    // Physics Update Interpolation Factor
    float physicsUpdateInterpolationFactor = 1.f;

    // Weather time is paused
    bool paused = false;
};