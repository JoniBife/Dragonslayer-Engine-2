#include "Framework/TestFramework.hpp"
#include "Math/MathAux.hpp"
#include "Math/Vec4.hpp"

TEST(Vec4, DefaultConstruction) {
    Vec4 v;
    TEST_CHECK(Cmpf(v.x, 0.0f));
    TEST_CHECK(Cmpf(v.y, 0.0f));
    TEST_CHECK(Cmpf(v.z, 0.0f));
    TEST_CHECK(Cmpf(v.w, 0.0f));
}

TEST(Vec4, ScalarConstruction) {
    Vec4 v(5.0f);
    TEST_CHECK(Cmpf(v.x, 5.0f));
    TEST_CHECK(Cmpf(v.y, 5.0f));
    TEST_CHECK(Cmpf(v.z, 5.0f));
    TEST_CHECK(Cmpf(v.w, 5.0f));
}

TEST(Vec4, ComponentConstruction) {
    Vec4 v(1.0f, 2.0f, 3.0f, 4.0f);
    TEST_CHECK(Cmpf(v.x, 1.0f));
    TEST_CHECK(Cmpf(v.y, 2.0f));
    TEST_CHECK(Cmpf(v.z, 3.0f));
    TEST_CHECK(Cmpf(v.w, 4.0f));
}

TEST(Vec4, StaticConstants) {
    TEST_CHECK_EQUAL(Vec4::X, Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    TEST_CHECK_EQUAL(Vec4::Y, Vec4(0.0f, 1.0f, 0.0f, 1.0f));
    TEST_CHECK_EQUAL(Vec4::Z, Vec4(0.0f, 0.0f, 1.0f, 1.0f));
    TEST_CHECK_EQUAL(Vec4::ONE, Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    TEST_CHECK_EQUAL(Vec4::ZERO, Vec4(0.0f, 0.0f, 0.0f, 0.0f));
}

TEST(Vec4, AdditionVec) {
    Vec4 result = Vec4(1.0f, 2.0f, 3.0f, 4.0f) + Vec4(5.0f, 6.0f, 7.0f, 8.0f);
    TEST_CHECK_EQUAL(result, Vec4(6.0f, 8.0f, 10.0f, 12.0f));
}

TEST(Vec4, SubtractionVec) {
    Vec4 result = Vec4(5.0f, 6.0f, 7.0f, 8.0f) - Vec4(1.0f, 2.0f, 3.0f, 4.0f);
    TEST_CHECK_EQUAL(result, Vec4(4.0f, 4.0f, 4.0f, 4.0f));
}

TEST(Vec4, MultiplyScalar) {
    Vec4 result = Vec4(1.0f, 2.0f, 3.0f, 4.0f) * 2.0f;
    TEST_CHECK_EQUAL(result, Vec4(2.0f, 4.0f, 6.0f, 8.0f));
}

TEST(Vec4, ScalarMultiplyVec) {
    Vec4 result = 2.0f * Vec4(1.0f, 2.0f, 3.0f, 4.0f);
    TEST_CHECK_EQUAL(result, Vec4(2.0f, 4.0f, 6.0f, 8.0f));
}

TEST(Vec4, DivideScalar) {
    Vec4 result = Vec4(2.0f, 4.0f, 6.0f, 8.0f) / 2.0f;
    TEST_CHECK_EQUAL(result, Vec4(1.0f, 2.0f, 3.0f, 4.0f));
}

TEST(Vec4, AddScalar) {
    Vec4 result = Vec4(1.0f, 2.0f, 3.0f, 4.0f) + 1.0f;
    TEST_CHECK_EQUAL(result, Vec4(2.0f, 3.0f, 4.0f, 5.0f));
}

TEST(Vec4, SubtractScalar) {
    Vec4 result = Vec4(5.0f, 6.0f, 7.0f, 8.0f) - 1.0f;
    TEST_CHECK_EQUAL(result, Vec4(4.0f, 5.0f, 6.0f, 7.0f));
}

TEST(Vec4, UnaryNegation) {
    Vec4 result = -Vec4(1.0f, -2.0f, 3.0f, -4.0f);
    TEST_CHECK_EQUAL(result, Vec4(-1.0f, 2.0f, -3.0f, 4.0f));
}

TEST(Vec4, PlusEqualsVec) {
    Vec4 v(1.0f, 2.0f, 3.0f, 4.0f);
    v += Vec4(5.0f, 6.0f, 7.0f, 8.0f);
    TEST_CHECK_EQUAL(v, Vec4(6.0f, 8.0f, 10.0f, 12.0f));
}

TEST(Vec4, MinusEqualsVec) {
    Vec4 v(5.0f, 6.0f, 7.0f, 8.0f);
    v -= Vec4(1.0f, 2.0f, 3.0f, 4.0f);
    TEST_CHECK_EQUAL(v, Vec4(4.0f, 4.0f, 4.0f, 4.0f));
}

TEST(Vec4, PlusEqualsScalar) {
    Vec4 v(1.0f, 2.0f, 3.0f, 4.0f);
    v += 1.0f;
    TEST_CHECK_EQUAL(v, Vec4(2.0f, 3.0f, 4.0f, 5.0f));
}

TEST(Vec4, MinusEqualsScalar) {
    Vec4 v(5.0f, 6.0f, 7.0f, 8.0f);
    v -= 1.0f;
    TEST_CHECK_EQUAL(v, Vec4(4.0f, 5.0f, 6.0f, 7.0f));
}

TEST(Vec4, TimesEqualsScalar) {
    Vec4 v(1.0f, 2.0f, 3.0f, 4.0f);
    v *= 2.0f;
    TEST_CHECK_EQUAL(v, Vec4(2.0f, 4.0f, 6.0f, 8.0f));
}

TEST(Vec4, DivEqualsScalar) {
    Vec4 v(2.0f, 4.0f, 6.0f, 8.0f);
    v /= 2.0f;
    TEST_CHECK_EQUAL(v, Vec4(1.0f, 2.0f, 3.0f, 4.0f));
}

TEST(Vec4, Equality) {
    TEST_CHECK(Vec4(1.0f, 2.0f, 3.0f, 4.0f) == Vec4(1.0f, 2.0f, 3.0f, 4.0f));
    TEST_CHECK_FALSE(Vec4(1.0f, 2.0f, 3.0f, 4.0f) == Vec4(5.0f, 6.0f, 7.0f, 8.0f));
}

TEST(Vec4, Inequality) {
    TEST_CHECK(Vec4(1.0f, 2.0f, 3.0f, 4.0f) != Vec4(5.0f, 6.0f, 7.0f, 8.0f));
    TEST_CHECK_FALSE(Vec4(1.0f, 2.0f, 3.0f, 4.0f) != Vec4(1.0f, 2.0f, 3.0f, 4.0f));
}

TEST(Vec4, Magnitude) {
    // sqrt(1 + 4 + 4 + 0) = sqrt(9) = 3
    TEST_CHECK(Cmpf(Vec4(1.0f, 2.0f, 2.0f, 0.0f).Magnitude(), 3.0f));
}

TEST(Vec4, SqrMagnitude) {
    TEST_CHECK(Cmpf(Vec4(1.0f, 2.0f, 2.0f, 0.0f).SqrMagnitude(), 9.0f));
}

TEST(Vec4, Normalize) {
    Vec4 n = Vec4(1.0f, 2.0f, 2.0f, 0.0f).Normalize();
    TEST_CHECK(Cmpf(n.Magnitude(), 1.0f));
    TEST_CHECK(Cmpf(n.x, 1.0f / 3.0f));
    TEST_CHECK(Cmpf(n.y, 2.0f / 3.0f));
    TEST_CHECK(Cmpf(n.z, 2.0f / 3.0f));
    TEST_CHECK(Cmpf(n.w, 0.0f));
}

TEST(Vec4, Dot) {
    // 1*5 + 2*6 + 3*7 + 4*8 = 5 + 12 + 21 + 32 = 70
    TEST_CHECK(Cmpf(Dot(Vec4(1.0f, 2.0f, 3.0f, 4.0f), Vec4(5.0f, 6.0f, 7.0f, 8.0f)), 70.0f));
}

TEST(Vec4, DotOrthogonal) {
    TEST_CHECK(Cmpf(Dot(Vec4(1.0f, 0.0f, 0.0f, 0.0f), Vec4(0.0f, 1.0f, 0.0f, 0.0f)), 0.0f));
}

TEST(Vec4, ScalarLeftHandAdd) {
    Vec4 result = 1.0f + Vec4(1.0f, 2.0f, 3.0f, 4.0f);
    TEST_CHECK_EQUAL(result, Vec4(2.0f, 3.0f, 4.0f, 5.0f));
}

TEST(Vec4, ScalarLeftHandSub) {
    Vec4 result = 10.0f - Vec4(1.0f, 2.0f, 3.0f, 4.0f);
    TEST_CHECK_EQUAL(result, Vec4(9.0f, 8.0f, 7.0f, 6.0f));
}
