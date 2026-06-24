#pragma once

#include "Core/Export.hpp"
#include "Core/Macros.hpp"
#include "Vec2.hpp"

struct ENGINE_API Mat2 {

	float m[2][2];

	Mat2();
    explicit Mat2(float fill);
	Mat2(float l1c1, float l1c2, float l2c1, float l2c2);
	Mat2(const Mat2& other);

	static Mat2 IDENTITY;

	Mat2& operator=(const Mat2& other);
	Mat2& operator+=(const Mat2& other);
	Mat2& operator-=(const Mat2& other);
	Mat2& operator*=(const Mat2& other);
	Mat2& operator*=(float s);
	Mat2& operator/=(float s);
	Mat2& operator+=(float s);
	Mat2& operator-=(float s);

	bool operator==(const Mat2& other) const;
	bool operator!=(const Mat2& other) const;

	Mat2 operator+(const Mat2& other) const;
	Mat2 operator-(const Mat2& other) const;
	Mat2 operator*(const Mat2& other) const;
	Mat2 operator*(float s) const;
	friend ENGINE_API Mat2 operator*(float s, const Mat2& mat2);
	Mat2 operator+(float s) const;
	friend ENGINE_API Mat2 operator+(float s, const Mat2& mat2);
	Mat2 operator-(float s) const;
	friend ENGINE_API Mat2 operator-(float s, const Mat2& mat2);
	Mat2 operator/(float s) const;
	Vec2 operator*(const Vec2& v) const;

	float* operator[](int lines);

	NO_DISCARD Mat2 Transpose() const;
	/*
	* Returns false if this is not invertible
	* else true;
	* @param inverse will contain the inverse matrix of this
	*/
	bool Inverse(Mat2& inverse) const;
	NO_DISCARD float Determinant() const;
	void ToOpenGLFormat(float array[4]) const;
	NO_DISCARD bool IsOrthogonal() const;

	/*
	 * Print result example:
	* [ 1 , 0 ]
	* [ 0 , 1 ]
	*/
	friend std::ostream& operator<<(std::ostream& os, const Mat2& mat2);
	friend std::istream& operator>>(std::istream& is, Mat2& mat2);
};
