#pragma once

#include "Core/Export.hpp"
#include "Core/Macros.hpp"
#include "MathFwd.hpp"

struct ENGINE_API Mat4 {

    float m[4][4];

    Mat4();
    explicit Mat4(float fill);
    Mat4(float l1c1, float l1c2, float l1c3, float l1c4,
         float l2c1, float l2c2, float l2c3, float l2c4,
         float l3c1, float l3c2, float l3c3, float l3c4,
         float l4c1, float l4c2, float l4c3, float l4c4);
    Mat4(const Mat4& other);

    static Mat4 IDENTITY;
    static Mat4 ZERO;
    static Mat4 Scaling(float xyz);
    static Mat4 Scaling(float x, float y, float z);
    static Mat4 Scaling(const Vec3& v);
    static Mat4 Translation(float x, float y, float z);
    static Mat4 Translation(const Vec3& v);
    static Mat4 Rotation(float angleRad, const Vec3& axis);
    static Mat4 RotationFromDir(const Vec3& dir, const Vec3& up);

    static Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up);
    static Mat4 Orthographic(float left, float right, float bottom, float top);
    static Mat4 Orthographic(float left, float right, float bottom, float top, float near, float far);
    static Mat4 OrthographicVulkan(float left, float right, float bottom, float top, float near, float far);
    static Mat4 Perspective(float fovYRad, float aspectRatio, float near, float far);
    static Mat4 PerspectiveVulkan(float fovYRad, float aspectRatio, float near, float far);

    Mat4& operator=(const Mat4& other);
    Mat4& operator+=(const Mat4& other);
    Mat4& operator-=(const Mat4& other);
    Mat4& operator*=(const Mat4& other);
    Mat4& operator*=(float s);
    Mat4& operator/=(float s);
    Mat4& operator+=(float s);
    Mat4& operator-=(float s);

    bool operator==(const Mat4& other) const;
    bool operator!=(const Mat4& other) const;

    Mat4 operator+(const Mat4& other) const;
    Mat4 operator-(const Mat4& other) const;
    Mat4 operator*(const Mat4& other) const;
    Mat4 operator*(float s) const;
    friend ENGINE_API Mat4 operator*(float s, const Mat4& mat4);
    Mat4 operator+(float s) const;
    friend ENGINE_API Mat4 operator+(float s, const Mat4& mat4);
    Mat4 operator-(float s) const;
    friend ENGINE_API Mat4 operator-(float s, const Mat4& mat4);
    Mat4 operator/(float s) const;
    Vec4 operator*(const Vec4& v) const;
    Vec3 operator*(const Vec3& v) const; // This operation is incorrect from linear algebra's perspective but it's useful

    float* operator[](int lines);

    /*
    * Returns false if this is not invertible else true;
    * @param inverse will contain the inverse matrix of this
    */
    bool Inverse(Mat4& inverse) const;
    NO_DISCARD Mat4 Transpose() const;
    void ToOpenGLFormat(float array[16]) const;

    NO_DISCARD Mat3 ToMat3() const; //removes last line and column
    NO_DISCARD bool IsOrthogonal() const;

    NO_DISCARD Vec3 GetRightAxis() const;
    NO_DISCARD Vec3 GetUpAxis() const;
    NO_DISCARD Vec3 GetForwardAxis() const;

    /* Decomposes a transformation matrix in each of its components */
    void Decompose(Vec3& scale, Vec3& rotation, Vec3& position) const;
};
