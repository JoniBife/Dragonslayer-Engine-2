#include "Math/MathAux.hpp"

#include <cmath>
#include <cstdlib>

#define EPSILON 0.00005f

bool Cmpf(float A, float B)
{
    return fabsf(A - B) < EPSILON;
}

float Round6(float number) {
    return std::round(number * std::pow(10, 6)) / std::pow(10, 6);
}

float RandomFloat() {
    float a = 5.0;
    return (static_cast<float>(rand()) / static_cast<float>((RAND_MAX))) * a;
}

float RandomFloat(float min, float max) {

    return static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min))) + min;
}

float RandomBinomial()
{
    return RandomFloat(0.f, 1.f) - RandomFloat(0.f, 1.f);
}

float DegreesToRadians(float angle) {
    return (angle * PI) / 180.0f;
}

float RadiansToDegrees(float angleRad) {
    return (angleRad * 180.0f) / PI;
}

float Lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

float Clerp(float a, float b, float t)
{
    const float t2 = (1.f - std::cos(t * PI)) / 2.f;
    return Lerp(a, b , t2);
}

float SmoothstepLerp(float a, float b, float t)
{
    const float t2 = t * t * (3.f - 2.f * t);
    return Lerp(a, b , t2);
}
