#include "Framework/TestFramework.hpp"
#include "Math/MathAux.hpp"
#include "Math/Quat.hpp"

TEST(Quat, DefaultConstruction) {
    Quat q;
    TEST_CHECK(Cmpf(q.t, 1.0f));
    TEST_CHECK(Cmpf(q.x, 0.0f));
    TEST_CHECK(Cmpf(q.y, 0.0f));
    TEST_CHECK(Cmpf(q.z, 0.0f));
}

TEST(Quat, ComponentConstruction) {
    Quat q(1.0f, 2.0f, 3.0f, 4.0f);
    TEST_CHECK(Cmpf(q.t, 1.0f));
    TEST_CHECK(Cmpf(q.x, 2.0f));
    TEST_CHECK(Cmpf(q.y, 3.0f));
    TEST_CHECK(Cmpf(q.z, 4.0f));
}

TEST(Quat, AngleAxisConstruction) {
    Quat q(PI / 2.0f, Vec3::Y);
    // Half angle = PI/4, so t = cos(PI/4), y = sin(PI/4)
    float halfAngle = PI / 4.0f;
    TEST_CHECK(Cmpf(q.t, std::cos(halfAngle)));
    TEST_CHECK(Cmpf(q.x, 0.0f));
    TEST_CHECK(Cmpf(q.y, std::sin(halfAngle)));
    TEST_CHECK(Cmpf(q.z, 0.0f));
}

TEST(Quat, StaticIdentity) {
    Quat id = Quat::IDENTITY;
    TEST_CHECK(Cmpf(id.t, 1.0f));
    TEST_CHECK(Cmpf(id.x, 0.0f));
    TEST_CHECK(Cmpf(id.y, 0.0f));
    TEST_CHECK(Cmpf(id.z, 0.0f));
}

TEST(Quat, Equality) {
    TEST_CHECK(Quat::IDENTITY == Quat(1.0f, 0.0f, 0.0f, 0.0f));
    TEST_CHECK_FALSE(Quat::IDENTITY == Quat(PI / 2.0f, Vec3::Y));
}

TEST(Quat, Inequality) {
    TEST_CHECK(Quat::IDENTITY != Quat(PI / 2.0f, Vec3::Y));
    TEST_CHECK_FALSE(Quat::IDENTITY != Quat::IDENTITY);
}

TEST(Quat, MultiplyScalar) {
    Quat q(1.0f, 0.0f, 0.0f, 0.0f);
    Quat result = q * 2.0f;
    TEST_CHECK(Cmpf(result.t, 2.0f));
    TEST_CHECK(Cmpf(result.x, 0.0f));
    TEST_CHECK(Cmpf(result.y, 0.0f));
    TEST_CHECK(Cmpf(result.z, 0.0f));
}

TEST(Quat, ScalarMultiply) {
    Quat q(1.0f, 0.0f, 0.0f, 0.0f);
    Quat result = 2.0f * q;
    TEST_CHECK(Cmpf(result.t, 2.0f));
}

TEST(Quat, MultiplyQuatIdentity) {
    Quat q(PI / 2.0f, Vec3::Y);
    Quat result = Quat::IDENTITY * q;
    TEST_CHECK_EQUAL(result, q);
}

TEST(Quat, MultiplyQuatComposition) {
    // Two 90-degree rotations about Y = 180-degree rotation about Y
    Quat q90 = Quat(PI / 2.0f, Vec3::Y);
    Quat q180 = q90 * q90;
    TEST_CHECK(Cmpf(q180.Angle(), PI));
}

TEST(Quat, AddQuat) {
    Quat a(1.0f, 2.0f, 3.0f, 4.0f);
    Quat b(4.0f, 3.0f, 2.0f, 1.0f);
    Quat result = a + b;
    TEST_CHECK(Cmpf(result.t, 5.0f));
    TEST_CHECK(Cmpf(result.x, 5.0f));
    TEST_CHECK(Cmpf(result.y, 5.0f));
    TEST_CHECK(Cmpf(result.z, 5.0f));
}

TEST(Quat, TimesEqualsScalar) {
    Quat q(1.0f, 0.0f, 0.0f, 0.0f);
    q *= 3.0f;
    TEST_CHECK(Cmpf(q.t, 3.0f));
}

TEST(Quat, TimesEqualsQuat) {
    Quat q(PI / 2.0f, Vec3::Y);
    Quat copy = q;
    q *= Quat::IDENTITY;
    TEST_CHECK_EQUAL(q, copy);
}

TEST(Quat, PlusEqualsQuat) {
    Quat q(1.0f, 0.0f, 0.0f, 0.0f);
    q += Quat(0.0f, 1.0f, 0.0f, 0.0f);
    TEST_CHECK(Cmpf(q.t, 1.0f));
    TEST_CHECK(Cmpf(q.x, 1.0f));
}

TEST(Quat, Quadrance) {
    TEST_CHECK(Cmpf(Quat::IDENTITY.Quadrance(), 1.0f));
}

TEST(Quat, QuadranceNonUnit) {
    Quat q(2.0f, 0.0f, 0.0f, 0.0f);
    TEST_CHECK(Cmpf(q.Quadrance(), 4.0f));
}

TEST(Quat, NormIdentity) {
    TEST_CHECK(Cmpf(Quat::IDENTITY.Norm(), 1.0f));
}

TEST(Quat, NormNonUnit) {
    Quat q(2.0f, 0.0f, 0.0f, 0.0f);
    TEST_CHECK(Cmpf(q.Norm(), 2.0f));
}

TEST(Quat, Angle) {
    Quat q(PI / 2.0f, Vec3::Y);
    TEST_CHECK(Cmpf(q.Angle(), PI / 2.0f));
}

TEST(Quat, AngleIdentity) {
    TEST_CHECK(Cmpf(Quat::IDENTITY.Angle(), 0.0f));
}

TEST(Quat, Normalize) {
    Quat q(2.0f, 0.0f, 0.0f, 0.0f);
    Quat n = q.Normalize();
    TEST_CHECK(Cmpf(n.Norm(), 1.0f));
    TEST_CHECK(Cmpf(n.t, 1.0f));
}

TEST(Quat, NormalizeIdentity) {
    Quat n = Quat::IDENTITY.Normalize();
    TEST_CHECK_EQUAL(n, Quat::IDENTITY);
}

TEST(Quat, Conjugate) {
    Quat q(1.0f, 2.0f, 3.0f, 4.0f);
    Quat c = q.Conjugate();
    TEST_CHECK(Cmpf(c.t, 1.0f));
    TEST_CHECK(Cmpf(c.x, -2.0f));
    TEST_CHECK(Cmpf(c.y, -3.0f));
    TEST_CHECK(Cmpf(c.z, -4.0f));
}

TEST(Quat, InverseUnit) {
    Quat q(PI / 2.0f, Vec3::Y);
    Quat inv = q.Inverse();
    Quat result = q * inv;
    TEST_CHECK(Cmpf(result.t, Quat::IDENTITY.t));
    TEST_CHECK(Cmpf(result.x, Quat::IDENTITY.x));
    TEST_CHECK(Cmpf(result.y, Quat::IDENTITY.y));
    TEST_CHECK(Cmpf(result.z, Quat::IDENTITY.z));
}

TEST(Quat, InverseIdentity) {
    Quat inv = Quat::IDENTITY.Inverse();
    TEST_CHECK_EQUAL(inv, Quat::IDENTITY);
}

TEST(Quat, ToAngleAxisRoundTrip) {
    float angle;
    Vec3 axis;
    Quat q(PI / 2.0f, Vec3::Y);
    q.ToAngleAxis(angle, axis);
    TEST_CHECK(Cmpf(angle, PI / 2.0f));
    TEST_CHECK(Cmpf(axis.x, 0.0f));
    TEST_CHECK(Cmpf(axis.y, 1.0f));
    TEST_CHECK(Cmpf(axis.z, 0.0f));
}

TEST(Quat, FromEulerAndToEuler) {
    float roll = 0.0f;
    float pitch = PI / 4.0f;
    float yaw = 0.0f;
    Quat q = Quat::FromEuler(roll, pitch, yaw);
    float outRoll, outPitch, outYaw;
    q.ToEuler(outRoll, outPitch, outYaw);
    TEST_CHECK(Cmpf(outRoll, roll));
    TEST_CHECK(Cmpf(outPitch, pitch));
    TEST_CHECK(Cmpf(outYaw, yaw));
}

TEST(Quat, ToRotationMatrixIdentity) {
    Mat4 m = Quat::IDENTITY.ToRotationMatrix();
    TEST_CHECK_EQUAL(m, Mat4::IDENTITY);
}

TEST(Quat, ToRotationMatrix90Y) {
    Quat q(PI / 2.0f, Vec3::Y);
    Mat4 m = q.ToRotationMatrix();
    // Rotating Z by this matrix should give X
    Vec3 result = m * Vec3::Z;
    TEST_CHECK(Cmpf(result.x, 1.0f));
    TEST_CHECK(Cmpf(result.y, 0.0f));
    TEST_CHECK(Cmpf(result.z, 0.0f));
}

TEST(Quat, LerpAtZero) {
    Quat q0 = Quat::IDENTITY;
    Quat q1(PI / 2.0f, Vec3::Y);
    Quat result = Quat::Lerp(q0, q1, 0.0f);
    TEST_CHECK_EQUAL(result, q0);
}

TEST(Quat, LerpAtOne) {
    Quat q0 = Quat::IDENTITY;
    Quat q1(PI / 2.0f, Vec3::Y);
    Quat result = Quat::Lerp(q0, q1, 1.0f);
    TEST_CHECK_EQUAL(result, q1);
}

TEST(Quat, SlerpAtZero) {
    Quat q0 = Quat::IDENTITY;
    Quat q1(PI / 2.0f, Vec3::Y);
    Quat result = Quat::Slerp(q0, q1, 0.0f);
    TEST_CHECK(Cmpf(result.t, q0.t));
    TEST_CHECK(Cmpf(result.x, q0.x));
    TEST_CHECK(Cmpf(result.y, q0.y));
    TEST_CHECK(Cmpf(result.z, q0.z));
}

TEST(Quat, SlerpAtOne) {
    Quat q0 = Quat::IDENTITY;
    Quat q1(PI / 2.0f, Vec3::Y);
    Quat result = Quat::Slerp(q0, q1, 1.0f);
    TEST_CHECK(Cmpf(result.t, q1.t));
    TEST_CHECK(Cmpf(result.x, q1.x));
    TEST_CHECK(Cmpf(result.y, q1.y));
    TEST_CHECK(Cmpf(result.z, q1.z));
}

TEST(Quat, SlerpAtHalf) {
    Quat q0 = Quat::IDENTITY;
    Quat q1(PI / 2.0f, Vec3::Y);
    Quat result = Quat::Slerp(q0, q1, 0.5f);
    // Should be approximately a PI/4 rotation about Y
    TEST_CHECK(Cmpf(result.Angle(), PI / 4.0f));
}

TEST(Quat, SlerpIdentical) {
    Quat q(PI / 2.0f, Vec3::Y);
    Quat result = Quat::Slerp(q, q, 0.5f);
    TEST_CHECK_EQUAL(result, q);
}

TEST(Quat, FromDir) {
    Quat q = Quat::FromDir(Vec3::FORWARD);
    Mat4 m = q.ToRotationMatrix();
    Vec3 fwd = m.GetForwardAxis();
    TEST_CHECK(Cmpf(Dot(fwd.Normalize(), Vec3::FORWARD.Normalize()), 1.0f));
}
