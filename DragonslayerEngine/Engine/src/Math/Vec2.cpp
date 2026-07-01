#include "Math/Vec2.hpp"

#include <algorithm>
#include <cmath>

#include "Core/Assert.hpp"
#include "Math/MathAux.hpp"
#include "Math/Vec4.hpp"
#include "Math/Vec3.hpp"

Vec2::Vec2() : Vec2(0.f) { }
Vec2::Vec2(float xy) : Vec2(xy,xy) { }
Vec2::Vec2(float x, float y) : x(x), y(y) { }

Vec2 Vec2::Y = Vec2(0.f, 1.f);
Vec2 Vec2::X = Vec2(1.f, 0.f);
Vec2 Vec2::ONE = Vec2(1.f, 1.f);

Vec2 Vec2::operator+(const Vec2& other) const {
	return {this->x + other.x, this->y + other.y};
}

Vec2 Vec2::operator-(const Vec2& other) const {
	return {this->x - other.x, this->y - other.y};
}

Vec2& Vec2::operator=(const Vec2& other) {
	this->x = other.x;
	this->y = other.y;
	return *this;
}

Vec2& Vec2::operator+=(const Vec2& other) {
	this->x += other.x;
	this->y += other.y;
	return *this;
}

Vec2& Vec2::operator-=(const Vec2& other) {
	this->x -= other.x;
	this->y -= other.y;
	return *this;
}


Vec2 Vec2::operator*(float scalar) const {
	return {this->x * scalar, this->y * scalar};
}

Vec2 Vec2::operator/(float scalar) const {
	return {this->x / scalar, this->y / scalar};
}

Vec2 Vec2::operator+(float scalar) const {
	return {this->x + scalar, this->y + scalar};
}

Vec2 Vec2::operator-(float scalar) const {
	return {this->x - scalar, this->y - scalar};
}

Vec2& Vec2::operator+=(float s) {
	this->x += s;
	this->y += s;
	return *this;
}

Vec2& Vec2::operator-=(float s) {
	this->x -= s;
	this->y -= s;
	return *this;
}

Vec2& Vec2::operator*=(float s) {
	this->x *= s;
	this->y *= s;
	return *this;
}

Vec2& Vec2::operator/=(float s) {
	this->x /= s;
	this->y /= s;
	return *this;
}

bool Vec2::operator==(const Vec2& other) const {
	return Cmpf(this->x,other.x) && Cmpf(this->y, other.y);
}

bool Vec2::operator!=(const Vec2& other) const {
	return !(*this == other);
}

float Vec2::Magnitude() const {
	return std::sqrt(this->x * this->x + this->y * this->y);
}

float Vec2::SqrMagnitude() const {
	return this->x * this->x + this->y * this->y;
}

void Vec2::ToOpenGLFormat(float array[2]) const {
	array[0] = this->x;
	array[1] = this->y;
}

Vec2 Vec2::Normalize() const {
	float magnitude = this->Magnitude();

	// Cannot divide by 0
	ASSERT(magnitude > 0);

	return (*this) / magnitude;
}

Vec2 Vec2::Clamp(float min, float max) const {
	return { std::clamp(x, min, max), std::clamp(y, min, max) };
}

Vec3 Vec2::ToVec3() const {
	return {this->x, this->y, 0};
}

Vec4 Vec2::ToVec4() const {
	return {this->x, this->y, 0, 0};
}

float Vec2::ToOrientation() const {
	return 0.0f;
}


Vec2 Vec2::FromOrientation(float angleRad) {
	return {0.0f, 0.0f};
}


float Dot(const Vec2& a, const Vec2& b) {
	return a.x * b.x + a.y * b.y;
}

Vec2 operator*(float scalar, const Vec2& vec2) {
	return {scalar * vec2.x, scalar * vec2.y};
}

Vec2 operator+(float scalar, const Vec2& vec2) {
	return {scalar + vec2.x, scalar + vec2.y};
}

Vec2 operator-(float scalar, const Vec2& vec2) {
	return {scalar - vec2.x, scalar - vec2.y};
}

Vec2 Vec2::operator-() const {
    return {-x,-y };
}
