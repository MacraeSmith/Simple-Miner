#pragma once
struct EulerAngles;
struct Vec3;
struct Mat44;
struct Quaternion
{
public:
	// (x, y, z) = vector part, w = scalar part
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
	float w = 1.f; // identity

	static const Quaternion IDENTITY;

	Quaternion() {};
	explicit Quaternion(float inputX, float inputY, float inputZ, float inputW);
	explicit Quaternion(EulerAngles const& eulerAngles);
	explicit Quaternion(float yawDegrees, float pitchDegrees, float rollDegrees);
	explicit Quaternion(Vec3 const& axis, float angleRadians);
	explicit Quaternion(Vec3 const& iFwrd, Vec3 const& jLeft, Vec3 const& kUp);
	~Quaternion() {};

	float GetLengthSquared() const;
	void Normalize();
	Quaternion GetNormalized() const;

	Vec3 GetRotatedVec3(Vec3 const& v) const;

	void GetAsVectors_IFwd_JLeft_KUp(Vec3& out_fwrd, Vec3& out_left, Vec3& out_up) const;
	Mat44 GetAsMatrix_IFwd_JLeft_KUp() const;
	Vec3 GetIFwrd() const;
	Vec3 GetJLeft() const;

	Quaternion GetConjugate() const;
	static Quaternion Slerp(Quaternion const& a, Quaternion const& b, float t);

	const Quaternion operator*(Quaternion const& quaternion) const;
};

