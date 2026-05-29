#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "math.h"
//#include "Engine/Core/EngineCommon.hpp"

Vec2 const Vec2::ZERO = Vec2();
Vec2 const Vec2::ONE = Vec2(1.f, 1.f);
Vec2 const Vec2::ZERO_TO_ONE = Vec2(0.f, 1.f);
Vec2 const Vec2::ONE_TO_ZERO = Vec2(1.f, 0.f);
Vec2 const Vec2::NORTH = Vec2(0.f, 1.f);
Vec2 const Vec2::EAST = Vec2(1.f, 0.f);
Vec2 const Vec2::SOUTH = Vec2(0.f, -1.f);
Vec2 const Vec2::WEST = Vec2(-1.f, 0.f);

//-----------------------------------------------------------------------------------------------
Vec2::Vec2( const Vec2& copy )
	: x( copy.x )
	, y( copy.y )
{
}

Vec2::Vec2(IntVec2 const& copyFromIntVec2)
	:x((float)copyFromIntVec2.x)
	,y((float)copyFromIntVec2.y)
{
}

//-----------------------------------------------------------------------------------------------
Vec2::Vec2( float initialX, float initialY )
	: x( initialX )
	, y( initialY)
{
}

Vec2::Vec2(int initialX, int initialY)
	:x((float)initialX)
	,y((float)initialY)
{
}

//Static Methods
//-----------------------------------------------------------------------------------------------
Vec2 const Vec2::MakeFromPolarRadians(float orientationRadians, float length)
{
	float newX = length * cosf(orientationRadians);
	float newY = length * sinf(orientationRadians);

	return Vec2(newX, newY);
}

Vec2 const Vec2::MakeFromPolarDegrees(float orientationDegrees, float length)
{
	float newX = length * CosDegrees(orientationDegrees);
	float newY = length * SinDegrees(orientationDegrees);

	return Vec2(newX, newY);
}

float const Vec2::GetOrientationRadians(float inputX, float inputY)
{
	return atan2f(inputY, inputX);
}

float const Vec2::GetOrientationDegrees(float inputX, float inputY)
{
	return Atan2Degrees(inputY, inputX);
}

float const Vec2::GetPolarAngleAboutPoint(Vec2 const& point, Vec2 const& origin)
{
	Vec2 delta = (point - origin);
	return atan2f(delta.y, delta.x);
}

bool Vec2::ArePointsColinear(Vec2 const& prev, Vec2 const& curr, Vec2 const& next)
{
	Vec2 const v1 = (curr - prev);
	Vec2 const v2 = (next - prev);

	float const cross = CrossProduct2D(v1, v2);
	return (cross >= 0.f) && (cross <= 0.f);
}


//Accessors (const methods)
//-----------------------------------------------------------------------------------------------
float const Vec2::GetLength() const
{
	return sqrtf((x * x) + (y * y));
}

float const Vec2::GetLengthSquared() const
{
	return (x * x) + (y * y);
}

float const Vec2::GetOrientationRadians() const
{
	return atan2f(y, x);
}


float const Vec2::GetOrientationDegrees() const
{
	return Atan2Degrees(y,x);
}

Vec2 const Vec2::GetRotated90Degrees() const
{
	Vec2 rotatedVec(x, y);
	float oldX = rotatedVec.x;
	rotatedVec.x = -rotatedVec.y;
	rotatedVec.y = oldX;
	return rotatedVec;
}

Vec2 const Vec2::GetRotatedMinus90Degrees() const
{
	Vec2 rotatedVec(x, y);
	float oldX = rotatedVec.x;
	rotatedVec.x = rotatedVec.y;
	rotatedVec.y = -oldX;
	return rotatedVec;
}

Vec2 const Vec2::GetRotatedDegrees(float deltaDegrees) const
{
	Vec2 rotatedVec(x, y);
	rotatedVec.RotateDegrees(deltaDegrees);
	return rotatedVec;
}

Vec2 const Vec2::GetRotatedRadians(float deltaRadians) const
{
	Vec2 rotatedVec(x, y);
	rotatedVec.RotateRadians(deltaRadians);
	return rotatedVec;
}

Vec2 const Vec2::GetClamped(float maxLength) const
{
	Vec2 clampedVec(x, y);
	clampedVec.ClampLength(maxLength);
	return clampedVec;
}

Vec2 const Vec2::GetClamped(float minLength, float maxLength) const
{
	Vec2 clampedVec(x, y);
	clampedVec.ClampLength(minLength, maxLength);
	return clampedVec;
}

Vec2 const Vec2::GetNormalized() const
{
	Vec2 clampedVec(x, y);
	clampedVec.Normalize();
	return clampedVec;
}

Vec2 const Vec2::GetReflected(Vec2 const& normalOfSurfaceToReflectOffOf) const
{
	Vec2 currentVector = Vec2(x,y);
	float projectedLength = GetProjectedLength2D(currentVector, normalOfSurfaceToReflectOffOf);
	Vec2 vectorAlongNormal = normalOfSurfaceToReflectOffOf * projectedLength;
	Vec2 vectorAlongPerpendicular = currentVector - vectorAlongNormal;
	return vectorAlongPerpendicular - vectorAlongNormal;
}

std::string Vec2::GetAsText(int numDecimals) const
{
	numDecimals = GetClampedInt(numDecimals, 0, 10);
	char format[64];
	std::snprintf(format, sizeof(format), "%%.%df, %%.%df", numDecimals, numDecimals);
	return Stringf(format, x, y);
}


//Mutators (non-const methods)
//-----------------------------------------------------------------------------------------------

void Vec2::SetFromText(char const* text, char delimiterToSplitOn)
{
	Strings numsFromText = SplitStringOnDelimiter(text, delimiterToSplitOn);
	x = (float)(atof(numsFromText[0].c_str()));
	y = (float)(atof(numsFromText[1].c_str()));
}

void Vec2::SetOrientationRadians(float newOrientationRadians)
{
	float length = GetLength();
	x = length * cosf(newOrientationRadians);
	y = length * sinf(newOrientationRadians);
}

void Vec2::SetOrientationDegrees(float newOrientationDegrees)
{
	float length = GetLength();
	x = length * CosDegrees(newOrientationDegrees);
	y = length * SinDegrees(newOrientationDegrees);

}

void Vec2::SetPolarRadians(float newOrientationRadians, float newLength)
{
	x = newLength * cosf(newOrientationRadians);
	y = newLength * sinf(newOrientationRadians);
}

void Vec2::SetPolarDegrees(float newOrientationDegrees, float newLength)
{
	x = newLength * CosDegrees(newOrientationDegrees);
	y = newLength * SinDegrees(newOrientationDegrees);
}

void Vec2::Rotate90Degrees()
{
	float oldX = x;
	x = -y;
	y = oldX;
}

void Vec2::RotateMinus90Degrees()
{
	float oldX = x;
	x = y;
	y = -oldX;
}

void Vec2::RotateRadians(float deltaRadians)
{
	Vec2 iBasis(cosf(deltaRadians), sinf(deltaRadians));
	Vec2 jBasis(-iBasis.y, iBasis.x);
	Vec2 rotatedVec((x * iBasis) + (y * jBasis));
	x = rotatedVec.x;
	y = rotatedVec.y;
}

void Vec2::RotateDegrees(float deltaDegrees)
{
	Vec2 iBasis(CosDegrees(deltaDegrees), SinDegrees(deltaDegrees));
	Vec2 jBasis(-iBasis.y, iBasis.x);
	Vec2 rotatedVec((x * iBasis) + (y * jBasis));
	x = rotatedVec.x;
	y = rotatedVec.y;
}

void Vec2::SetLength(float newLength)
{
	float oldLength = GetLength();
	if (oldLength == 0)
		return;

	float scale = (newLength / oldLength);
	x *= scale;
	y *= scale;
}

void Vec2::ClampLength(float maxLength)
{
	float oldLength = GetLength();

	if (oldLength > maxLength) 
	{
		SetLength(maxLength);
	}
}

void Vec2::ClampLength(float minLength, float maxLength)
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

void Vec2::Normalize()
{
	float length = GetLength();
	if (length == 0.f) return; //don't continue if length is zero
	float scale = 1.f / length;
	x *= scale;
	y *= scale;
}

float Vec2::NormalizeAndGetPreviousLength()
{
	float oldLength = GetLength();
	if (oldLength == 0.f) return oldLength; //don't continue if oldLength is zero
	float scale = 1.f / oldLength;
	x *= scale;
	y *= scale;

	return oldLength;
}

void Vec2::Reflect(Vec2 const& normalOfSurfaceToReflectOffOf)
{
	Vec2 currentVector = Vec2(x, y);
	float projectedLength = GetProjectedLength2D(currentVector, normalOfSurfaceToReflectOffOf);
	Vec2 vectorAlongNormal = normalOfSurfaceToReflectOffOf * projectedLength;
	Vec2 vectorAlongPerpendicular = currentVector - vectorAlongNormal;
	Vec2 reflectedVector = vectorAlongPerpendicular - vectorAlongNormal;
	x = reflectedVector.x;
	y = reflectedVector.y;
}



//Operators
//-----------------------------------------------------------------------------------------------
const Vec2 Vec2::operator + ( const Vec2& vecToAdd ) const
{
	return Vec2(x + vecToAdd.x, y + vecToAdd.y);
}


//-----------------------------------------------------------------------------------------------
const Vec2 Vec2::operator-( const Vec2& vecToSubtract ) const
{
	return Vec2(x - vecToSubtract.x, y - vecToSubtract.y);
}


//------------------------------------------------------------------------------------------------
const Vec2 Vec2::operator-() const
{
	return Vec2(-x, -y);
}


//-----------------------------------------------------------------------------------------------
const Vec2 Vec2::operator*( float uniformScale ) const
{
	return Vec2(x * uniformScale, y * uniformScale);
}


//------------------------------------------------------------------------------------------------
const Vec2 Vec2::operator*( const Vec2& vecToMultiply ) const
{
	return Vec2( x * vecToMultiply.x, y * vecToMultiply.y);
}


//-----------------------------------------------------------------------------------------------
const Vec2 Vec2::operator/( float inverseScale ) const
{
	return Vec2( x / inverseScale, y / inverseScale );
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator+=( const Vec2& vecToAdd )
{
	x += vecToAdd.x;
	y += vecToAdd.y;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator-=( const Vec2& vecToSubtract )
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator*=( const float uniformScale )
{
	x *= uniformScale;
	y *= uniformScale;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator/=( const float uniformDivisor )
{
	x /= uniformDivisor;
	y /= uniformDivisor;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator=( const Vec2& copyFrom )
{
	x = copyFrom.x;
	y = copyFrom.y;
}



//-----------------------------------------------------------------------------------------------
const Vec2 operator*( float uniformScale, const Vec2& vecToScale )
{
	return Vec2(vecToScale.x * uniformScale, vecToScale.y * uniformScale);
}


//-----------------------------------------------------------------------------------------------
bool Vec2::operator==( const Vec2& compare ) const
{
	return (x == compare.x && y == compare.y);
}


//-----------------------------------------------------------------------------------------------
bool Vec2::operator!=( const Vec2& compare ) const
{
	return (x != compare.x || y != compare.y);
}



