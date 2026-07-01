#include "Math/Vec4.hpp"

#include <cmath>

#include "Core/Assert.hpp"
#include "Math/MathAux.hpp"

Vec4::Vec4() : Vec4(0.f) {}
Vec4::Vec4(float xyzw) : Vec4(xyzw,xyzw,xyzw,xyzw) {}
Vec4::Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) { }

Vec4 Vec4::ONE = Vec4(1.f);
Vec4 Vec4::ZERO = Vec4(0.f);
Vec4 Vec4::Z = Vec4(0.f, 0.f, 1.f, 1.f);
Vec4 Vec4::Y = Vec4(0.f, 1.f, 0.f, 1.f);
Vec4 Vec4::X = Vec4(1.f, 0.f, 0.f, 1.f);

Vec4 Vec4::operator+(const Vec4& other) const {
	return {this->x + other.x, this->y + other.y, this->z + other.z, this->w + other.w};
}

Vec4 Vec4::operator-(const Vec4& other) const {
	return {this->x - other.x, this->y - other.y, this->z - other.z, this->w - other.w};
}

Vec4& Vec4::operator=(const Vec4& other) {
	this->x = other.x;
	this->y = other.y;
	this->z = other.z;
	this->w = other.w;
	return *this;
}

Vec4& Vec4::operator+=(const Vec4& other) {
	this->x += other.x;
	this->y += other.y;
	this->z += other.z;
	this->w += other.w;
	return *this;
}

Vec4& Vec4::operator-=(const Vec4& other) {
	this->x -= other.x;
	this->y -= other.y;
	this->z -= other.z;
	this->w -= other.w;
	return *this;
}



Vec4 Vec4::operator*(float scalar) const {
	return {this->x * scalar, this->y * scalar, this->z * scalar, this->w * scalar};
}

Vec4 Vec4::operator/(float scalar) const {
	return {this->x / scalar, this->y / scalar, this->z / scalar, this->w / scalar};
}

Vec4 Vec4::operator+(float scalar) const {
	return {this->x + scalar, this->y + scalar, this->z + scalar, this->w + scalar};
}

Vec4 Vec4::operator-(float scalar) const {
	return {this->x - scalar, this->y - scalar, this->z - scalar, this->w - scalar};
}

Vec4& Vec4::operator+=(float s) {
	this->x += s;
	this->y += s;
	this->z += s;
	this->w += s;
	return *this;
}

Vec4& Vec4::operator-=(float s) {
	this->x -= s;
	this->y -= s;
	this->z -= s;
	this->w -= s;
	return *this;
}

Vec4& Vec4::operator*=(float s) {
	this->x *= s;
	this->y *= s;
	this->z *= s;
	this->w *= s;
	return *this;
}

Vec4& Vec4::operator/=(float s) {
	this->x /= s;
	this->y /= s;
	this->z /= s;
	this->w /= s;
	return *this;
}



bool Vec4::operator==(const Vec4& other) const {
	return Cmpf(this->x, other.x) && Cmpf(this->y, other.y) && Cmpf(this->z, other.z) && Cmpf(this->w, other.w);
}

bool Vec4::operator!=(const Vec4& other) const {
	return !(*this == other);
}



float Vec4::Magnitude() const {
	return std::sqrt(this->x * this->x + this->y * this->y + this->z * this->z + this->w*this->w);
}

float Vec4::SqrMagnitude() const {
	return this->x * this->x + this->y * this->y + this->z * this->z + this->w* this->w;
}

Vec4 Vec4::Normalize() const {
	float magnitude = this->Magnitude();

	// Cannot divide by 0
	ASSERT(magnitude > 0);

	return (*this) / magnitude;
}

void Vec4::ToOpenGLFormat(float array[4]) const {
	array[0] = this->x;
	array[1] = this->y;
	array[2] = this->z;
	array[3] = this->w;
}

float Dot(const Vec4& a, const Vec4& b) {
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

Vec4 operator*(float scalar, const Vec4& vec4) {
	return {scalar * vec4.x, scalar * vec4.y, scalar * vec4.z, scalar * vec4.w};
}

Vec4 operator+(float scalar, const Vec4& vec4) {
	return {scalar + vec4.x, scalar + vec4.y, scalar + vec4.z, scalar + vec4.w};
}

Vec4 operator-(float scalar, const Vec4& vec4) {
	return {scalar - vec4.x, scalar - vec4.y, scalar - vec4.z, scalar - vec4.w};
}

Vec4 Vec4::operator-() const {
    return { -x, -y, -z, -w};
}
