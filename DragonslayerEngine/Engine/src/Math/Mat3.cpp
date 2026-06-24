
#include <cmath>

#include "Math/Mat3.hpp"

#include "Core/Assert.hpp"
#include "Math/MathAux.hpp"
#include "Math/Quat.hpp"

Mat3::Mat3() : Mat3(0) {}

Mat3::Mat3(float fill) : Mat3(fill, fill, fill, fill, fill, fill, fill, fill, fill) {}

Mat3::Mat3(float l1c1, float l1c2, float l1c3,
           float l2c1, float l2c2, float l2c3,
           float l3c1, float l3c2, float l3c3) {

	m[0][0] = l1c1;
	m[0][1] = l1c2;
	m[0][2] = l1c3;
	m[1][0] = l2c1;
	m[1][1] = l2c2;
	m[1][2] = l2c3;
	m[2][0] = l3c1;
	m[2][1] = l3c2;
	m[2][2] = l3c3;
}

Mat3::Mat3(const Mat3& other) {
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			m[l][c] = other.m[l][c];
		}
	}
}

Mat3::Mat3(const Vec3& forward, const Vec3& right, const Vec3& up) {
	m[0][0] = right.x;
	m[0][1] = up.x;
	m[0][2] = forward.x;
	m[1][0] = right.y;
	m[1][1] = up.y;
	m[1][2] = forward.y;
	m[2][0] = right.z;
	m[2][1] = up.z;
	m[2][2] = forward.z;
}

float Mat3::Determinant() const {
	return	m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
			m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
			m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

Mat3 Mat3::ZERO = { 0, 0, 0,
					0, 0, 0,
					0, 0, 0 };

Mat3 Mat3::IDENTITY = { 1, 0, 0,
						0, 1, 0,
						0, 0, 1 };

Mat3 Mat3::Dual(const Vec3& v) {
	return {  0 , -v.z,  v.y,
			  v.z ,  0  , -v.x,
			 -v.y,  v.x,   0
	};
}

Mat3 Mat3::RandomMatrix(float min, float max) {
	Mat3 randomM;
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			randomM.m[l][c] = RandomFloat(min, max);
		}
	}
	return randomM;
}
Mat3 Mat3::Scaling(const Vec3& scale) {
    return {
        scale.x, 0.f, 0.f,
        0.f, scale.y, 0.f,
        0.f, 0.f, scale.z
    };
}

Mat3& Mat3::operator=(const Mat3& other) {
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			m[l][c] = other.m[l][c];
		}
	}
	return *this;
}

Mat3& Mat3::operator+=(const Mat3& other) {
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			m[l][c] += other.m[l][c];
		}
	}
	return *this;
}

Mat3& Mat3::operator-=(const Mat3& other) {
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			m[l][c] -= other.m[l][c];
		}
	}
	return *this;
}

Mat3& Mat3::operator*=(const Mat3& other) {
	Mat3 prod;
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			prod.m[l][c] =  m[l][0] * other.m[0][c] +
							m[l][1] * other.m[1][c] +
							m[l][2] * other.m[2][c];
		}
	}
	*this = prod;
	return *this;
}

Mat3& Mat3::operator*=(float s) {
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			m[l][c] *= s;
		}
	}
	return *this;
}

Mat3& Mat3::operator/=(float s) {
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			m[l][c] /= s;
		}
	}
	return *this;
}

Mat3& Mat3::operator+=(float s) {
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			m[l][c] += s;
		}
	}
	return *this;
}

Mat3& Mat3::operator-=(float s) {
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			m[l][c] -= s;
		}
	}
	return *this;
}

bool Mat3::operator==(const Mat3& other) const {
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			if (!Cmpf(m[l][c], other.m[l][c]))
				return false;
		}
	}
	return true;
}

bool Mat3::operator!=(const Mat3& other) const {
	return !(*this == other);
}

Mat3 Mat3::operator+(const Mat3& other) const {
	Mat3 sum;
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			sum.m[l][c] = m[l][c] + other.m[l][c];
		}
	}
	return sum;
}

Mat3 Mat3::operator-(const Mat3& other) const {
	Mat3 diff;
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			diff.m[l][c] = m[l][c] - other.m[l][c];
		}
	}
	return diff;
}

Mat3 Mat3::operator*(const Mat3& other) const {
	Mat3 prod;
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			prod.m[l][c] =  m[l][0] * other.m[0][c] +
							m[l][1] * other.m[1][c] +
							m[l][2] * other.m[2][c];
		}
	}
	return prod;
}

Mat3 Mat3::operator*(float s) const {
	Mat3 prod;
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			prod.m[l][c] = m[l][c] * s;
		}
	}
	return prod;
}

Mat3 operator*(float s, const Mat3& mat3) {
	Mat3 prod;
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			prod.m[l][c] = mat3.m[l][c] * s;
		}
	}
	return prod;
}

Mat3 Mat3::operator+(float s) const {
	Mat3 sum;
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			sum.m[l][c] = m[l][c] + s;
		}
	}
	return sum;
}

Mat3 operator+(float s, const Mat3& mat3) {
	Mat3 sum;
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			sum.m[l][c] = s + mat3.m[l][c];
		}
	}
	return sum;
}

Mat3 Mat3::operator-(float s) const {
	Mat3 diff;
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			diff.m[l][c] = m[l][c] - s;
		}
	}
	return diff;
}

Mat3 operator-(float s, const Mat3& mat3) {
	Mat3 diff;
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			diff.m[l][c] = s - mat3.m[l][c];
		}
	}
	return diff;
}

Mat3 Mat3::operator/(float s) const {
	Mat3 divid;
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			divid.m[l][c] = m[l][c] / s;
		}
	}
	return divid;
}

Vec3 Mat3::operator*(const Vec3& v) const {
	Vec3 prod;
	prod.x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z;
	prod.y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z;
	prod.z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z;
	return prod;
}

float* Mat3::operator[](int lines) {
	return m[lines];
}

Mat3 Mat3::Transpose() const {
	Mat3 trans;
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			trans.m[l][c] = m[c][l];
		}
	}
	return trans;
}

Quat Mat3::ToQuaternion() const {
    float tr = m[0][0] + m[1][1] + m[2][2];
    float x, y, z, w;

    if (tr > 0.0f) {
        float s = std::sqrt(tr + 1.0f) * 2.0f; // s = 4 * w
        w = 0.25f * s;
        x = (m[2][1] - m[1][2]) / s;
        y = (m[0][2] - m[2][0]) / s;
        z = (m[1][0] - m[0][1]) / s;
    } else if ((m[0][0] > m[1][1]) && (m[0][0] > m[2][2])) {
        float s = std::sqrt(1.0f + m[0][0] - m[1][1] - m[2][2]) * 2.0f; // s = 4 * x
        w = (m[2][1] - m[1][2]) / s;
        x = 0.25f * s;
        y = (m[0][1] + m[1][0]) / s;
        z = (m[0][2] + m[2][0]) / s;
    } else if (m[1][1] > m[2][2]) {
        float s = std::sqrt(1.0f + m[1][1] - m[0][0] - m[2][2]) * 2.0f; // s = 4 * y
        w = (m[0][2] - m[2][0]) / s;
        x = (m[0][1] + m[1][0]) / s;
        y = 0.25f * s;
        z = (m[1][2] + m[2][1]) / s;
    } else {
        float s = std::sqrt(1.0f + m[2][2] - m[0][0] - m[1][1]) * 2.0f; // s = 4 * z
        w = (m[1][0] - m[0][1]) / s;
        x = (m[0][2] + m[2][0]) / s;
        y = (m[1][2] + m[2][1]) / s;
        z = 0.25f * s;
    }
    return { w, x, y, z };
}

Vec3 Mat3::GetRightAxis() const {
	return Vec3(m[0][0], m[1][0], m[2][0]);
}

Vec3 Mat3::GetUpAxis() const {
	return Vec3(m[0][1], m[1][1], m[2][1]);
}

Vec3 Mat3::GetForwardAxis() const {
	return Vec3(m[0][2], m[1][2], m[2][2]);
}

bool Mat3::Inverse(Mat3& mat3) const {
	Mat3 adj;
	Mat3 trans = this->Transpose();
	float det = this->Determinant();

	if (Cmpf(det, 0.0f)) return false;

	adj.m[0][0] =	 (trans.m[1][1] * trans.m[2][2] - trans.m[1][2] * trans.m[2][1]);
	adj.m[0][1] = -1*(trans.m[1][0] * trans.m[2][2] - trans.m[1][2] * trans.m[2][0]);
	adj.m[0][2] =	 (trans.m[1][0] * trans.m[2][1] - trans.m[1][1] * trans.m[2][0]);
	adj.m[1][0] = -1*(trans.m[0][1] * trans.m[2][2] - trans.m[0][2] * trans.m[2][1]);
	adj.m[1][1] =	 (trans.m[0][0] * trans.m[2][2] - trans.m[0][2] * trans.m[2][0]);
	adj.m[1][2] = -1*(trans.m[0][0] * trans.m[2][1] - trans.m[0][1] * trans.m[2][0]);
	adj.m[2][0] =	 (trans.m[0][1] * trans.m[1][2] - trans.m[0][2] * trans.m[1][1]);
	adj.m[2][1] = -1*(trans.m[0][0] * trans.m[1][2] - trans.m[0][2] * trans.m[1][0]);
	adj.m[2][2] =	 (trans.m[0][0] * trans.m[1][1] - trans.m[0][1] * trans.m[1][0]);

	mat3 = (1 / det) * adj;

	return true;
}

void Mat3::ToOpenGLFormat(float array[9]) const {
	int i = 0;
	for (int c = 0; c < 3; c++) {
		for (int l = 0; l < 3; l++) {
			array[i] = m[l][c];
			++i;
		}
	}
}

bool Mat3::IsOrthogonal() const {
	return *this * this->Transpose() == Mat3::IDENTITY;
}

std::ostream& operator<<(std::ostream& os, const Mat3& mat3) {
	for (int l = 0; l < 3; l++) {
		os << "[ ";
		for (int c = 0; c < 3; c++) {
			os << mat3.m[l][c];
			if (c < 2) {
				os << " , ";
			}
		}
		os << " ]" << std::endl;
	}
	return os;
}

std::istream& operator>>(std::istream& is, Mat3& mat3) {
	for (int l = 0; l < 3; l++) {
		for (int c = 0; c < 3; c++) {
			is >> mat3.m[l][c];
		}
	}
	return is;
}
