
#include <cmath>

#include "Math/Quat.hpp"
#include "Math/MathAux.hpp"
#include "Core/Assert.hpp"

Quat::Quat() : Quat(1.0f, 0.0f, 0.0f, 0.0f) {}

Quat::Quat(float t, float x, float y, float z) : x(x), y(y), z(z), t(t) { }

Quat::Quat(float thetaRad, const Vec3& axis) {
	Vec3 axisNormalized = axis.Normalize();

    const float halfTheta = thetaRad * 0.5f;

	t = std::cos(halfTheta);

    const float s = std::sin(halfTheta);

	x = axis.x * s;
	y = axis.y * s;
	z = axis.z * s;

	Clean();
}

Quat Quat::IDENTITY = Quat(1.f, 0.f, 0.f, 0.f);

void Quat::ToAngleAxis(float& thetaRad, Vec3& axis) const {

	Quat normalized = this->Normalize();

	thetaRad = 2 * std::acos(normalized.t);
	float s = sqrt(1 - normalized.t * normalized.t);

	if (Cmpf(s, 0.0f)) {
		axis.x = 1.0f;
		axis.y = 0.0f;
		axis.z = 0.0f;
	}
	else {
		float sInv = 1.0f / s;
		axis.x = sInv * normalized.x;
		axis.y = sInv * normalized.y;
		axis.z = sInv * normalized.z;
	}
}

void Quat::Clean() {
	if (Cmpf(t, 0.0f)) t = 0.0f;
	if (Cmpf(x, 0.0f)) x = 0.0f;
	if (Cmpf(y, 0.0f)) y = 0.0f;
	if (Cmpf(z, 0.0f)) z = 0.0f;
}

float Quat::Quadrance() const {
	return SQR(t) + SQR(x) + SQR(y) + SQR(z);
}

float Quat::Norm() const {
	return std::sqrt(Quadrance());
}

float Quat::Angle() const {
	const Quat normalized = this->Normalize();
	return 2.f * std::acos(normalized.t);
}

Quat Quat::Normalize() const {

	float n = Norm();

	// Cannot divide by zero
	ASSERT(n > 0.0f);

	float s = 1 / Norm();

	return (*this) * s;
}

Quat Quat::Conjugate() const {
	return {t, -x, -y, -z};
}

Quat Quat::Inverse() const {

	float quad = Quadrance();

	ASSERT(quad > 0.0f);

	return Conjugate() * (1.0f / quad);
}

void Quat::ToEuler(float &roll, float &pitch, float &yaw) const {
	
	const float ySqrd = y*y;

	const float t0 = 2.0f * (t * x + y * z);
	const float t1 = 1.0f - 2.0f * (x * x + ySqrd);

	float t2 = 2.0f * (t * y - z * x);
	t2 = t2 > 1.0f ? 1.0f : t2;
	t2 = t2 < -1.0f ? -1.0f : t2;

	const float t3 = 2.0f * (t * z + x * y);
	const float t4 = 1.0f - 2.0f * (ySqrd + z * z);

	roll = std::atan2(t0, t1);
	pitch = std::asin(t2);
	yaw = std::atan2(t3, t4);
}

Quat Quat::operator*(float s) const {
	return { t * s, x * s, y * s, z * s };
}

Quat operator*(float s, const Quat& q) {
	return { s * q.t, s * q.x, s * q.y, s * q.z };
}

Quat Quat::operator*(const Quat& q) const {
	Quat prod;
	prod.t = t * q.t - x * q.x - y * q.y - z * q.z;
	prod.x = t * q.x + x * q.t + y * q.z - z * q.y;
	prod.y = t * q.y + y * q.t + z * q.x - x * q.z;
	prod.z = t * q.z + z * q.t + x * q.y - y * q.x;
	return prod;
}

Quat Quat::operator+(const Quat& q) const {
	return { t + q.t, x + q.x, y + q.y, z + q.z };
}

Quat& Quat::operator*=(float s) {
	t *= s;
	x *= s;
	y *= s;
	z *= s;
	return *this;
}

Quat& Quat::operator*=(const Quat& q) {
	const float ot = t, ox = x, oy = y, oz = z;
	t = ot * q.t - ox * q.x - oy * q.y - oz * q.z;
	x = ot * q.x + ox * q.t + oy * q.z - oz * q.y;
	y = ot * q.y + oy * q.t + oz * q.x - ox * q.z;
	z = ot * q.z + oz * q.t + ox * q.y - oy * q.x;
	return *this;
}

Quat& Quat::operator+=(const Quat& q) {
	t += q.t;
	x += q.x;
	y += q.y;
	z += q.z;
	return *this;
}

Quat& Quat::operator=(const Quat& q) {
	t = q.t;
	x = q.x;
	y = q.y;
	z = q.z;
	return *this;
}

bool Quat::operator==(const Quat& q) const {
	return Cmpf(t, q.t) && Cmpf(x, q.x) && Cmpf(y, q.y) && Cmpf(z, q.z);
}

bool Quat::operator!=(const Quat& q) const {
	return !(*this == q);
}

Quat Quat::Lerp(const Quat& q0, const Quat& q1, float k) {
	float cosAngle = q0.t * q1.t + q0.x * q1.x + q0.y * q1.y + q0.z * q1.z;
	float k0 = 1.0f - k;
	float k1 = (cosAngle > 0) ? k : -k;
	return (q0 * k0 + q1 * k1).Normalize();
}

Quat Quat::Slerp(const Quat& q0, const Quat& q1, float k) {

	// Difference at which to LERP instead of SLERP
	constexpr float delta = 0.0001f;

	// Calc cosine (dot product of q0 and q1)
	float sign_scale1 = 1.0f;
	float cos_omega = q0.t * q1.t + q0.x * q1.x + q0.y * q1.y + q0.z * q1.z;

	// Adjust signs (if necessary)
	if (cos_omega < 0.0f) {
		cos_omega = -cos_omega;
		sign_scale1 = -1.0f;
	}

	// Calculate coefficients
	float scale0, scale1;
	if (1.0f - cos_omega > delta) {
		// Standard case (slerp)
		float omega = std::acos(cos_omega);
		float sin_omega = std::sin(omega);
		scale0 = std::sin((1.0f - k) * omega) / sin_omega;
		scale1 = sign_scale1 * std::sin(k * omega) / sin_omega;
	} else {
		// Quaternions are very close so we can do a linear interpolation
		scale0 = 1.0f - k;
		scale1 = sign_scale1 * k;
	}

	// Interpolate between the two quaternions
	return Quat(q0 * scale0 + q1 * scale1).Normalize();
}

Quat Quat::FromDir(const Vec3& dir, const Vec3& ref) {
	Vec3 axis = Cross(ref, dir);
	float dotProduct = Dot(ref, dir);
	float halfAngle = std::acos(dotProduct) * .5f;

	// Handle special case where ref and dir are parallel
	if (axis.Magnitude() == 0.0f) {
		if (dotProduct > 0.9999f) {
			// Identity quaternion (no rotation)
			return Quat(1, 0, 0, 0);
		} else {
			// 180-degree rotation around any axis perpendicular to ref
			return Quat(0, 1, 0, 0);  // Example: 180-degree rotation around X-axis
		}
	}

	Quat q(cos(halfAngle), axis.x * sin(halfAngle), axis.y * sin(halfAngle), axis.z * sin(halfAngle));
	q = q.Normalize();

	return q;
}

Quat Quat::FromEuler(float roll, float pitch, float yaw) {

	const float cx = std::cos(roll * .5f);
	const float sx = std::sin(roll * .5f);
	const float cy = std::cos(pitch * .5f);
	const float sy = std::sin(pitch * .5f);
	const float cz = std::cos(yaw * .5f);
	const float sz = std::sin(yaw * .5f);

	return {
		cz * cx * cy + sz * sx * sy,
		cz * sx * cy - sz * cx * sy,
		cz * cx * sy + sz * sx * cy,
		sz * cx * cy - cz * sx * sy
	};
}
Quat Quat::FromTo(const Vec3& from, const Vec3& to) {

    /*
        Uses (inFrom = v1, inTo = v2):

        angle = arcos(v1 . v2 / |v1||v2|)
        axis = normalize(v1 x v2)

        Quaternion is then:

        s = sin(angle / 2)
        x = axis.x * s
        y = axis.y * s
        z = axis.z * s
        w = cos(angle / 2)

        Using identities:

        sin(2 * a) = 2 * sin(a) * cos(a)
        cos(2 * a) = cos(a)^2 - sin(a)^2
        sin(a)^2 + cos(a)^2 = 1

        This reduces to:

        x = (v1 x v2).x
        y = (v1 x v2).y
        z = (v1 x v2).z
        w = |v1||v2| + v1 . v2

        which then needs to be normalized because the whole equation was multiplied by 2 cos(angle / 2)
    */

    const float len_v1_v2 = sqrt(from.MagnitudeSquared() * to.MagnitudeSquared());
    const float w = len_v1_v2 + Dot(from, to);

    if (w == 0.0f)
    {
        if (len_v1_v2 == 0.0f)
        {
            // If either of the vectors has zero length, there is no rotation and we return identity
            return Quat::IDENTITY;
        }
        else
        {
            // If vectors are perpendicular, take one of the many 180 degree rotations that exist
            Vec3 normalizedPerpendicular = from.NormalizedPerpendicular();
            return Quat(0, normalizedPerpendicular.x, normalizedPerpendicular.y, normalizedPerpendicular.z);
        }
    }

    const Vec3 v = Cross(from, to);
    return Quat(w, v.x, v.y, v.z).Normalize();
}

Mat4 Quat::ToRotationMatrix() const {

	const Quat qn = this->Normalize();

	const float xt = qn.x * qn.t;
	const float xx = qn.x * qn.x;
	const float xy = qn.x * qn.y;
	const float xz = qn.x * qn.z;
	const float yt = qn.y * qn.t;
	const float yy = qn.y * qn.y;
	const float yz = qn.y * qn.z;
	const float zt = qn.z * qn.t;
	const float zz = qn.z * qn.z;

	Mat4 rot;

	rot[0][0] = 1.0f - 2.0f * (yy + zz);
	rot[1][0] = 2.0f * (xy + zt);
	rot[2][0] = 2.0f * (xz - yt);
	rot[3][0] = 0.0f;

	rot[0][1] = 2.0f * (xy - zt);
	rot[1][1] = 1.0f - 2.0f * (xx + zz);
	rot[2][1] = 2.0f * (yz + xt);
	rot[3][1] = 0.0f;

	rot[0][2] = 2.0f * (xz + yt);
	rot[1][2] = 2.0f * (yz - xt);
	rot[2][2] = 1.0f - 2.0f * (xx + yy);
	rot[3][2] = 0.0f;

	rot[0][3] = 0.0f;
	rot[1][3] = 0.0f;
	rot[2][3] = 0.0f;
	rot[3][3] = 1.0f;

	return rot;
}

Mat3 Quat::ToRotationMatrix3x3() const {
    const Quat qn = this->Normalize();

    const float xt = qn.x * qn.t;
    const float xx = qn.x * qn.x;
    const float xy = qn.x * qn.y;
    const float xz = qn.x * qn.z;
    const float yt = qn.y * qn.t;
    const float yy = qn.y * qn.y;
    const float yz = qn.y * qn.z;
    const float zt = qn.z * qn.t;
    const float zz = qn.z * qn.z;

    return {
        1.0f - 2.0f * (yy + zz), 2.0f * (xy - zt), 2.0f * (xz + yt),
        2.0f * (xy + zt), 1.0f - 2.0f * (xx + zz), 2.0f * (yz - xt),
        2.0f * (xz - yt), 2.0f * (yz + xt), 1.0f - 2.0f * (xx + yy)
    };
}

std::ostream& operator<<(std::ostream& os, const Quat& Qtrn) {
	os << "(" << Qtrn.t << "," << Qtrn.x << "," << Qtrn.y << "," << Qtrn.z << "," << ")";
	return os;
}

void Quat::PrintAngleAxis() {
	float angleRad;
	Vec3 axis;
	ToAngleAxis(angleRad, axis);
	std::cout << "Angle: " << RadiansToDegrees(angleRad) << " Axis: " << axis;
}
