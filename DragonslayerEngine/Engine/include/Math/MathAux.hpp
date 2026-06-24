#pragma once

#include "Core/Export.hpp"

#define PI 3.14159265358979323846f

#define SQR(x) ((x) * (x))

/*
*
* Checks if the floats A and B are equal
* using FLT_EPISILON as error margin
*
* @param A first float
* @param B second float
*
* @return true if A equals B else false
*/
ENGINE_API bool Cmpf(float A, float B);

ENGINE_API float Round6(float number);

ENGINE_API float RandomFloat();

ENGINE_API float RandomFloat(float min, float max);

ENGINE_API float RandomBinomial();

ENGINE_API float DegreesToRadians(float angle);

ENGINE_API float RadiansToDegrees(float angleRad);

ENGINE_API float Lerp(float a, float b, float t);

/* cosine interpolation */
ENGINE_API float Clerp(float a, float b, float t);

ENGINE_API float SmoothstepLerp(float a, float b, float t);