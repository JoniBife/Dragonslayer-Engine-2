#include "Framework/TestFramework.hpp"
#include "Math/MathAux.hpp"
#include "Math/Vec2.hpp"

TEST(Vec2, DefaultConstruction) {
    Vec2 v;
    TEST_CHECK(Cmpf(v.x, 0.0f));
    TEST_CHECK(Cmpf(v.y, 0.0f));
}

TEST(Vec2, ScalarConstruction) {
    Vec2 v(5.0f);
    TEST_CHECK(Cmpf(v.x, 5.0f));
    TEST_CHECK(Cmpf(v.y, 5.0f));
}

TEST(Vec2, ComponentConstruction) {
    Vec2 v(3.0f, 4.0f);
    TEST_CHECK(Cmpf(v.x, 3.0f));
    TEST_CHECK(Cmpf(v.y, 4.0f));
}

TEST(Vec2, StaticConstants) {
    TEST_CHECK_EQUAL(Vec2::X, Vec2(1.0f, 0.0f));
    TEST_CHECK_EQUAL(Vec2::Y, Vec2(0.0f, 1.0f));
    TEST_CHECK_EQUAL(Vec2::ONE, Vec2(1.0f, 1.0f));
}

TEST(Vec2, AdditionVec) {
    Vec2 result = Vec2(1.0f, 2.0f) + Vec2(3.0f, 4.0f);
    TEST_CHECK_EQUAL(result, Vec2(4.0f, 6.0f));
}

TEST(Vec2, SubtractionVec) {
    Vec2 result = Vec2(5.0f, 6.0f) - Vec2(1.0f, 2.0f);
    TEST_CHECK_EQUAL(result, Vec2(4.0f, 4.0f));
}

TEST(Vec2, MultiplyScalar) {
    Vec2 result = Vec2(2.0f, 3.0f) * 2.0f;
    TEST_CHECK_EQUAL(result, Vec2(4.0f, 6.0f));
}

TEST(Vec2, ScalarMultiplyVec) {
    Vec2 result = 2.0f * Vec2(2.0f, 3.0f);
    TEST_CHECK_EQUAL(result, Vec2(4.0f, 6.0f));
}

TEST(Vec2, DivideScalar) {
    Vec2 result = Vec2(4.0f, 6.0f) / 2.0f;
    TEST_CHECK_EQUAL(result, Vec2(2.0f, 3.0f));
}

TEST(Vec2, AddScalar) {
    Vec2 result = Vec2(1.0f, 2.0f) + 3.0f;
    TEST_CHECK_EQUAL(result, Vec2(4.0f, 5.0f));
}

TEST(Vec2, SubtractScalar) {
    Vec2 result = Vec2(5.0f, 6.0f) - 1.0f;
    TEST_CHECK_EQUAL(result, Vec2(4.0f, 5.0f));
}

TEST(Vec2, UnaryNegation) {
    Vec2 result = -Vec2(1.0f, -2.0f);
    TEST_CHECK_EQUAL(result, Vec2(-1.0f, 2.0f));
}

TEST(Vec2, PlusEqualsVec) {
    Vec2 v(1.0f, 2.0f);
    v += Vec2(3.0f, 4.0f);
    TEST_CHECK_EQUAL(v, Vec2(4.0f, 6.0f));
}

TEST(Vec2, MinusEqualsVec) {
    Vec2 v(5.0f, 6.0f);
    v -= Vec2(1.0f, 2.0f);
    TEST_CHECK_EQUAL(v, Vec2(4.0f, 4.0f));
}

TEST(Vec2, PlusEqualsScalar) {
    Vec2 v(1.0f, 2.0f);
    v += 3.0f;
    TEST_CHECK_EQUAL(v, Vec2(4.0f, 5.0f));
}

TEST(Vec2, MinusEqualsScalar) {
    Vec2 v(5.0f, 6.0f);
    v -= 1.0f;
    TEST_CHECK_EQUAL(v, Vec2(4.0f, 5.0f));
}

TEST(Vec2, TimesEqualsScalar) {
    Vec2 v(2.0f, 3.0f);
    v *= 3.0f;
    TEST_CHECK_EQUAL(v, Vec2(6.0f, 9.0f));
}

TEST(Vec2, DivEqualsScalar) {
    Vec2 v(6.0f, 9.0f);
    v /= 3.0f;
    TEST_CHECK_EQUAL(v, Vec2(2.0f, 3.0f));
}

TEST(Vec2, Equality) {
    TEST_CHECK(Vec2(1.0f, 2.0f) == Vec2(1.0f, 2.0f));
    TEST_CHECK_FALSE(Vec2(1.0f, 2.0f) == Vec2(3.0f, 4.0f));
}

TEST(Vec2, Inequality) {
    TEST_CHECK(Vec2(1.0f, 2.0f) != Vec2(3.0f, 4.0f));
    TEST_CHECK_FALSE(Vec2(1.0f, 2.0f) != Vec2(1.0f, 2.0f));
}

TEST(Vec2, Magnitude) {
    TEST_CHECK(Cmpf(Vec2(3.0f, 4.0f).Magnitude(), 5.0f));
}

TEST(Vec2, SqrMagnitude) {
    TEST_CHECK(Cmpf(Vec2(3.0f, 4.0f).SqrMagnitude(), 25.0f));
}

TEST(Vec2, MagnitudeUnit) {
    TEST_CHECK(Cmpf(Vec2::X.Magnitude(), 1.0f));
    TEST_CHECK(Cmpf(Vec2::Y.Magnitude(), 1.0f));
}

TEST(Vec2, Normalize) {
    Vec2 n = Vec2(3.0f, 4.0f).Normalize();
    TEST_CHECK(Cmpf(n.Magnitude(), 1.0f));
    TEST_CHECK(Cmpf(n.x, 3.0f / 5.0f));
    TEST_CHECK(Cmpf(n.y, 4.0f / 5.0f));
}

TEST(Vec2, NormalizeUnit) {
    Vec2 n = Vec2::X.Normalize();
    TEST_CHECK_EQUAL(n, Vec2(1.0f, 0.0f));
}

TEST(Vec2, Clamp) {
    Vec2 result = Vec2(5.0f, -3.0f).Clamp(-1.0f, 2.0f);
    TEST_CHECK(Cmpf(result.x, 2.0f));
    TEST_CHECK(Cmpf(result.y, -1.0f));
}

TEST(Vec2, ClampWithinRange) {
    Vec2 result = Vec2(0.5f, 1.5f).Clamp(-1.0f, 2.0f);
    TEST_CHECK(Cmpf(result.x, 0.5f));
    TEST_CHECK(Cmpf(result.y, 1.5f));
}

TEST(Vec2, DotPerpendicular) {
    TEST_CHECK(Cmpf(Dot(Vec2::X, Vec2::Y), 0.0f));
}

TEST(Vec2, DotParallel) {
    TEST_CHECK(Cmpf(Dot(Vec2::X, Vec2::X), 1.0f));
}

TEST(Vec2, DotGeneral) {
    TEST_CHECK(Cmpf(Dot(Vec2(1.0f, 2.0f), Vec2(3.0f, 4.0f)), 11.0f));
}

TEST(Vec2, ToVec3) {
    Vec3 result = Vec2(1.0f, 2.0f).ToVec3();
    TEST_CHECK(Cmpf(result.x, 1.0f));
    TEST_CHECK(Cmpf(result.y, 2.0f));
    TEST_CHECK(Cmpf(result.z, 0.0f));
}

TEST(Vec2, ToVec4) {
    Vec4 result = Vec2(1.0f, 2.0f).ToVec4();
    TEST_CHECK(Cmpf(result.x, 1.0f));
    TEST_CHECK(Cmpf(result.y, 2.0f));
    TEST_CHECK(Cmpf(result.z, 0.0f));
    TEST_CHECK(Cmpf(result.w, 0.0f));
}

TEST(Vec2, ScalarLeftHandAdd) {
    Vec2 result = 3.0f + Vec2(1.0f, 2.0f);
    TEST_CHECK_EQUAL(result, Vec2(4.0f, 5.0f));
}

TEST(Vec2, ScalarLeftHandSub) {
    Vec2 result = 5.0f - Vec2(1.0f, 2.0f);
    TEST_CHECK_EQUAL(result, Vec2(4.0f, 3.0f));
}
