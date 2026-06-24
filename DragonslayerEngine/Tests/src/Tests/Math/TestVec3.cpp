#include "Framework/TestFramework.hpp"
#include "Math/MathAux.hpp"
#include "Math/Vec3.hpp"

TEST(Vec3, DefaultConstruction) {
    Vec3 v;
    TEST_CHECK(Cmpf(v.x, 0.0f));
    TEST_CHECK(Cmpf(v.y, 0.0f));
    TEST_CHECK(Cmpf(v.z, 0.0f));
}

TEST(Vec3, ScalarConstruction) {
    Vec3 v(5.0f);
    TEST_CHECK(Cmpf(v.x, 5.0f));
    TEST_CHECK(Cmpf(v.y, 5.0f));
    TEST_CHECK(Cmpf(v.z, 5.0f));
}

TEST(Vec3, ComponentConstruction) {
    Vec3 v(1.0f, 2.0f, 3.0f);
    TEST_CHECK(Cmpf(v.x, 1.0f));
    TEST_CHECK(Cmpf(v.y, 2.0f));
    TEST_CHECK(Cmpf(v.z, 3.0f));
}

TEST(Vec3, StaticConstants) {
    TEST_CHECK_EQUAL(Vec3::X, Vec3(1.0f, 0.0f, 0.0f));
    TEST_CHECK_EQUAL(Vec3::Y, Vec3(0.0f, 1.0f, 0.0f));
    TEST_CHECK_EQUAL(Vec3::Z, Vec3(0.0f, 0.0f, 1.0f));
    TEST_CHECK_EQUAL(Vec3::ONE, Vec3(1.0f, 1.0f, 1.0f));
    TEST_CHECK_EQUAL(Vec3::ZERO, Vec3(0.0f, 0.0f, 0.0f));
}

TEST(Vec3, StaticDirections) {
    TEST_CHECK(Cmpf(Dot(Vec3::FORWARD, Vec3::RIGHT), 0.0f));
    TEST_CHECK(Cmpf(Dot(Vec3::FORWARD, Vec3::UP), 0.0f));
    TEST_CHECK(Cmpf(Dot(Vec3::RIGHT, Vec3::UP), 0.0f));
}

TEST(Vec3, AdditionVec) {
    Vec3 result = Vec3(1.0f, 2.0f, 3.0f) + Vec3(4.0f, 5.0f, 6.0f);
    TEST_CHECK_EQUAL(result, Vec3(5.0f, 7.0f, 9.0f));
}

TEST(Vec3, SubtractionVec) {
    Vec3 result = Vec3(5.0f, 6.0f, 7.0f) - Vec3(1.0f, 2.0f, 3.0f);
    TEST_CHECK_EQUAL(result, Vec3(4.0f, 4.0f, 4.0f));
}

TEST(Vec3, MultiplyScalar) {
    Vec3 result = Vec3(1.0f, 2.0f, 3.0f) * 2.0f;
    TEST_CHECK_EQUAL(result, Vec3(2.0f, 4.0f, 6.0f));
}

TEST(Vec3, ScalarMultiplyVec) {
    Vec3 result = 2.0f * Vec3(1.0f, 2.0f, 3.0f);
    TEST_CHECK_EQUAL(result, Vec3(2.0f, 4.0f, 6.0f));
}

TEST(Vec3, DivideScalar) {
    Vec3 result = Vec3(2.0f, 4.0f, 6.0f) / 2.0f;
    TEST_CHECK_EQUAL(result, Vec3(1.0f, 2.0f, 3.0f));
}

TEST(Vec3, AddScalar) {
    Vec3 result = Vec3(1.0f, 2.0f, 3.0f) + 1.0f;
    TEST_CHECK_EQUAL(result, Vec3(2.0f, 3.0f, 4.0f));
}

TEST(Vec3, SubtractScalar) {
    Vec3 result = Vec3(5.0f, 6.0f, 7.0f) - 1.0f;
    TEST_CHECK_EQUAL(result, Vec3(4.0f, 5.0f, 6.0f));
}

TEST(Vec3, UnaryNegation) {
    Vec3 result = -Vec3(1.0f, -2.0f, 3.0f);
    TEST_CHECK_EQUAL(result, Vec3(-1.0f, 2.0f, -3.0f));
}

TEST(Vec3, PlusEqualsVec) {
    Vec3 v(1.0f, 2.0f, 3.0f);
    v += Vec3(4.0f, 5.0f, 6.0f);
    TEST_CHECK_EQUAL(v, Vec3(5.0f, 7.0f, 9.0f));
}

TEST(Vec3, MinusEqualsVec) {
    Vec3 v(5.0f, 6.0f, 7.0f);
    v -= Vec3(1.0f, 2.0f, 3.0f);
    TEST_CHECK_EQUAL(v, Vec3(4.0f, 4.0f, 4.0f));
}

TEST(Vec3, TimesEqualsVec) {
    Vec3 v(2.0f, 3.0f, 4.0f);
    v *= Vec3(2.0f, 3.0f, 4.0f);
    TEST_CHECK_EQUAL(v, Vec3(4.0f, 9.0f, 16.0f));
}

TEST(Vec3, DivEqualsVec) {
    Vec3 v(4.0f, 9.0f, 16.0f);
    v /= Vec3(2.0f, 3.0f, 4.0f);
    TEST_CHECK_EQUAL(v, Vec3(2.0f, 3.0f, 4.0f));
}

TEST(Vec3, PlusEqualsScalar) {
    Vec3 v(1.0f, 2.0f, 3.0f);
    v += 1.0f;
    TEST_CHECK_EQUAL(v, Vec3(2.0f, 3.0f, 4.0f));
}

TEST(Vec3, MinusEqualsScalar) {
    Vec3 v(5.0f, 6.0f, 7.0f);
    v -= 1.0f;
    TEST_CHECK_EQUAL(v, Vec3(4.0f, 5.0f, 6.0f));
}

TEST(Vec3, TimesEqualsScalar) {
    Vec3 v(1.0f, 2.0f, 3.0f);
    v *= 2.0f;
    TEST_CHECK_EQUAL(v, Vec3(2.0f, 4.0f, 6.0f));
}

TEST(Vec3, DivEqualsScalar) {
    Vec3 v(2.0f, 4.0f, 6.0f);
    v /= 2.0f;
    TEST_CHECK_EQUAL(v, Vec3(1.0f, 2.0f, 3.0f));
}

TEST(Vec3, Equality) {
    TEST_CHECK(Vec3(1.0f, 2.0f, 3.0f) == Vec3(1.0f, 2.0f, 3.0f));
    TEST_CHECK_FALSE(Vec3(1.0f, 2.0f, 3.0f) == Vec3(4.0f, 5.0f, 6.0f));
}

TEST(Vec3, Inequality) {
    TEST_CHECK(Vec3(1.0f, 2.0f, 3.0f) != Vec3(4.0f, 5.0f, 6.0f));
    TEST_CHECK_FALSE(Vec3(1.0f, 2.0f, 3.0f) != Vec3(1.0f, 2.0f, 3.0f));
}

TEST(Vec3, Magnitude) {
    // sqrt(4 + 9 + 36) = sqrt(49) = 7
    TEST_CHECK(Cmpf(Vec3(2.0f, 3.0f, 6.0f).Magnitude(), 7.0f));
}

TEST(Vec3, MagnitudeSquared) {
    TEST_CHECK(Cmpf(Vec3(2.0f, 3.0f, 6.0f).MagnitudeSquared(), 49.0f));
}

TEST(Vec3, MagnitudeUnit) {
    TEST_CHECK(Cmpf(Vec3::X.Magnitude(), 1.0f));
    TEST_CHECK(Cmpf(Vec3::Y.Magnitude(), 1.0f));
    TEST_CHECK(Cmpf(Vec3::Z.Magnitude(), 1.0f));
}

TEST(Vec3, Normalize) {
    Vec3 n = Vec3(0.0f, 3.0f, 4.0f).Normalize();
    TEST_CHECK(Cmpf(n.Magnitude(), 1.0f));
    TEST_CHECK(Cmpf(n.x, 0.0f));
    TEST_CHECK(Cmpf(n.y, 3.0f / 5.0f));
    TEST_CHECK(Cmpf(n.z, 4.0f / 5.0f));
}

TEST(Vec3, NormalizeAlreadyUnit) {
    Vec3 n = Vec3::Y.Normalize();
    TEST_CHECK_EQUAL(n, Vec3(0.0f, 1.0f, 0.0f));
}

TEST(Vec3, ClampMagnitude) {
    Vec3 v(3.0f, 4.0f, 0.0f); // magnitude = 5
    Vec3 clamped = v.ClampMagnitude(1.0f);
    TEST_CHECK(Cmpf(clamped.Magnitude(), 1.0f));
    // Direction preserved
    TEST_CHECK(Cmpf(clamped.x / clamped.y, 3.0f / 4.0f));
}

TEST(Vec3, Abs) {
    Vec3 result = Vec3(-1.0f, 2.0f, -3.0f).Abs();
    TEST_CHECK_EQUAL(result, Vec3(1.0f, 2.0f, 3.0f));
}

TEST(Vec3, AbsAlreadyPositive) {
    Vec3 result = Vec3(1.0f, 2.0f, 3.0f).Abs();
    TEST_CHECK_EQUAL(result, Vec3(1.0f, 2.0f, 3.0f));
}

TEST(Vec3, DotPerpendicular) {
    TEST_CHECK(Cmpf(Dot(Vec3::X, Vec3::Y), 0.0f));
    TEST_CHECK(Cmpf(Dot(Vec3::X, Vec3::Z), 0.0f));
    TEST_CHECK(Cmpf(Dot(Vec3::Y, Vec3::Z), 0.0f));
}

TEST(Vec3, DotParallel) {
    TEST_CHECK(Cmpf(Dot(Vec3::X, Vec3::X), 1.0f));
}

TEST(Vec3, DotAntiParallel) {
    TEST_CHECK(Cmpf(Dot(Vec3::X, -Vec3::X), -1.0f));
}

TEST(Vec3, DotGeneral) {
    // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
    TEST_CHECK(Cmpf(Dot(Vec3(1.0f, 2.0f, 3.0f), Vec3(4.0f, 5.0f, 6.0f)), 32.0f));
}

TEST(Vec3, CrossProductBasis) {
    TEST_CHECK_EQUAL(Cross(Vec3::X, Vec3::Y), Vec3::Z);
    TEST_CHECK_EQUAL(Cross(Vec3::Y, Vec3::Z), Vec3::X);
    TEST_CHECK_EQUAL(Cross(Vec3::Z, Vec3::X), Vec3::Y);
}

TEST(Vec3, CrossProductAntiCommutative) {
    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(4.0f, 5.0f, 6.0f);
    TEST_CHECK_EQUAL(Cross(a, b), -Cross(b, a));
}

TEST(Vec3, CrossProductSelfIsZero) {
    TEST_CHECK_EQUAL(Cross(Vec3::X, Vec3::X), Vec3::ZERO);
}

TEST(Vec3, AnglePerpendicular) {
    TEST_CHECK(Cmpf(Vec3::Angle(Vec3::X, Vec3::Y), 90.0f));
}

TEST(Vec3, AngleParallel) {
    TEST_CHECK(Cmpf(Vec3::Angle(Vec3::X, Vec3::X), 0.0f));
}

TEST(Vec3, AngleRadPerpendicular) {
    TEST_CHECK(Cmpf(Vec3::AngleRad(Vec3::X, Vec3::Y), PI / 2.0f));
}

TEST(Vec3, LerpAtZero) {
    Vec3 result = Vec3::Lerp(Vec3::ZERO, Vec3::ONE, 0.0f);
    TEST_CHECK_EQUAL(result, Vec3::ZERO);
}

TEST(Vec3, LerpAtOne) {
    Vec3 result = Vec3::Lerp(Vec3::ZERO, Vec3::ONE, 1.0f);
    TEST_CHECK_EQUAL(result, Vec3::ONE);
}

TEST(Vec3, LerpAtHalf) {
    Vec3 result = Vec3::Lerp(Vec3::ZERO, Vec3(10.0f, 10.0f, 10.0f), 0.5f);
    TEST_CHECK_EQUAL(result, Vec3(5.0f));
}

TEST(Vec3, ToVec4) {
    Vec4 result = Vec3(1.0f, 2.0f, 3.0f).ToVec4();
    TEST_CHECK(Cmpf(result.x, 1.0f));
    TEST_CHECK(Cmpf(result.y, 2.0f));
    TEST_CHECK(Cmpf(result.z, 3.0f));
}

TEST(Vec3, Rodrigues) {
    // Rotate X axis by PI/2 around Z axis -> should give Y axis
    Vec3 result = Rodrigues(Vec3::X, PI / 2.0f, Vec3::Z);
    TEST_CHECK(Cmpf(result.x, 0.0f));
    TEST_CHECK(Cmpf(result.y, 1.0f));
    TEST_CHECK(Cmpf(result.z, 0.0f));
}

TEST(Vec3, RodriguesFullRotation) {
    // Rotate by 2*PI should return the same vector
    Vec3 v(1.0f, 2.0f, 3.0f);
    Vec3 result = Rodrigues(v, 2.0f * PI, Vec3::Y);
    TEST_CHECK(Cmpf(result.x, v.x));
    TEST_CHECK(Cmpf(result.y, v.y));
    TEST_CHECK(Cmpf(result.z, v.z));
}

TEST(Vec3, ScalarLeftHandAdd) {
    Vec3 result = 1.0f + Vec3(1.0f, 2.0f, 3.0f);
    TEST_CHECK_EQUAL(result, Vec3(2.0f, 3.0f, 4.0f));
}

TEST(Vec3, ScalarLeftHandSub) {
    Vec3 result = 5.0f - Vec3(1.0f, 2.0f, 3.0f);
    TEST_CHECK_EQUAL(result, Vec3(4.0f, 3.0f, 2.0f));
}
