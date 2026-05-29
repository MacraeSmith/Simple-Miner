#include "Engine/Math/Quaternion.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/MathUtils.hpp"

const Quaternion Quaternion::IDENTITY = Quaternion(0.f,0.f,0.f,1.f);

Quaternion::Quaternion(float inputX, float inputY, float inputZ, float inputW)
	:x(inputX)
	,y(inputY)
	,z(inputZ)
	,w(inputW)
{
}

Quaternion::Quaternion(EulerAngles const& eulerAngles)
{
	float degreesToRadians = 3.1415926535f / 180.f;
	float yawRads = degreesToRadians * eulerAngles.m_yawDegrees;
	float pitchRads = degreesToRadians * eulerAngles.m_pitchDegrees;
	float rollRads = degreesToRadians * eulerAngles.m_rollDegrees;

	float cy = cosf(yawRads * 0.5f);
	float sy = sinf(yawRads * 0.5f);
	float cp = cosf(pitchRads * 0.5f);
	float sp = sinf(pitchRads * 0.5f);
	float cr = cosf(rollRads * 0.5f);
	float sr = sinf(rollRads * 0.5f);

	// yaw(Z), pitch(Y), roll(X)
	w = cr * cp * cy + sr * sp * sy;
	x = sr * cp * cy - cr * sp * sy;
	y = cr * sp * cy + sr * cp * sy;
	z = cr * cp * sy - sr * sp * cy;

	Normalize();
}

Quaternion::Quaternion(float yawDegrees, float pitchDegrees, float rollDegrees)
{
	float degreesToRadians = 3.1415926535f / 180.f;
	float yawRads = degreesToRadians * yawDegrees;
	float pitchRads = degreesToRadians * pitchDegrees;
	float rollRads = degreesToRadians * rollDegrees;

	float cy = cosf(yawRads * 0.5f);
	float sy = sinf(yawRads * 0.5f);
	float cp = cosf(pitchRads * 0.5f);
	float sp = sinf(pitchRads * 0.5f);
	float cr = cosf(rollRads * 0.5f);
	float sr = sinf(rollRads * 0.5f);

	// yaw(Z), pitch(Y), roll(X)
	w = cr * cp * cy + sr * sp * sy;
	x = sr * cp * cy - cr * sp * sy;
	y = cr * sp * cy + sr * cp * sy;
	z = cr * cp * sy - sr * sp * cy;

	Normalize();
}

Quaternion::Quaternion(Vec3 const& axis, float angleRadians)
{
	float half = angleRadians * 0.5f;
	float s = sinf(half);

	x = axis.x * s;
	y = axis.y * s;
	z = axis.z * s;
	w = cosf(half);
}

Quaternion::Quaternion(Vec3 const& iFwrd, Vec3 const& jLeft, Vec3 const& kUp)
{
	// Column-major rotation matrix built from basis vectors
	float forwardX = iFwrd.x;
	float forwardY = iFwrd.y;
	float forwardZ = iFwrd.z;

	float leftX = jLeft.x;
	float leftY = jLeft.y;
	float leftZ = jLeft.z;

	float upX = kUp.x;
	float upY = kUp.y;
	float upZ = kUp.z;

	float trace = (forwardX + leftY + upZ);

	if (trace > 0.f)
	{
		float s = sqrtf((trace + 1.f)) * 2.f;
		w = 0.25f * s;
		x = (leftZ - upY) / s;
		y = (upX - forwardZ) / s;
		z = (forwardY - leftX) / s;
	}
	else if ((forwardX > leftY) && (forwardX > upZ))
	{
		float s = sqrtf((1.f + forwardX - leftY - upZ)) * 2.f;
		w = (leftZ - upY) / s;
		x = 0.25f * s;
		y = (leftX + forwardY) / s;
		z = (upX + forwardZ) / s;
	}
	else if (leftY > upZ)
	{
		float s = sqrtf((1.f + leftY - forwardX - upZ)) * 2.f;
		w = (upX - forwardZ) / s;
		x = (leftX + forwardY) / s;
		y = 0.25f * s;
		z = (upY + leftZ) / s;
	}
	else
	{
		float s = sqrtf((1.f + upZ - forwardX - leftY)) * 2.f;
		w = (forwardY - leftX) / s;
		x = (upX + forwardZ) / s;
		y = (upY + leftZ) / s;
		z = 0.25f * s;
	}

	Normalize();
}

float Quaternion::GetLengthSquared() const
{
	return (x * x) + (y * y) + (z * z) + (w * w);
}

void Quaternion::Normalize()
{
	float lenSq = GetLengthSquared();
	if (lenSq > 0.000001f)
	{
		float invLen = 1.f / sqrtf(lenSq);
		x *= invLen;
		y *= invLen;
		z *= invLen;
		w *= invLen;
	}

	else
	{
		x = 0.f;
		y = 0.f;
		z = 0.f;
		w = 1.f;
	}
}

Quaternion Quaternion::GetNormalized() const
{
	Quaternion q(x,y,z,w);
	q.Normalize();
	return q;
}

Vec3 Quaternion::GetRotatedVec3(Vec3 const& v) const
{
	Vec3 qv(x, y, z);
	Vec3 t = CrossProduct3D(qv, v) * 2.f;
	return v + (t * w) + CrossProduct3D(qv, t);
}

void Quaternion::GetAsVectors_IFwd_JLeft_KUp(Vec3& out_fwrd, Vec3& out_left, Vec3& out_up) const
{
	out_fwrd = GetRotatedVec3(Vec3(1.f, 0.f, 0.f));
	out_left = GetRotatedVec3(Vec3(0.f, 1.f, 0.f));
	out_up = GetRotatedVec3(Vec3(0.f, 0.f, 1.f));
}

Mat44 Quaternion::GetAsMatrix_IFwd_JLeft_KUp() const
{
	Vec3 fwd;
	Vec3 left;
	Vec3 up;
	GetAsVectors_IFwd_JLeft_KUp(fwd, left, up);

	Mat44 matrix;
	matrix.SetIJK3D(fwd,left,up);
	return matrix;
}

Vec3 Quaternion::GetIFwrd() const
{
	return GetRotatedVec3(Vec3(1.f, 0.f, 0.f));
}

Vec3 Quaternion::GetJLeft() const
{
	return GetRotatedVec3(Vec3(0.f, 1.f, 0.f));
}

Quaternion Quaternion::GetConjugate() const
{
	Quaternion result;
	result.x = -x;
	result.y = -y;
	result.z = -z;
	result.w = w;
	return result;
}

Quaternion Quaternion::Slerp(Quaternion const& a, Quaternion const& b, float t)
{
	Quaternion q0 = a;
	Quaternion q1 = b;

	float dot = ((q0.x * q1.x) + (q0.y * q1.y) + (q0.z * q1.z) + (q0.w * q1.w));

	// Shortest path: if dot < 0, negate q1 (same rotation, opposite hemisphere).
	if (dot < 0.f)
	{
		dot = -dot;
		q1.x = -q1.x;
		q1.y = -q1.y;
		q1.z = -q1.z;
		q1.w = -q1.w;
	}

	dot = GetClamped(dot, -1.f, 1.f);

	// If perfectly identical, just return either.
	if (dot == 1.f)
	{
		return q0;
	}

	float theta = acosf(dot);
	float sinTheta = sinf(theta);

	// If theta is pi (dot == -1) then sinTheta == 0 and the axis is ambiguous.
	// With the shortest-path handling above, dot should not be negative here,
	// but keep this exact check anyway.
	if (sinTheta == 0.f)
	{
		return q0;
	}

	float aScale = sinf((1.f - t) * theta) / sinTheta;
	float bScale = sinf(t * theta) / sinTheta;

	Quaternion out;
	out.x = (aScale * q0.x) + (bScale * q1.x);
	out.y = (aScale * q0.y) + (bScale * q1.y);
	out.z = (aScale * q0.z) + (bScale * q1.z);
	out.w = (aScale * q0.w) + (bScale * q1.w);

	return out.GetNormalized();
}

const Quaternion Quaternion::operator*(Quaternion const& quaternionToMultiply) const 
{
	Quaternion out;
	out.w = (w * quaternionToMultiply.w) - (x * quaternionToMultiply.x) - (y * quaternionToMultiply.y) - (z * quaternionToMultiply.z);
	out.x = (w * quaternionToMultiply.x) + (x * quaternionToMultiply.w) + (y * quaternionToMultiply.z) - (z * quaternionToMultiply.y);
	out.y = (w * quaternionToMultiply.y) - (x * quaternionToMultiply.z) + (y * quaternionToMultiply.w) + (z * quaternionToMultiply.x);
	out.z = (w * quaternionToMultiply.z) + (x * quaternionToMultiply.y) - (y * quaternionToMultiply.x) + (z * quaternionToMultiply.w);

	return out;
}
