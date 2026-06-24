#pragma once

#include <ECS/Component.hpp>

#include "Game/StaticGameSettings.hpp"

DSTRUCT(RotatorModule, .numComponents = MAX_MODULES, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DSTRUCT_BODY(RotatorModule)

    Vec3 offsetLocation = Vec3::ZERO;
    Quat offsetRotation = Quat::IDENTITY;

    // Configuration
    float maxAngularSpeed = 20.f;  // rad/s (max rotation speed)
    float angularAcceleration = 50.f;  // rad/s^2 (how fast we ramp up)
    float angularDamping = 50.0f;        // rad/s^2 (decel when no input)

    // State
    float rotation = 0.0f;  // current angle (radians)
    float angularVelocity = 0.0f;  // current angular speed (rad/s)

    void UpdateRotation(float inputDir, float deltaTime) {
        // Clamp input to valid range
        inputDir = std::clamp(inputDir, -1.0f, 1.0f);

        if (std::fabs(inputDir) > 0.0001f)
        {
            // Player is providing input -> accelerate toward target velocity
            float targetVelocity = inputDir * maxAngularSpeed;
            float delta          = targetVelocity - angularVelocity;
            float step           = angularAcceleration * deltaTime;

            // Move angularVelocity toward targetVelocity by at most `step`
            if (std::fabs(delta) <= step)
                angularVelocity = targetVelocity;
            else
                angularVelocity += (delta > 0.0f ? step : -step);
        }
        else
        {
            // No input -> damp toward zero
            float step = angularDamping * deltaTime;
            if (std::fabs(angularVelocity) <= step)
                angularVelocity = 0.0f;
            else
                angularVelocity -= (angularVelocity > 0.0f ? step : -step);
        }

        // Safety clamp (in case maxAngularSpeed was lowered at runtime)
        angularVelocity = std::clamp(angularVelocity,
                                     -maxAngularSpeed,
                                     maxAngularSpeed);

        // Integrate velocity into rotation (semi-implicit Euler)
        rotation += angularVelocity * deltaTime;

        // Optional: wrap to [-PI, PI] to avoid float drift over long sessions
        constexpr float TWO_PI = 6.2831853f;
        if (rotation >  3.1415927f) rotation -= TWO_PI;
        if (rotation < -3.1415927f) rotation += TWO_PI;
    }

#if WITH_EDITOR
    void OnHierarchy() {
        //ImGui::InputFloat("Max Angular Acceleration (d/s^2)", &maxAngularAcceleration);
//
        //ImGui::BeginDisabled();
        //ImGui::InputFloat("Current Rotation", &currentRotation);
        //ImGui::EndDisabled();
    }
#endif

}; END_DSTRUCT(RotatorModule)
