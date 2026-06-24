#pragma once

#include <ostream>

#include "Core/Export.hpp"
#include "Math/Vec3.hpp"
#include "Math/Vec4.hpp"

struct ENGINE_API Vec2 {

	float x, y;

	Vec2();
    explicit Vec2(float xy);
	Vec2(float x, float y);

	static Vec2 Y;
	static Vec2 X;

	static Vec2 ONE;

	Vec2 operator+(const Vec2& other) const;
	Vec2 operator-(const Vec2& other) const;
	Vec2& operator=(const Vec2& other);
	Vec2& operator+=(const Vec2& other);
	Vec2& operator-=(const Vec2& other);

	Vec2 operator*(float scalar) const;
	friend ENGINE_API Vec2 operator*(float scalar, const Vec2& vec2);
	Vec2 operator/(float scalar) const;
	Vec2 operator+(float scalar) const;
	friend ENGINE_API Vec2 operator+(float scalar, const Vec2& vec2);
	Vec2 operator-(float scalar) const;
	friend ENGINE_API Vec2 operator-(float scalar, const Vec2& vec2);
    Vec2 operator-() const;

	Vec2& operator+=(float s);
	Vec2& operator-=(float s);
	Vec2& operator*=(float s);
	Vec2& operator/=(float s);

	// Should compare the vector components (x,y)
	bool operator==(const Vec2& other) const;
	bool operator!=(const Vec2& other) const;

	NO_DISCARD float Magnitude() const;
	NO_DISCARD float SqrMagnitude() const;
	NO_DISCARD Vec2 Normalize() const;
	NO_DISCARD Vec2 Clamp(float min, float max) const;

	void ToOpenGLFormat(float array[2]) const;

	NO_DISCARD Vec3 ToVec3() const;
	NO_DISCARD Vec4 ToVec4() const;

	NO_DISCARD float ToOrientation() const;
	NO_DISCARD static Vec2 FromOrientation(float angleRad);
};

ENGINE_API float Dot(const Vec2& a, const Vec2& b);
ENGINE_API std::ostream& operator<<(std::ostream& os, const Vec2& vec2);
