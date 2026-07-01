#pragma once

#include "Core/Export.hpp"
#include "Core/Macros.hpp"
#include "MathFwd.hpp"

struct ENGINE_API Vec3 {

	float x, y, z;

	Vec3();
	explicit Vec3(float xyz);
	Vec3(float x, float y, float z);

	static Vec3 ONE;
	static Vec3 ZERO;

	static Vec3 Z;
	static Vec3 Y;
	static Vec3 X;

	static Vec3 FORWARD;
	static Vec3 RIGHT;
	static Vec3 UP;

	static Vec3 Random();

	Vec3 operator+(const Vec3& other) const;
	Vec3 operator-(const Vec3& other) const;
	Vec3& operator=(const Vec3& other);
	Vec3& operator+=(const Vec3& other);
	Vec3& operator-=(const Vec3& other);
	Vec3& operator*=(const Vec3& other);
	Vec3& operator/=(const Vec3& other);

	Vec3 operator*(float scalar) const;
	friend ENGINE_API Vec3 operator*(float scalar, const Vec3& vec3);
	Vec3 operator/(float scalar) const;
	Vec3 operator+(float scalar) const;
	friend ENGINE_API Vec3 operator+(float scalar, const Vec3& vec3);
	Vec3 operator-(float scalar) const;
	friend ENGINE_API Vec3 operator-(float scalar, const Vec3& vec3);
    Vec3 operator-() const;

	Vec3& operator+=(float s);
	Vec3& operator-=(float s);
	Vec3& operator*=(float s);
	Vec3& operator/=(float s);

	bool operator==(const Vec3& other) const;
	bool operator!=(const Vec3& other) const;

	NO_DISCARD static float Angle(const Vec3& a, const Vec3& b);
	NO_DISCARD static float AngleRad(const Vec3& a, const Vec3& b);

	NO_DISCARD float Magnitude() const;
	NO_DISCARD float MagnitudeSquared() const;

	void ToOpenGLFormat(float array[3]) const;

	NO_DISCARD static Vec3 Lerp(Vec3 initial, Vec3 final, float f);

	NO_DISCARD Vec3 Normalize() const;
    NO_DISCARD Vec3 NormalizedPerpendicular() const;
	NO_DISCARD Vec4 ToVec4() const;
	NO_DISCARD Vec3 ClampMagnitude(float newMagnitude) const;
	NO_DISCARD Vec3 Abs() const;
};

ENGINE_API float Dot(const Vec3& a, const Vec3& b);
ENGINE_API Vec3 Cross(const Vec3& a, const Vec3& b);
ENGINE_API Vec3 Rodrigues(Vec3 v, float theta, Vec3 k);