#pragma once

#include <iostream>

#include "Core/Export.hpp"
#include "Core/Macros.hpp"

struct ENGINE_API Vec4 {

	float x, y, z, w;

	Vec4();
	explicit Vec4(float xyzw);
	Vec4(float x, float y, float z, float w);

	static Vec4 ONE;
	static Vec4 ZERO;
	static Vec4 Z;
	static Vec4 Y;
	static Vec4 X;

	Vec4 operator+(const Vec4& other) const;
	Vec4 operator-(const Vec4& other) const;
	Vec4& operator=(const Vec4& other);
	Vec4& operator+=(const Vec4& other);
	Vec4& operator-=(const Vec4& other);

	Vec4 operator*(float scalar) const;
	friend ENGINE_API Vec4 operator*(float scalar, const Vec4& vec4);
	Vec4 operator/(float scalar) const;
	Vec4 operator+(float scalar) const;
	friend ENGINE_API Vec4 operator+(float scalar, const Vec4& vec4);
	Vec4 operator-(float scalar) const;
	friend ENGINE_API Vec4 operator-(float scalar, const Vec4& vec4);
    Vec4 operator-() const;

	Vec4& operator+=(float s);
	Vec4& operator-=(float s);
	Vec4& operator*=(float s);
	Vec4& operator/=(float s);

	bool operator==(const Vec4& other) const;
	bool operator!=(const Vec4& other) const;

	NO_DISCARD float Magnitude() const;
	NO_DISCARD float SqrMagnitude() const;
	NO_DISCARD Vec4 Normalize() const;

	void ToOpenGLFormat(float array[4]) const;
};

ENGINE_API float Dot(const Vec4& a, const Vec4& b);
ENGINE_API std::ostream& operator<<(std::ostream& os, const Vec4& vec4);