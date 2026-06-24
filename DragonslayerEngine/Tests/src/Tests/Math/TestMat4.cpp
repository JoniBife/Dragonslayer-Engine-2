#include "Framework/TestFramework.hpp"
#include "Math/MathAux.hpp"
#include "Math/Mat4.hpp"

TEST(Mat4, DefaultConstruction) {
    Mat4 m;
    for (int l = 0; l < 4; l++)
        for (int c = 0; c < 4; c++)
            TEST_CHECK(Cmpf(m.m[l][c], 0.0f));
}

TEST(Mat4, FillConstruction) {
    Mat4 m(5.0f);
    for (int l = 0; l < 4; l++)
        for (int c = 0; c < 4; c++)
            TEST_CHECK(Cmpf(m.m[l][c], 5.0f));
}

TEST(Mat4, ElementConstruction) {
    Mat4 m(1.0f,  2.0f,  3.0f,  4.0f,
           5.0f,  6.0f,  7.0f,  8.0f,
           9.0f,  10.0f, 11.0f, 12.0f,
           13.0f, 14.0f, 15.0f, 16.0f);
    TEST_CHECK(Cmpf(m.m[0][0], 1.0f));
    TEST_CHECK(Cmpf(m.m[0][3], 4.0f));
    TEST_CHECK(Cmpf(m.m[1][1], 6.0f));
    TEST_CHECK(Cmpf(m.m[2][2], 11.0f));
    TEST_CHECK(Cmpf(m.m[3][3], 16.0f));
}

TEST(Mat4, CopyConstruction) {
    Mat4 a = Mat4::IDENTITY;
    Mat4 b(a);
    TEST_CHECK_EQUAL(a, b);
}

TEST(Mat4, StaticZero) {
    for (int l = 0; l < 4; l++)
        for (int c = 0; c < 4; c++)
            TEST_CHECK(Cmpf(Mat4::ZERO.m[l][c], 0.0f));
}

TEST(Mat4, StaticIdentity) {
    for (int l = 0; l < 4; l++)
        for (int c = 0; c < 4; c++)
            TEST_CHECK(Cmpf(Mat4::IDENTITY.m[l][c], l == c ? 1.0f : 0.0f));
}

TEST(Mat4, ScalingUniform) {
    Mat4 s = Mat4::Scaling(2.0f);
    Vec3 result = s * Vec3(1.0f, 1.0f, 1.0f);
    TEST_CHECK_EQUAL(result, Vec3(2.0f, 2.0f, 2.0f));
}

TEST(Mat4, ScalingNonUniform) {
    Mat4 s = Mat4::Scaling(1.0f, 2.0f, 3.0f);
    Vec3 result = s * Vec3(1.0f, 1.0f, 1.0f);
    TEST_CHECK_EQUAL(result, Vec3(1.0f, 2.0f, 3.0f));
}

TEST(Mat4, ScalingFromVec3) {
    TEST_CHECK_EQUAL(Mat4::Scaling(Vec3(2.0f, 3.0f, 4.0f)), Mat4::Scaling(2.0f, 3.0f, 4.0f));
}

TEST(Mat4, TranslationFloats) {
    Mat4 t = Mat4::Translation(1.0f, 2.0f, 3.0f);
    Vec3 result = t * Vec3(0.0f, 0.0f, 0.0f);
    TEST_CHECK_EQUAL(result, Vec3(1.0f, 2.0f, 3.0f));
}

TEST(Mat4, TranslationFromVec3) {
    TEST_CHECK_EQUAL(Mat4::Translation(Vec3(5.0f, 6.0f, 7.0f)), Mat4::Translation(5.0f, 6.0f, 7.0f));
}

TEST(Mat4, TranslationMultiplyVec3) {
    Mat4 t = Mat4::Translation(10.0f, 20.0f, 30.0f);
    Vec3 result = t * Vec3(1.0f, 2.0f, 3.0f);
    TEST_CHECK_EQUAL(result, Vec3(11.0f, 22.0f, 33.0f));
}

TEST(Mat4, RotationXAxis) {
    // Rotate Y around X by PI/2 -> should give Z
    Mat4 r = Mat4::Rotation(PI / 2.0f, Vec3::X);
    Vec3 result = r * Vec3::Y;
    TEST_CHECK(Cmpf(result.x, 0.0f));
    TEST_CHECK(Cmpf(result.y, 0.0f));
    TEST_CHECK(Cmpf(result.z, 1.0f));
}

TEST(Mat4, RotationYAxis) {
    // Rotate Z around Y by PI/2 -> should give X
    Mat4 r = Mat4::Rotation(PI / 2.0f, Vec3::Y);
    Vec3 result = r * Vec3::Z;
    TEST_CHECK(Cmpf(result.x, 1.0f));
    TEST_CHECK(Cmpf(result.y, 0.0f));
    TEST_CHECK(Cmpf(result.z, 0.0f));
}

TEST(Mat4, RotationZAxis) {
    // Rotate X around Z by PI/2 -> should give Y
    Mat4 r = Mat4::Rotation(PI / 2.0f, Vec3::Z);
    Vec3 result = r * Vec3::X;
    TEST_CHECK(Cmpf(result.x, 0.0f));
    TEST_CHECK(Cmpf(result.y, 1.0f));
    TEST_CHECK(Cmpf(result.z, 0.0f));
}

TEST(Mat4, RotationFullCircle) {
    Vec3 v(1.0f, 2.0f, 3.0f);
    Mat4 r = Mat4::Rotation(2.0f * PI, Vec3::Y);
    Vec3 result = r * v;
    TEST_CHECK(Cmpf(result.x, v.x));
    TEST_CHECK(Cmpf(result.y, v.y));
    TEST_CHECK(Cmpf(result.z, v.z));
}

TEST(Mat4, RotationFromDir) {
    Mat4 r = Mat4::RotationFromDir(Vec3::Z);
    Vec3 fwd = r.GetForwardAxis();
    TEST_CHECK(Cmpf(fwd.x, 0.0f));
    TEST_CHECK(Cmpf(fwd.y, 0.0f));
    TEST_CHECK(Cmpf(fwd.z, 1.0f));
}

TEST(Mat4, AdditionMat) {
    Mat4 result = Mat4::IDENTITY + Mat4::IDENTITY;
    for (int l = 0; l < 4; l++)
        for (int c = 0; c < 4; c++)
            TEST_CHECK(Cmpf(result.m[l][c], l == c ? 2.0f : 0.0f));
}

TEST(Mat4, SubtractionMat) {
    Mat4 result = Mat4::IDENTITY - Mat4::IDENTITY;
    TEST_CHECK_EQUAL(result, Mat4::ZERO);
}

TEST(Mat4, MultiplyMatIdentity) {
    Mat4 a = Mat4::Translation(1.0f, 2.0f, 3.0f);
    TEST_CHECK_EQUAL(Mat4::IDENTITY * a, a);
    TEST_CHECK_EQUAL(a * Mat4::IDENTITY, a);
}

TEST(Mat4, MultiplyScalar) {
    Mat4 result = Mat4::IDENTITY * 3.0f;
    TEST_CHECK(Cmpf(result.m[0][0], 3.0f));
    TEST_CHECK(Cmpf(result.m[0][1], 0.0f));
}

TEST(Mat4, ScalarMultiplyMat) {
    Mat4 a = Mat4::IDENTITY;
    TEST_CHECK_EQUAL(2.0f * a, a * 2.0f);
}

TEST(Mat4, DivideScalar) {
    Mat4 a = Mat4::IDENTITY * 4.0f;
    TEST_CHECK_EQUAL(a / 4.0f, Mat4::IDENTITY);
}

TEST(Mat4, MultiplyVec4Identity) {
    Vec4 v(1.0f, 2.0f, 3.0f, 1.0f);
    TEST_CHECK_EQUAL(Mat4::IDENTITY * v, v);
}

TEST(Mat4, MultiplyVec3Identity) {
    Vec3 v(1.0f, 2.0f, 3.0f);
    TEST_CHECK_EQUAL(Mat4::IDENTITY * v, v);
}

TEST(Mat4, IndexOperator) {
    Mat4 a = Mat4::IDENTITY;
    TEST_CHECK(Cmpf(a[0][0], 1.0f));
    TEST_CHECK(Cmpf(a[0][1], 0.0f));
    a[0][1] = 5.0f;
    TEST_CHECK(Cmpf(a[0][1], 5.0f));
}

TEST(Mat4, CompoundAssignment) {
    Mat4 a = Mat4::IDENTITY;
    a += Mat4::ZERO;
    TEST_CHECK_EQUAL(a, Mat4::IDENTITY);
    a -= Mat4::ZERO;
    TEST_CHECK_EQUAL(a, Mat4::IDENTITY);
    a *= Mat4::IDENTITY;
    TEST_CHECK_EQUAL(a, Mat4::IDENTITY);
    a *= 1.0f;
    TEST_CHECK_EQUAL(a, Mat4::IDENTITY);
    a /= 1.0f;
    TEST_CHECK_EQUAL(a, Mat4::IDENTITY);
}

TEST(Mat4, Equality) {
    TEST_CHECK(Mat4::IDENTITY == Mat4::IDENTITY);
    TEST_CHECK_FALSE(Mat4::IDENTITY == Mat4::ZERO);
}

TEST(Mat4, Inequality) {
    TEST_CHECK(Mat4::IDENTITY != Mat4::ZERO);
    TEST_CHECK_FALSE(Mat4::IDENTITY != Mat4::IDENTITY);
}

TEST(Mat4, InverseIdentity) {
    Mat4 inv;
    TEST_REQUIRE(Mat4::IDENTITY.Inverse(inv));
    TEST_CHECK_EQUAL(inv, Mat4::IDENTITY);
}

TEST(Mat4, InverseTranslation) {
    Mat4 t = Mat4::Translation(1.0f, 2.0f, 3.0f);
    Mat4 inv;
    TEST_REQUIRE(t.Inverse(inv));
    TEST_CHECK_EQUAL(t * inv, Mat4::IDENTITY);
}

TEST(Mat4, InverseRotation) {
    Mat4 r = Mat4::Rotation(PI / 4.0f, Vec3::Y);
    Mat4 inv;
    TEST_REQUIRE(r.Inverse(inv));
    TEST_CHECK_EQUAL(r * inv, Mat4::IDENTITY);
}

TEST(Mat4, InverseSingular) {
    Mat4 inv;
    TEST_CHECK_FALSE(Mat4::ZERO.Inverse(inv));
}

TEST(Mat4, Transpose) {
    Mat4 t = Mat4::IDENTITY.Transpose();
    TEST_CHECK_EQUAL(t, Mat4::IDENTITY);
}

TEST(Mat4, TransposeGeneral) {
    Mat4 a = Mat4::Translation(1.0f, 2.0f, 3.0f);
    Mat4 t = a.Transpose();
    // Translation is in m[0][3], m[1][3], m[2][3] -> after transpose in m[3][0], m[3][1], m[3][2]
    TEST_CHECK(Cmpf(t.m[3][0], 1.0f));
    TEST_CHECK(Cmpf(t.m[3][1], 2.0f));
    TEST_CHECK(Cmpf(t.m[3][2], 3.0f));
}

TEST(Mat4, IsOrthogonalIdentity) {
    TEST_CHECK(Mat4::IDENTITY.IsOrthogonal());
}

TEST(Mat4, IsOrthogonalRotation) {
    TEST_CHECK(Mat4::Rotation(PI / 3.0f, Vec3::Y).IsOrthogonal());
}

TEST(Mat4, IsOrthogonalFalse) {
    TEST_CHECK_FALSE(Mat4::Scaling(2.0f).IsOrthogonal());
}

TEST(Mat4, ToMat3Identity) {
    Mat3 m3 = Mat4::IDENTITY.ToMat3();
    TEST_CHECK_EQUAL(m3, Mat3::IDENTITY);
}

TEST(Mat4, GetAxesIdentity) {
    TEST_CHECK_EQUAL(Mat4::IDENTITY.GetRightAxis(), Vec3::X);
    TEST_CHECK_EQUAL(Mat4::IDENTITY.GetUpAxis(), Vec3::Y);
    TEST_CHECK_EQUAL(Mat4::IDENTITY.GetForwardAxis(), Vec3::Z);
}

TEST(Mat4, DecomposeIdentity) {
    Vec3 scale, rotation, position;
    Mat4::IDENTITY.Decompose(scale, rotation, position);
    TEST_CHECK_EQUAL(scale, Vec3::ONE);
    TEST_CHECK_EQUAL(position, Vec3::ZERO);
    TEST_CHECK(Cmpf(rotation.x, 0.0f));
    TEST_CHECK(Cmpf(rotation.y, 0.0f));
    TEST_CHECK(Cmpf(rotation.z, 0.0f));
}

TEST(Mat4, DecomposeTranslation) {
    Vec3 scale, rotation, position;
    Mat4::Translation(5.0f, 10.0f, 15.0f).Decompose(scale, rotation, position);
    TEST_CHECK_EQUAL(position, Vec3(5.0f, 10.0f, 15.0f));
    TEST_CHECK_EQUAL(scale, Vec3::ONE);
}

TEST(Mat4, DecomposeScaling) {
    Vec3 scale, rotation, position;
    Mat4::Scaling(2.0f, 3.0f, 4.0f).Decompose(scale, rotation, position);
    TEST_CHECK(Cmpf(scale.x, 2.0f));
    TEST_CHECK(Cmpf(scale.y, 3.0f));
    TEST_CHECK(Cmpf(scale.z, 4.0f));
    TEST_CHECK_EQUAL(position, Vec3::ZERO);
}

TEST(Mat4, DecomposeScaleAndTranslation) {
    Vec3 scale, rotation, position;
    Mat4 m = Mat4::Translation(1.0f, 2.0f, 3.0f) * Mat4::Scaling(2.0f, 3.0f, 4.0f);
    m.Decompose(scale, rotation, position);
    TEST_CHECK(Cmpf(scale.x, 2.0f));
    TEST_CHECK(Cmpf(scale.y, 3.0f));
    TEST_CHECK(Cmpf(scale.z, 4.0f));
    TEST_CHECK_EQUAL(position, Vec3(1.0f, 2.0f, 3.0f));
}
