#pragma once

#include <iostream>

#include "Core/Export.hpp"
#include "Core/Macros.hpp"
#include "Vec3.hpp"

struct ENGINE_API Mat3 {

	float m[3][3];

	Mat3();

	explicit Mat3(float fill);

	Mat3(float l1c1, float l1c2, float l1c3,
	     float l2c1, float l2c2, float l2c3,
		 float l3c1, float l3c2, float l3c3);
	Mat3(const Mat3& other);
	Mat3(const Vec3& forward, const Vec3& right, const Vec3& up);

	static Mat3 ZERO;
	static Mat3 IDENTITY;
	static Mat3 Dual(const Vec3& v);
	static Mat3 RandomMatrix(float min, float max);
	static Mat3 Scaling(const Vec3& scale);

	Mat3& operator=(const Mat3& other);
	Mat3& operator+=(const Mat3& other);
	Mat3& operator-=(const Mat3& other);
	Mat3& operator*=(const Mat3& other);
	Mat3& operator*=(float s);
	Mat3& operator/=(float s);
	Mat3& operator+=(float s);
	Mat3& operator-=(float s);

	bool operator==(const Mat3& other) const;
	bool operator!=(const Mat3& other) const;

	Mat3 operator+(const Mat3& other) const;
	Mat3 operator-(const Mat3& other) const;
	Mat3 operator*(const Mat3& other) const;
	Mat3 operator*(float s) const;
	friend ENGINE_API Mat3 operator*(float s, const Mat3& mat3);
	Mat3 operator+(float s) const;
	friend ENGINE_API Mat3 operator+(float s, const Mat3& mat3);
	Mat3 operator-(float s) const;
	friend ENGINE_API Mat3 operator-(float s, const Mat3& mat3);
	Mat3 operator/(float s) const;
	Vec3 operator*(const Vec3& v) const;

	float* operator[](int lines);

	NO_DISCARD float Determinant() const;
	NO_DISCARD Mat3 Transpose() const;
	NO_DISCARD struct Quat ToQuaternion() const;
	NO_DISCARD Vec3 GetRightAxis() const;
	NO_DISCARD Vec3 GetUpAxis() const;
	NO_DISCARD Vec3 GetForwardAxis() const;
	/*
	* Returns false if this is not invertible else true;
	* @param inverse will contain the inverse matrix of this
	*/
	bool Inverse(Mat3& inverse) const;
	void ToOpenGLFormat(float array[9]) const;
	NO_DISCARD bool IsOrthogonal() const;

	friend std::ostream& operator<<(std::ostream& os, const Mat3& mat3);
	friend std::istream& operator>>(std::istream& is, Mat3& mat3);
};