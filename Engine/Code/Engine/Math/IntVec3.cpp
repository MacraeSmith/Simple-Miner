#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "math.h"

IntVec3 const IntVec3::ZERO = IntVec3(0, 0, 0);
IntVec3 const IntVec3::ONE = IntVec3(1, 1, 1);
IntVec3 const IntVec3::NORTH = IntVec3(0, 1, 0);
IntVec3 const IntVec3::SOUTH = IntVec3(0, -1, 0);
IntVec3 const IntVec3::EAST = IntVec3(1, 0, 0);
IntVec3 const IntVec3::WEST = IntVec3(-1, 0, 0);
IntVec3 const IntVec3::UP = IntVec3(0, 0, 1);
IntVec3 const IntVec3::DOWN = IntVec3(0, 0, -1);
IntVec3 const IntVec3::LEFT = IntVec3(0, 1, 0);
IntVec3 const IntVec3::FORWARD = IntVec3(1, 0, 0);
IntVec3 const IntVec3::RIGHT = IntVec3(0, -1, 0);
IntVec3 const IntVec3::BACKWARD = IntVec3(-1, 0, 0);
int const IntVec3::NUM_CARDINAL_DIRECTIONS = 6;
IntVec3 const IntVec3::CARDINAL_DIRECTIONS[6] =
{
	IntVec3::NORTH,
	IntVec3::EAST,
	IntVec3::SOUTH,
	IntVec3::WEST,
	IntVec3::UP,
	IntVec3::DOWN,
};

IntVec3::IntVec3(int initialX, int initialY, int initialZ)
	:x(initialX)
	,y(initialY)
	,z(initialZ)
{
}

IntVec3::IntVec3(Vec3 const& copyFromVec3)
	:x((int)(floorf(copyFromVec3.x)))
	,y((int)(floorf(copyFromVec3.y)))
	,z((int)(floorf(copyFromVec3.z)))
{
}

std::string IntVec3::GetAsText() const
{
	return Stringf("%i,%i,%i", x, y, z);
}

//Operators
//-----------------------------------------------------------------------------------------------
const IntVec3 IntVec3::operator + (const IntVec3& vecToAdd) const
{
	return IntVec3(x + vecToAdd.x, y + vecToAdd.y, z + vecToAdd.z);
}


//-----------------------------------------------------------------------------------------------
const IntVec3 IntVec3::operator-(const IntVec3& vecToSubtract) const
{
	return IntVec3(x - vecToSubtract.x, y - vecToSubtract.y, z - vecToSubtract.z);
}


//------------------------------------------------------------------------------------------------
const IntVec3 IntVec3::operator-() const
{
	return IntVec3(-x, -y, -z);
}


//-----------------------------------------------------------------------------------------------
const IntVec3 IntVec3::operator*(int uniformScale) const
{
	return IntVec3(x * uniformScale, y * uniformScale, z * uniformScale);
}


//------------------------------------------------------------------------------------------------
const IntVec3 IntVec3::operator*(const IntVec3& vecToMultiply) const
{
	return IntVec3(x * vecToMultiply.x, y * vecToMultiply.y, z * vecToMultiply.z);
}


//-----------------------------------------------------------------------------------------------
const IntVec3 IntVec3::operator/(int inverseScale) const
{
	return IntVec3(x / inverseScale, y / inverseScale, z / inverseScale);
}


//-----------------------------------------------------------------------------------------------
void IntVec3::operator+=(const IntVec3& vecToAdd)
{
	x += vecToAdd.x;
	y += vecToAdd.y;
	z += vecToAdd.z;
}


//-----------------------------------------------------------------------------------------------
void IntVec3::operator-=(const IntVec3& vecToSubtract)
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
	z -= vecToSubtract.z;
}


//-----------------------------------------------------------------------------------------------
void IntVec3::operator*=(const int uniformScale)
{
	x *= uniformScale;
	y *= uniformScale;
	z *= uniformScale;
}


//-----------------------------------------------------------------------------------------------
void IntVec3::operator/=(const int uniformDivisor)
{
	x /= uniformDivisor;
	y /= uniformDivisor;
	z /= uniformDivisor;
}


//-----------------------------------------------------------------------------------------------
void IntVec3::operator=(const IntVec3& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
	z = copyFrom.z;
}



//-----------------------------------------------------------------------------------------------
const IntVec3 operator*(int uniformScale, const IntVec3& vecToScale)
{
	return IntVec3(vecToScale.x * uniformScale, vecToScale.y * uniformScale, vecToScale.z * uniformScale);
}


void IntVec3::SetFromText(char const* text, char delimiterToSplitOn)
{
	Strings numsFromText = SplitStringOnDelimiter(text, delimiterToSplitOn);
	x = atoi(numsFromText[0].c_str());
	y = atoi(numsFromText[1].c_str());
	z = atoi(numsFromText[2].c_str());
}

//-----------------------------------------------------------------------------------------------
bool IntVec3::operator==(const IntVec3& compare) const
{
	return (x == compare.x && y == compare.y && z == compare.z);
}


//-----------------------------------------------------------------------------------------------
bool IntVec3::operator!=(const IntVec3& compare) const
{
	return (x != compare.x || y != compare.y || z != compare.z);
}
