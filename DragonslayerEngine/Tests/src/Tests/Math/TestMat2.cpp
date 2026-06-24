#include "Framework/TestFramework.hpp"
#include "Math/MathAux.hpp"
#include "Math/Mat2.hpp"

TEST(Mat2, DefaultConstruction) {
    Mat2 m;
    TEST_CHECK(Cmpf(m.m[0][0], 0.0f));
    TEST_CHECK(Cmpf(m.m[0][1], 0.0f));
    TEST_CHECK(Cmpf(m.m[1][0], 0.0f));
    TEST_CHECK(Cmpf(m.m[1][1], 0.0f));
}

TEST(Mat2, FillConstruction) {
    Mat2 m(5.0f);
    TEST_CHECK(Cmpf(m.m[0][0], 5.0f));
    TEST_CHECK(Cmpf(m.m[0][1], 5.0f));
    TEST_CHECK(Cmpf(m.m[1][0], 5.0f));
    TEST_CHECK(Cmpf(m.m[1][1], 5.0f));
}

TEST(Mat2, ElementConstruction) {
    Mat2 m(1.0f, 2.0f, 3.0f, 4.0f);
    TEST_CHECK(Cmpf(m.m[0][0], 1.0f));
    TEST_CHECK(Cmpf(m.m[0][1], 2.0f));
    TEST_CHECK(Cmpf(m.m[1][0], 3.0f));
    TEST_CHECK(Cmpf(m.m[1][1], 4.0f));
}

TEST(Mat2, CopyConstruction) {
    Mat2 a(1.0f, 2.0f, 3.0f, 4.0f);
    Mat2 b(a);
    TEST_CHECK_EQUAL(a, b);
}

TEST(Mat2, StaticIdentity) {
    Mat2 id = Mat2::IDENTITY;
    TEST_CHECK(Cmpf(id.m[0][0], 1.0f));
    TEST_CHECK(Cmpf(id.m[0][1], 0.0f));
    TEST_CHECK(Cmpf(id.m[1][0], 0.0f));
    TEST_CHECK(Cmpf(id.m[1][1], 1.0f));
}

TEST(Mat2, AdditionMat) {
    Mat2 a(1.0f, 2.0f, 3.0f, 4.0f);
    Mat2 b(5.0f, 6.0f, 7.0f, 8.0f);
    Mat2 result = a + b;
    TEST_CHECK_EQUAL(result, Mat2(6.0f, 8.0f, 10.0f, 12.0f));
}

TEST(Mat2, SubtractionMat) {
    Mat2 a(5.0f, 6.0f, 7.0f, 8.0f);
    Mat2 b(1.0f, 2.0f, 3.0f, 4.0f);
    Mat2 result = a - b;
    TEST_CHECK_EQUAL(result, Mat2(4.0f, 4.0f, 4.0f, 4.0f));
}

TEST(Mat2, MultiplyMatIdentity) {
    Mat2 a(1.0f, 2.0f, 3.0f, 4.0f);
    Mat2 result = Mat2::IDENTITY * a;
    TEST_CHECK_EQUAL(result, a);
}

TEST(Mat2, MultiplyMat) {
    // [1,2; 3,4] * [5,6; 7,8] = [1*5+2*7, 1*6+2*8; 3*5+4*7, 3*6+4*8] = [19,22; 43,50]
    Mat2 a(1.0f, 2.0f, 3.0f, 4.0f);
    Mat2 b(5.0f, 6.0f, 7.0f, 8.0f);
    Mat2 result = a * b;
    TEST_CHECK_EQUAL(result, Mat2(19.0f, 22.0f, 43.0f, 50.0f));
}

TEST(Mat2, MultiplyScalar) {
    Mat2 a(1.0f, 2.0f, 3.0f, 4.0f);
    Mat2 result = a * 2.0f;
    TEST_CHECK_EQUAL(result, Mat2(2.0f, 4.0f, 6.0f, 8.0f));
}

TEST(Mat2, ScalarMultiplyMat) {
    Mat2 a(1.0f, 2.0f, 3.0f, 4.0f);
    Mat2 result = 2.0f * a;
    TEST_CHECK_EQUAL(result, a * 2.0f);
}

TEST(Mat2, DivideScalar) {
    Mat2 a(2.0f, 4.0f, 6.0f, 8.0f);
    Mat2 result = a / 2.0f;
    TEST_CHECK_EQUAL(result, Mat2(1.0f, 2.0f, 3.0f, 4.0f));
}

TEST(Mat2, AddScalar) {
    Mat2 a(1.0f, 2.0f, 3.0f, 4.0f);
    Mat2 result = a + 1.0f;
    TEST_CHECK_EQUAL(result, Mat2(2.0f, 3.0f, 4.0f, 5.0f));
}

TEST(Mat2, SubtractScalar) {
    Mat2 a(5.0f, 6.0f, 7.0f, 8.0f);
    Mat2 result = a - 1.0f;
    TEST_CHECK_EQUAL(result, Mat2(4.0f, 5.0f, 6.0f, 7.0f));
}

TEST(Mat2, MultiplyVec2) {
    Mat2 id = Mat2::IDENTITY;
    Vec2 v(3.0f, 4.0f);
    TEST_CHECK_EQUAL(id * v, v);
}

TEST(Mat2, MultiplyVec2General) {
    // [1,2; 3,4] * (5,6) = (1*5+2*6, 3*5+4*6) = (17, 39)
    Mat2 a(1.0f, 2.0f, 3.0f, 4.0f);
    Vec2 result = a * Vec2(5.0f, 6.0f);
    TEST_CHECK_EQUAL(result, Vec2(17.0f, 39.0f));
}

TEST(Mat2, IndexOperator) {
    Mat2 a(1.0f, 2.0f, 3.0f, 4.0f);
    TEST_CHECK(Cmpf(a[0][0], 1.0f));
    TEST_CHECK(Cmpf(a[0][1], 2.0f));
    TEST_CHECK(Cmpf(a[1][0], 3.0f));
    TEST_CHECK(Cmpf(a[1][1], 4.0f));
}

TEST(Mat2, IndexOperatorWrite) {
    Mat2 a;
    a[0][0] = 5.0f;
    a[1][1] = 10.0f;
    TEST_CHECK(Cmpf(a.m[0][0], 5.0f));
    TEST_CHECK(Cmpf(a.m[1][1], 10.0f));
}

TEST(Mat2, PlusEqualsMat) {
    Mat2 a(1.0f, 2.0f, 3.0f, 4.0f);
    a += Mat2(1.0f, 1.0f, 1.0f, 1.0f);
    TEST_CHECK_EQUAL(a, Mat2(2.0f, 3.0f, 4.0f, 5.0f));
}

TEST(Mat2, MinusEqualsMat) {
    Mat2 a(5.0f, 6.0f, 7.0f, 8.0f);
    a -= Mat2(1.0f, 2.0f, 3.0f, 4.0f);
    TEST_CHECK_EQUAL(a, Mat2(4.0f, 4.0f, 4.0f, 4.0f));
}

TEST(Mat2, TimesEqualsMat) {
    Mat2 a(1.0f, 2.0f, 3.0f, 4.0f);
    a *= Mat2::IDENTITY;
    TEST_CHECK_EQUAL(a, Mat2(1.0f, 2.0f, 3.0f, 4.0f));
}

TEST(Mat2, TimesEqualsScalar) {
    Mat2 a(1.0f, 2.0f, 3.0f, 4.0f);
    a *= 2.0f;
    TEST_CHECK_EQUAL(a, Mat2(2.0f, 4.0f, 6.0f, 8.0f));
}

TEST(Mat2, DivEqualsScalar) {
    Mat2 a(2.0f, 4.0f, 6.0f, 8.0f);
    a /= 2.0f;
    TEST_CHECK_EQUAL(a, Mat2(1.0f, 2.0f, 3.0f, 4.0f));
}

TEST(Mat2, Equality) {
    TEST_CHECK(Mat2(1.0f, 2.0f, 3.0f, 4.0f) == Mat2(1.0f, 2.0f, 3.0f, 4.0f));
    TEST_CHECK_FALSE(Mat2(1.0f, 2.0f, 3.0f, 4.0f) == Mat2(5.0f, 6.0f, 7.0f, 8.0f));
}

TEST(Mat2, Inequality) {
    TEST_CHECK(Mat2(1.0f, 2.0f, 3.0f, 4.0f) != Mat2(5.0f, 6.0f, 7.0f, 8.0f));
    TEST_CHECK_FALSE(Mat2(1.0f, 2.0f, 3.0f, 4.0f) != Mat2(1.0f, 2.0f, 3.0f, 4.0f));
}

TEST(Mat2, Transpose) {
    Mat2 a(1.0f, 2.0f, 3.0f, 4.0f);
    Mat2 t = a.Transpose();
    TEST_CHECK_EQUAL(t, Mat2(1.0f, 3.0f, 2.0f, 4.0f));
}

TEST(Mat2, TransposeIdentity) {
    TEST_CHECK_EQUAL(Mat2::IDENTITY.Transpose(), Mat2::IDENTITY);
}

TEST(Mat2, DeterminantIdentity) {
    TEST_CHECK(Cmpf(Mat2::IDENTITY.Determinant(), 1.0f));
}

TEST(Mat2, DeterminantKnown) {
    // det([1,2; 3,4]) = 1*4 - 2*3 = -2
    TEST_CHECK(Cmpf(Mat2(1.0f, 2.0f, 3.0f, 4.0f).Determinant(), -2.0f));
}

TEST(Mat2, DeterminantSingular) {
    // [1,2; 2,4] -> det = 4 - 4 = 0
    TEST_CHECK(Cmpf(Mat2(1.0f, 2.0f, 2.0f, 4.0f).Determinant(), 0.0f));
}

TEST(Mat2, InverseIdentity) {
    Mat2 inv;
    TEST_REQUIRE(Mat2::IDENTITY.Inverse(inv));
    TEST_CHECK_EQUAL(inv, Mat2::IDENTITY);
}

TEST(Mat2, InverseKnown) {
    Mat2 a(1.0f, 2.0f, 3.0f, 4.0f);
    Mat2 inv;
    TEST_REQUIRE(a.Inverse(inv));
    TEST_CHECK_EQUAL(a * inv, Mat2::IDENTITY);
}

TEST(Mat2, InverseSingular) {
    Mat2 a(1.0f, 2.0f, 2.0f, 4.0f);
    Mat2 inv;
    TEST_CHECK_FALSE(a.Inverse(inv));
}

TEST(Mat2, IsOrthogonalIdentity) {
    TEST_CHECK(Mat2::IDENTITY.IsOrthogonal());
}

TEST(Mat2, IsOrthogonalFalse) {
    TEST_CHECK_FALSE(Mat2(1.0f, 2.0f, 3.0f, 4.0f).IsOrthogonal());
}
