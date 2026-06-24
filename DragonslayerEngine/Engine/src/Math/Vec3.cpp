
#include <cmath>

#include "Math/Vec3.hpp"
#include "Math/MathAux.hpp"

Vec3::Vec3() : Vec3(0.f) {}
Vec3::Vec3(float xyz) : Vec3(xyz, xyz, xyz) {}
Vec3::Vec3(float x, float y, float z) : x(x), y(y), z(z) { }

Vec3 Vec3::ONE = Vec3(1.f);
Vec3 Vec3::ZERO = Vec3(0.f);
Vec3 Vec3::Z = Vec3(0.f, 0.f, 1.f);
Vec3 Vec3::Y = Vec3(0.f, 1.f, 0.f);
Vec3 Vec3::X = Vec3(1.f, 0.f, 0.f);
Vec3 Vec3::FORWARD = Vec3(0.f, 0.f, 1.f);
Vec3 Vec3::RIGHT = Vec3(1.f, 0.f, 0.f);
Vec3 Vec3::UP = Vec3(0.f, 1.f, 0.f);

Vec3 Vec3::Random() {
	return {RandomFloat(), RandomFloat(), RandomFloat()};
}

Vec3 Vec3::operator+(const Vec3& other) const {
	return {this->x + other.x, this->y + other.y, this->z + other.z};
}

Vec3 Vec3::operator-(const Vec3& other) const {
	return {this->x - other.x, this->y - other.y, this->z - other.z};
}

Vec3& Vec3::operator=(const Vec3& other) {
	this->x = other.x;
	this->y = other.y;
	this->z = other.z;
	return *this;
}

Vec3& Vec3::operator+=(const Vec3& other) {
	this->x += other.x;
	this->y += other.y;
	this->z += other.z;
	return *this;
}

Vec3& Vec3::operator-=(const Vec3& other) {
	this->x -= other.x;
	this->y -= other.y;
	this->z -= other.z;
	return *this;
}

Vec3& Vec3::operator*=(const Vec3& other)
{
	this->x *= other.x;
	this->y *= other.y;
	this->z *= other.z;
	return *this;
}

Vec3& Vec3::operator/=(const Vec3& other)
{
	this->x /= other.x;
	this->y /= other.y;
	this->z /= other.z;
	return *this;
}

Vec3 Vec3::operator*(float scalar) const {
	return {this->x * scalar, this->y * scalar, this->z * scalar};
}

Vec3 Vec3::operator/(float scalar) const {
	return {this->x / scalar, this->y / scalar, this->z / scalar};
}

Vec3 Vec3::operator+(float scalar) const {
	return {this->x + scalar, this->y + scalar, this->z + scalar};
}

Vec3 Vec3::operator-(float scalar) const {
	return {this->x - scalar, this->y - scalar, this->z - scalar};
}

Vec3& Vec3::operator+=(float s) {
	this->x += s;
	this->y += s;
	this->z += s;
	return *this;
}
Vec3& Vec3::operator-=(float s) {
	this->x -= s;
	this->y -= s;
	this->z -= s;
	return *this;
}
Vec3& Vec3::operator*=(float s) {
	this->x *= s;
	this->y *= s;
	this->z *= s;
	return *this;
}
Vec3& Vec3::operator/=(float s) {
	this->x /= s;
	this->y /= s;
	this->z /= s;
	return *this;
}


// cmpf uses compares the floats with an error (EPSILON)
bool Vec3::operator==(const Vec3& other) const {
	return Cmpf(this->x , other.x) && Cmpf(this->y, other.y) && Cmpf(this->z, other.z);
}

bool Vec3::operator!=(const Vec3& other) const {
	return !(*this == other);
}

float Vec3::Angle(const Vec3& a, const Vec3& b) {
	float magnitudeA = a.Magnitude();
	float magnitudeB = b.Magnitude();

	if (Cmpf(magnitudeA, 0.0f) || Cmpf(magnitudeB, 0.0f))
		return 0.0f;

	return RadiansToDegrees(std::acos(Dot(a, b) / (magnitudeA * magnitudeB)));
}


float Vec3::Magnitude() const {
	return std::sqrt(this->x * this->x + this->y * this->y + this->z*this->z);
}

float Vec3::MagnitudeSquared() const {
	return this->x * this->x + this->y * this->y + this->z * this->z;
}

void Vec3::ToOpenGLFormat(float array[3]) const {
	array[0] = this->x;
	array[1] = this->y;
	array[2] = this->z;
}


Vec3 Vec3::Normalize() const {

	float magnitude = this->Magnitude();

	if (Cmpf(magnitude, 0.0f)) {
	    return *this;
	}

	// Cannot divide by 0
	//assert(magnitude > 0);

	return (*this) / magnitude;
}
Vec3 Vec3::NormalizedPerpendicular() const {
    if (abs(x) > abs(y))
    {
        float len = sqrt(x * x + z * z);
        return Vec3(z, 0.0f, -x) / len;
    }
    else
    {
        float len = sqrt(y * y + z * z);
        return Vec3(0.0f, z, -y) / len;
    }
}

Vec4 Vec3::ToVec4() const {
	return {this->x, this->y, this->z, 0};
}

Vec3 Vec3::ClampMagnitude(float newMagnitude) const
{
	if (MagnitudeSquared() > newMagnitude * newMagnitude) {
		return Normalize() * newMagnitude;
	}
	return *this;
}

Vec3 Vec3::Abs() const {
	return { std::abs(x), std::abs(y), std::abs(z) };
}

float Vec3::AngleRad(const Vec3& a, const Vec3& b)
{
	// Assumes a and b are Normalized

	const float cosine = Dot(a, b);
	const float sqrMagnitudeA = a.MagnitudeSquared();
	const float sqrMagnitudeB = b.MagnitudeSquared();

	return std::acos(cosine/std::sqrt(sqrMagnitudeA * sqrMagnitudeB));
}

Vec3 Vec3::Lerp(Vec3 initial, Vec3 final, float f) {
	return initial + f * (final - initial);
}

float Dot(const Vec3& a, const Vec3& b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 Cross(const Vec3& a, const Vec3& b) {
	return {(a.y * b.z - a.z * b.y),(a.z * b.x - a.x * b.z),(a.x * b.y - a.y * b.x)};
}

// K has to be normalized
// Thetha should be converted to radians
// Vrot = v*cos() + (k x v)*sin() + k*(k.v) * (1- cos())  
Vec3 Rodrigues(Vec3 v, float thetaRadians, Vec3 k) {
	Vec3 unitK = k.Normalize();

	return v * std::cos(thetaRadians) + Cross(unitK, v)* std::sin(thetaRadians) + unitK * Dot(unitK, v) * (1 - std::cos(thetaRadians));
}

std::ostream& operator<<(std::ostream& os, const Vec3& vec3) {
	return os << "(" << vec3.x << "," << vec3.y << "," << vec3.z << ')';
}

Vec3 operator*(float scalar, const Vec3& vec3) {
	return {scalar * vec3.x, scalar * vec3.y, scalar * vec3.z};
}

Vec3 operator+(float scalar, const Vec3& vec3) {
	return {scalar + vec3.x, scalar + vec3.y, scalar + vec3.z};
}

Vec3 operator-(float scalar, const Vec3& vec3) {
	return {scalar - vec3.x, scalar - vec3.y, scalar - vec3.z};
}

Vec3 Vec3::operator-() const {
    return {-x, -y, -z};
}
