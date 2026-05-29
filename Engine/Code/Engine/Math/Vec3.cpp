#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/Mat44.hpp"
#include "math.h"

Vec3 const Vec3::ZERO = Vec3(0.f, 0.f, 0.f);
Vec3 const Vec3::ONE = Vec3(1.f, 1.f, 1.f);
Vec3 const Vec3::NORTH = Vec3(0.f, 1.f, 0.f);
Vec3 const Vec3::SOUTH = Vec3(0.f, -1.f, 0.f);
Vec3 const Vec3::EAST = Vec3(1.f, 0.f, 0.f);
Vec3 const Vec3::WEST = Vec3(-1.f, 0.f, 0.f);
Vec3 const Vec3::UP = Vec3(0.f, 0.f, 1.f);
Vec3 const Vec3::DOWN = Vec3(0.f, 0.f, -1.f);
Vec3 const Vec3::LEFT = Vec3(0.f, 1.f, 0.f);
Vec3 const Vec3::FORWARD = Vec3(1.f, 0.f, 0.f);
Vec3 const Vec3::RIGHT = Vec3(0.f, -1.f, 0.f);
Vec3 const Vec3::BACKWARD = Vec3(-1.f, 0.f, 0.f);

Vec3::Vec3(const Vec3& copy)
	: x( copy.x )
	, y( copy.y )
	, z (copy.z )
{
}

//-----------------------------------------------------------------------------------------------
Vec3::Vec3( float initialX, float initialY, float initialZ )
	: x(initialX)
	, y(initialY)
	, z(initialZ)
{
}

Vec3::Vec3(Vec2 const& initialXY, float initialZ)
	:x(initialXY.x)
	,y(initialXY.y)
	,z(initialZ)
{
}

Vec3::Vec3(int initialX, int initialY, int initialZ)
	: x((float)initialX)
	, y((float)initialY)
	, z((float)initialZ)
{
}

Vec3::Vec3(IntVec3 const& copyFrom)
	: x((float)copyFrom.x)
	, y((float)copyFrom.y)
	, z((float)copyFrom.z)
{
}

//Accessors
//-----------------------------------------------------------------------------------------------



Vec3 const Vec3::MakeFromPolarRadians(float yawRadians, float pitchRadians, float length)
{
	float cosPitch = cosf(pitchRadians);
	float sinPitch = sinf(pitchRadians);
	float cosYaw = cosf(yawRadians);
	float sinYaw = sinf(yawRadians);
	Vec3 newPoint;
	newPoint.x = length * cosPitch * cosYaw;
	newPoint.y = length * cosPitch * sinYaw;
	newPoint.z = -length * sinPitch;
	return newPoint;
}

Vec3 const Vec3::MakeFromPolarDegrees(float yawDegrees, float pitchDegrees, float length)
{
	float cosPitch = CosDegrees(pitchDegrees); 
	float sinPitch = SinDegrees(pitchDegrees); 
	float cosYaw = CosDegrees(yawDegrees); 
	float sinYaw = SinDegrees(yawDegrees);
	Vec3 newPoint;
	newPoint.x = length * cosPitch * cosYaw;
	newPoint.y = length * cosPitch * sinYaw;
	newPoint.z = -length * sinPitch;
	return newPoint;
}

std::vector<Vec3> const Vec3::GetPositionInCircleAround(Vec3 const& centerPoint, Vec3 const& normal, float radius, int numPositionsToGet)
{
	std::vector<Vec3> circlePositions;
	circlePositions.reserve(numPositionsToGet);

	Vec3 referenceDirection(0.0f, 0.0f, 1.0f);
	if (fabsf(DotProduct3D(normal, referenceDirection)) > 0.99f)
	{
		referenceDirection = Vec3(1.0f, 0.0f, 0.0f);
	}

	Vec3 projectedReference = referenceDirection - (normal * DotProduct3D(referenceDirection, normal));
	Vec3 circleAxisX = projectedReference.GetNormalized();
	Vec3 circleAxisY = CrossProduct3D(normal, circleAxisX).GetNormalized();


	const float twoPi = 6.2831853071795864769f;

	for (int positionIndex = 0; positionIndex < numPositionsToGet; positionIndex++)
	{
		float angleFraction = ((float)positionIndex / (float)numPositionsToGet);
		float angleRadians = (angleFraction * twoPi);

		float cosAngle = cosf(angleRadians);
		float sinAngle = sinf(angleRadians);

		Vec3 offset = (circleAxisX * (cosAngle * radius)) + (circleAxisY * (sinAngle * radius));
		Vec3 position = centerPoint + offset;

		circlePositions.push_back(position);
	}
	
	return circlePositions;
}


float Vec3::GetLength() const
{
	return sqrtf((x * x) + (y * y) + (z * z));
}

float Vec3::GetLengthXY() const
{
	return sqrtf((x * x) + (y * y));
}

float Vec3::GetLengthSquared() const
{
	return (x * x) + (y * y) + (z * z);
}

float Vec3::GetLengthXYSquared() const
{
	return (x * x) + (y * y);
}

float Vec3::GetAngleAboutZRadians() const
{
	return atan2f(y,x);
}

float Vec3::GetAngleAboutZDegrees() const
{
	return Atan2Degrees(y, x);
}

EulerAngles Vec3::GetNormalizedOrientation_Xfwrd_Yleft_Zup() const
{
	Vec3 normalized = GetNormalized();
	float yaw = Atan2Degrees(normalized.y, normalized.x);
	float pitch = -AsinDegrees(normalized.z);
	return EulerAngles(yaw, pitch, 0.f);
}

Vec3 const Vec3::GetRotatedAboutZRadians(float deltaRadians) const
{
	Vec2 vec2XY(x, y);
	vec2XY.RotateRadians(deltaRadians);
	return Vec3(vec2XY.x, vec2XY.y, z);
}

Vec3 const Vec3::GetRotatedAboutZDegrees(float deltaDegrees) const
{
	Vec2 vec2XY(x, y);
	vec2XY.RotateDegrees(deltaDegrees);
	return Vec3(vec2XY.x, vec2XY.y, z);
}

Vec3 const Vec3::GetClamped(float maxLength) const
{
	Vec3 clampedVec(x, y, z);
	clampedVec.ClampLength(maxLength);
	return clampedVec;
}

Vec3 const Vec3::GetClamped(float minLength, float maxLength)
{
	Vec3 clampedVec(x, y, z);
	clampedVec.ClampLength(minLength, maxLength);
	return clampedVec;
}

Vec3 const Vec3::GetNormalized() const
{
	Vec3 normalizedVec(x, y, z);
	normalizedVec.Normalize();
	return normalizedVec;
}

Vec2 const Vec3::GetXY() const
{
	return Vec2(x, y);
}

std::string Vec3::GetAsText(int numDecimals) const
{
	numDecimals = GetClampedInt(numDecimals, 0, 10);
	char format[64];
	std::snprintf(format, sizeof(format), "%%.%df, %%.%df, %%.%df", numDecimals, numDecimals, numDecimals);
	return Stringf(format, x, y, z);
}

//Mutators
//-----------------------------------------------------------------------------------------------

void Vec3::SetFromText(char const* text, char delimiterToSplitOn)
{
	Strings numsFromText = SplitStringOnDelimiter(text, delimiterToSplitOn);
	x = (float)(atof(numsFromText[0].c_str()));
	y = (float)(atof(numsFromText[1].c_str()));
	z = (float)(atof(numsFromText[2].c_str()));
}

void Vec3::SetLength(float newLength)
{
	float oldLength = GetLength();
	float scale = (newLength / oldLength);
	x *= scale;
	y *= scale;
	z *= scale;
}

void Vec3::ClampLength(float maxLength)
{
	float oldLength = GetLength();

	if (oldLength > maxLength)
	{
		SetLength(maxLength);
	}
}

void Vec3::ClampLength(float minLength, float maxLength)
{
	float oldLength = GetLength();

	if (oldLength > maxLength)
	{
		SetLength(maxLength);
	}

	else if (oldLength < minLength)
	{
		SetLength(minLength);
	}
}

void Vec3::Normalize()
{
	float length = GetLength();
	if (length == 0.f || length == 1.f) return;
	float scale = 1.f / length;

	x *= scale;
	y *= scale;
	z *= scale;
}

//-----------------------------------------------------------------------------------------------
const Vec3 Vec3::operator+(const Vec3& vecToAdd) const
{
	return Vec3(x + vecToAdd.x, y + vecToAdd.y, z + vecToAdd.z);
}

//-----------------------------------------------------------------------------------------------
const Vec3 Vec3::operator-(const Vec3& vecToSubtract) const
{
	return Vec3(x - vecToSubtract.x, y - vecToSubtract.y, z - vecToSubtract.z);
}

//-----------------------------------------------------------------------------------------------
const Vec3 Vec3::operator-() const
{
	return Vec3(-x, -y, -z);
}

//-----------------------------------------------------------------------------------------------
const Vec3 Vec3::operator*(float uniformScale) const
{
	return Vec3(x * uniformScale, y * uniformScale, z * uniformScale);
}

//-----------------------------------------------------------------------------------------------
const Vec3 Vec3::operator*(const Vec3& vecToMultiply) const
{
	return Vec3(x * vecToMultiply.x, y * vecToMultiply.y, z * vecToMultiply.z);
}

//-----------------------------------------------------------------------------------------------
const Vec3 Vec3::operator/(float inverseScale) const
{
	return Vec3(x / inverseScale, y / inverseScale, z / inverseScale);
}

//-----------------------------------------------------------------------------------------------
void Vec3::operator+=(const Vec3& vecToAdd)
{
	x += vecToAdd.x;
	y += vecToAdd.y;
	z += vecToAdd.z;
}

//-----------------------------------------------------------------------------------------------
void Vec3::operator-=(const Vec3& vecToSubtract)
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
	z -= vecToSubtract.z;
}

//-----------------------------------------------------------------------------------------------
void Vec3::operator*=(const float uniformScale)
{
	x *= uniformScale;
	y *= uniformScale;
	z *= uniformScale;
}

//-----------------------------------------------------------------------------------------------
void Vec3::operator/=(const float uniformDivisor)
{
	x /= uniformDivisor;
	y /= uniformDivisor;
	z /= uniformDivisor;
}

//-----------------------------------------------------------------------------------------------
void Vec3::operator=(const Vec3& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
	z = copyFrom.z;
}

//-----------------------------------------------------------------------------------------------
const Vec3 operator*(float uniformScale, const Vec3& vecToScale)
{
	return Vec3(vecToScale.x * uniformScale, vecToScale.y * uniformScale, vecToScale.z * uniformScale);
}

//-----------------------------------------------------------------------------------------------
bool Vec3::operator==(const Vec3& compare) const
{
	return(x == compare.x && y == compare.y && z == compare.z);
}

//-----------------------------------------------------------------------------------------------
bool Vec3::operator!=(const Vec3& compare) const
{
	return (x != compare.x || y != compare.y || z != compare.z);
}


