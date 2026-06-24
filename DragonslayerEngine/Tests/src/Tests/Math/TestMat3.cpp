#include "Framework/TestFramework.hpp"
#include "Math/MathAux.hpp"
#include "Math/Mat3.hpp"
#include "Math/Quat.hpp"

TEST(Mat3, DefaultConstruction) {
    Mat3 m;
    for (int l = 0; l < 3; l++)
        for (int c = 0; c < 3; c++)
            TEST_CHECK(Cmpf(m.m[l][c], 0.0f));
}

TEST(Mat3, FillConstruction) {
    Mat3 m(5.0f);
    for (int l = 0; l < 3; l++)
        for (int c = 0; c < 3; c++)
            TEST_CHECK(Cmpf(m.m[l][c], 5.0f));
}

TEST(Mat3, ElementConstruction) {
    Mat3 m(1.0f, 2.0f, 3.0f,
           4.0f, 5.0f, 6.0f,
           7.0f, 8.0f, 9.0f);
    TEST_CHECK(Cmpf(m.m[0][0], 1.0f));
    TEST_CHECK(Cmpf(m.m[0][2], 3.0f));
    TEST_CHECK(Cmpf(m.m[1][1], 5.0f));
    TEST_CHECK(Cmpf(m.m[2][0], 7.0f));
    TEST_CHECK(Cmpf(m.m[2][2], 9.0f));
}

TEST(Mat3, CopyConstruction) {
    Mat3 a(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    Mat3 b(a);
    TEST_CHECK_EQUAL(a, b);
}

TEST(Mat3, VectorConstruction) {
    // Mat3(forward, right, up) stores columns as: right=col0, up=col1, forward=col2
    Mat3 m(Vec3::Z, Vec3::X, Vec3::Y);
    TEST_CHECK_EQUAL(m.GetRightAxis(), Vec3::X);
    TEST_CHECK_EQUAL(m.GetUpAxis(), Vec3::Y);
    TEST_CHECK_EQUAL(m.GetForwardAxis(), Vec3::Z);
}

TEST(Mat3, StaticZero) {
    for (int l = 0; l < 3; l++)
        for (int c = 0; c < 3; c++)
            TEST_CHECK(Cmpf(Mat3::ZERO.m[l][c], 0.0f));
}

TEST(Mat3, StaticIdentity) {
    for (int l = 0; l < 3; l++)
        for (int c = 0; c < 3; c++)
            TEST_CHECK(Cmpf(Mat3::IDENTITY.m[l][c], l == c ? 1.0f : 0.0f));
}

TEST(Mat3, Dual) {
    // Dual of (1,0,0) = [0,0,0; 0,0,-1; 0,1,0]
    Mat3 d = Mat3::Dual(Vec3::X);
    TEST_CHECK(Cmpf(d.m[0][0], 0.0f));
    TEST_CHECK(Cmpf(d.m[0][1], 0.0f));
    TEST_CHECK(Cmpf(d.m[0][2], 0.0f));
    TEST_CHECK(Cmpf(d.m[1][0], 0.0f));
    TEST_CHECK(Cmpf(d.m[1][1], 0.0f));
    TEST_CHECK(Cmpf(d.m[1][2], -1.0f));
    TEST_CHECK(Cmpf(d.m[2][0], 0.0f));
    TEST_CHECK(Cmpf(d.m[2][1], 1.0f));
    TEST_CHECK(Cmpf(d.m[2][2], 0.0f));
}

TEST(Mat3, DualSkewSymmetric) {
    Vec3 v(1.0f, 2.0f, 3.0f);
    Mat3 d = Mat3::Dual(v);
    // Skew-symmetric: D^T = -D
    Mat3 negD = d * -1.0f;
    TEST_CHECK_EQUAL(d.Transpose(), negD);
}

TEST(Mat3, AdditionMat) {
    Mat3 a(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    Mat3 b(9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f);
    Mat3 result = a + b;
    TEST_CHECK_EQUAL(result, Mat3(10.0f));
}

TEST(Mat3, SubtractionMat) {
    Mat3 a(10.0f);
    Mat3 b(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    Mat3 result = a - b;
    TEST_CHECK_EQUAL(result, Mat3(9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f));
}

TEST(Mat3, MultiplyMatIdentity) {
    Mat3 a(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    TEST_CHECK_EQUAL(Mat3::IDENTITY * a, a);
    TEST_CHECK_EQUAL(a * Mat3::IDENTITY, a);
}

TEST(Mat3, MultiplyScalar) {
    Mat3 a(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    Mat3 result = a * 2.0f;
    TEST_CHECK_EQUAL(result, Mat3(2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 12.0f, 14.0f, 16.0f, 18.0f));
}

TEST(Mat3, ScalarMultiplyMat) {
    Mat3 a(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    TEST_CHECK_EQUAL(2.0f * a, a * 2.0f);
}

TEST(Mat3, DivideScalar) {
    Mat3 a(2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 12.0f, 14.0f, 16.0f, 18.0f);
    TEST_CHECK_EQUAL(a / 2.0f, Mat3(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f));
}

TEST(Mat3, MultiplyVec3Identity) {
    Vec3 v(1.0f, 2.0f, 3.0f);
    TEST_CHECK_EQUAL(Mat3::IDENTITY * v, v);
}

TEST(Mat3, MultiplyVec3) {
    // [1,2,3; 4,5,6; 7,8,9] * (1,0,0) = (1,4,7)
    Mat3 a(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    TEST_CHECK_EQUAL(a * Vec3::X, Vec3(1.0f, 4.0f, 7.0f));
}

TEST(Mat3, IndexOperator) {
    Mat3 a(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    TEST_CHECK(Cmpf(a[0][0], 1.0f));
    TEST_CHECK(Cmpf(a[1][1], 5.0f));
    TEST_CHECK(Cmpf(a[2][2], 9.0f));
}

TEST(Mat3, CompoundAssignment) {
    Mat3 a(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    Mat3 copy = a;
    a += Mat3::ZERO;
    TEST_CHECK_EQUAL(a, copy);
    a -= Mat3::ZERO;
    TEST_CHECK_EQUAL(a, copy);
    a *= Mat3::IDENTITY;
    TEST_CHECK_EQUAL(a, copy);
    a *= 1.0f;
    TEST_CHECK_EQUAL(a, copy);
    a /= 1.0f;
    TEST_CHECK_EQUAL(a, copy);
}

TEST(Mat3, Equality) {
    Mat3 a(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    Mat3 b(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    TEST_CHECK(a == b);
}

TEST(Mat3, Inequality) {
    TEST_CHECK(Mat3::IDENTITY != Mat3::ZERO);
}

TEST(Mat3, DeterminantIdentity) {
    TEST_CHECK(Cmpf(Mat3::IDENTITY.Determinant(), 1.0f));
}

TEST(Mat3, DeterminantZero) {
    TEST_CHECK(Cmpf(Mat3::ZERO.Determinant(), 0.0f));
}

TEST(Mat3, DeterminantKnown) {
    // det([1,2,3; 0,1,4; 5,6,0]) = 1(0-24) - 2(0-20) + 3(0-5) = -24+40-15 = 1
    Mat3 a(1.0f, 2.0f, 3.0f,
           0.0f, 1.0f, 4.0f,
           5.0f, 6.0f, 0.0f);
    TEST_CHECK(Cmpf(a.Determinant(), 1.0f));
}

TEST(Mat3, TransposeIdentity) {
    TEST_CHECK_EQUAL(Mat3::IDENTITY.Transpose(), Mat3::IDENTITY);
}

TEST(Mat3, Transpose) {
    Mat3 a(1.0f, 2.0f, 3.0f,
           4.0f, 5.0f, 6.0f,
           7.0f, 8.0f, 9.0f);
    Mat3 t = a.Transpose();
    TEST_CHECK(Cmpf(t.m[0][1], 4.0f));
    TEST_CHECK(Cmpf(t.m[1][0], 2.0f));
    TEST_CHECK(Cmpf(t.m[0][2], 7.0f));
    TEST_CHECK(Cmpf(t.m[2][0], 3.0f));
}

TEST(Mat3, InverseIdentity) {
    Mat3 inv;
    TEST_REQUIRE(Mat3::IDENTITY.Inverse(inv));
    TEST_CHECK_EQUAL(inv, Mat3::IDENTITY);
}

TEST(Mat3, InverseKnown) {
    Mat3 a(1.0f, 2.0f, 3.0f,
           0.0f, 1.0f, 4.0f,
           5.0f, 6.0f, 0.0f);
    Mat3 inv;
    TEST_REQUIRE(a.Inverse(inv));
    TEST_CHECK_EQUAL(a * inv, Mat3::IDENTITY);
}

TEST(Mat3, InverseSingular) {
    Mat3 inv;
    TEST_CHECK_FALSE(Mat3::ZERO.Inverse(inv));
}

TEST(Mat3, IsOrthogonalIdentity) {
    TEST_CHECK(Mat3::IDENTITY.IsOrthogonal());
}

TEST(Mat3, IsOrthogonalFalse) {
    TEST_CHECK_FALSE(Mat3(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f).IsOrthogonal());
}

TEST(Mat3, GetAxesIdentity) {
    TEST_CHECK_EQUAL(Mat3::IDENTITY.GetRightAxis(), Vec3::X);
    TEST_CHECK_EQUAL(Mat3::IDENTITY.GetUpAxis(), Vec3::Y);
    TEST_CHECK_EQUAL(Mat3::IDENTITY.GetForwardAxis(), Vec3::Z);
}

TEST(Mat3, ToQuaternionIdentity) {
    Quat q = Mat3::IDENTITY.ToQuaternion();
    TEST_CHECK_EQUAL(q, Quat::IDENTITY);
}
