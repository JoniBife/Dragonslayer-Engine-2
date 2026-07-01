#pragma once

#include "Core/Export.hpp"
#include "Core/Macros.hpp"
#include "MathFwd.hpp"

struct ENGINE_API Quat {

    float x, y, z, t;

    void Clean();

    Quat();
    Quat(float thetaRad, const Vec3& axis);
    Quat(float t, float x, float y, float z);

    static Quat IDENTITY;

    void ToAngleAxis(float& thetaRad, Vec3& axis) const;

    NO_DISCARD float Quadrance() const;
    NO_DISCARD float Norm() const;
    NO_DISCARD float Angle() const;

    NO_DISCARD Quat Normalize() const;
    NO_DISCARD Quat Conjugate() const;
    NO_DISCARD Quat Inverse() const;

    void ToEuler(float& roll, float& pitch, float& yaw) const;

    Quat operator*(float s) const;
    friend ENGINE_API Quat operator*(float s, const Quat& q);
    Quat operator*(const Quat& q) const;
    Quat operator+(const Quat& q) const;
    Quat& operator=(const Quat& q);

    Quat& operator*=(float s);
    Quat& operator*=(const Quat& q);
    Quat& operator+=(const Quat& q);

    bool operator==(const Quat& q) const;
    bool operator!=(const Quat& q) const;

    static Quat Lerp(const Quat& q0, const Quat& q1, float k);
    static Quat Slerp(const Quat& q0, const Quat& q1, float k);
    static Quat FromDir(const Vec3& dir, const Vec3& ref);
    static Quat FromEuler(float roll, float pitch, float yaw);
    static Quat FromTo(const Vec3& from, const Vec3& to);

    NO_DISCARD Mat4 ToRotationMatrix() const;
    NO_DISCARD Mat3 ToRotationMatrix3x3() const;
};
