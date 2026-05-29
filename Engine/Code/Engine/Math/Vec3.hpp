#pragma once
#include<string>
#include <vector>
struct EulerAngles;
struct Vec2;
struct IntVec3;
class RandomNumberGenerator;

struct Vec3
{
public: // NOTE: this is one of the few cases where we break both the "m_" naming rule AND the avoid-public-members rule
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;

	static const Vec3 ZERO;
	static const Vec3 ONE;
	static const Vec3 NORTH;
	static const Vec3 EAST;
	static const Vec3 SOUTH;
	static const Vec3 WEST;
	static const Vec3 UP;
	static const Vec3 DOWN;
	static const Vec3 LEFT;
	static const Vec3 FORWARD;
	static const Vec3 RIGHT;
	static const Vec3 BACKWARD;

	static Vec3 const MakeFromPolarRadians(float yawRadians, float pitchRadians, float length = 1.f);
	static Vec3 const MakeFromPolarDegrees(float yawDegrees, float pitchDegrees, float length = 1.f);
	static std::vector<Vec3> const GetPositionInCircleAround(Vec3 const& centerPoint, Vec3 const& normal, float radius, int numPositionsToGet);

	//Accessors (const methods)
	//-----------------------------------------------------------------------------------------------

	float GetLength() const;
	float GetLengthXY() const;
	float GetLengthSquared() const;
	float GetLengthXYSquared() const;
	float GetAngleAboutZRadians() const;
	float GetAngleAboutZDegrees() const;
	EulerAngles GetNormalizedOrientation_Xfwrd_Yleft_Zup() const;
	Vec3 const GetRotatedAboutZRadians(float deltaRadians) const;
	Vec3 const GetRotatedAboutZDegrees(float deltaDegrees) const;

	Vec3 const GetClamped(float maxLength) const;
	Vec3 const GetClamped(float minLength, float maxLength);

	Vec3 const GetNormalized() const;
	Vec2 const GetXY() const;

	std::string GetAsText(int numDecimals = 2) const;


	//Mutators (non-const methods)
	//-----------------------------------------------------------------------------------------------
	void SetFromText(char const* text, char delimiterToSplitOn = ',');
	void SetLength(float newLength);
	void ClampLength(float maxLength);
	void ClampLength(float minLength, float maxLength); // clamp with max and min
	void Normalize();

public:
	// Construction/Destruction
	//-----------------------------------------------------------------------------------------------
	~Vec3() {}												// destructor (do nothing)
	Vec3() {}												// default constructor (do nothing)
	Vec3(const Vec3& copyFrom);							// copy constructor (from another vec3)
	explicit Vec3(float initialX, float initialY, float initialZ);		// explicit constructor (from x, y, z)
	explicit Vec3(Vec2 const& initialXY, float initialZ);
	explicit Vec3(int initialX, int initialY, int initialZ);
	explicit Vec3(IntVec3 const& copyFrom);

	// Operators (const)
	//-----------------------------------------------------------------------------------------------
	bool		operator==(const Vec3& compare) const;		// vec3 == vec3
	bool		operator!=(const Vec3& compare) const;		// vec3 != vec3
	const Vec3	operator+(const Vec3& vecToAdd) const;		// vec3 + vec3
	const Vec3	operator-(const Vec3& vecToSubtract) const;	// vec3 - vec3
	const Vec3	operator-() const;								// -vec3, i.e. "unary negation"
	const Vec3	operator*(float uniformScale) const;			// vec3 * float
	const Vec3	operator/(float inverseScale) const;			// vec3 / float
	const Vec3	operator*(const Vec3& vecToMultiply) const;	// vec3 * vec3

	// Operators (self-mutating / non-const)
	//-----------------------------------------------------------------------------------------------
	void		operator+=(const Vec3& vecToAdd);				// vec3 += vec3
	void		operator-=(const Vec3& vecToSubtract);		// vec3 -= vec3
	void		operator*=(const float uniformScale);			// vec3 *= float
	void		operator/=(const float uniformDivisor);		// vec3 /= float
	void		operator=(const Vec3& copyFrom);				// vec3 = vec3

	// Standalone "friend" functions that are conceptually, but not actually, part of Vec3::
	friend const Vec3 operator*(float uniformScale, const Vec3& vecToScale);	// float * vec3
};

