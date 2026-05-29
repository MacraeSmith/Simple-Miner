#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "math.h"

IntVec2 const IntVec2::ZERO = IntVec2();
IntVec2 const IntVec2::ONE = IntVec2(1, 1);
IntVec2 const IntVec2::ZERO_TO_ONE = IntVec2(0, 1);
IntVec2 const IntVec2::ONE_TO_ZERO = IntVec2(1, 0);
IntVec2 const IntVec2::NORTH = IntVec2(0, 1);
IntVec2 const IntVec2::EAST = IntVec2(1, 0);
IntVec2 const IntVec2::SOUTH = IntVec2(0, -1);
IntVec2 const IntVec2::WEST = IntVec2(-1, 0);
int const IntVec2::NUM_DIRECTIONS_N_E_S_W = 4;
IntVec2 const IntVec2::DIRECTIONS_N_E_S_W[4] =
{
	IntVec2::NORTH,
	IntVec2::EAST,
	IntVec2::SOUTH,
	IntVec2::WEST,

};
int const IntVec2::NUM_DIRECTIONS_N_E_S_W_NE_SE_SW_NW = 8;
IntVec2 const IntVec2::DIRECTIONS_N_E_S_W_NE_SE_SW_NW[8] =
{
	IntVec2::NORTH,
	IntVec2::EAST,
	IntVec2::SOUTH,
	IntVec2::WEST,
	IntVec2(1,1),
	IntVec2(1,-1),
	IntVec2(-1,-1),
	IntVec2(-1,1),
};

//-----------------------------------------------------------------------------------------------
IntVec2::IntVec2( const IntVec2& copy )
	: x( copy.x )
	, y( copy.y )
{
}

//#TODO: make sure flooring is the best option for this, Changing might break Libra
IntVec2::IntVec2(Vec2 const& copyFromVec2)
	:x((int)(floorf(copyFromVec2.x)))
	,y((int)(floorf(copyFromVec2.y)))
{
}

//-----------------------------------------------------------------------------------------------
IntVec2::IntVec2( int initialX, int initialY )
	: x( initialX )
	, y( initialY)
{
}

IntVec2::IntVec2(size_t initialX, size_t initialY)
	:x((int)(initialX))
	, y((int)(initialY))
{
}

//Accessors (const methods)
//-----------------------------------------------------------------------------------------------
float const IntVec2::GetLength() const
{
	return sqrtf((float)((x * x) + (y * y)));
}

int const IntVec2::GetLengthSquared() const
{
	return (x * x) + (y * y);
}

int const IntVec2::GetTaxicabLength() const
{
	return abs(x) + abs(y);
}

float const IntVec2::GetOrientationRadians() const
{
	return atan2f((float)(y), (float)(x));
}

float const IntVec2::GetOrientationDegrees() const
{
	return Atan2Degrees((float)(y), (float)(x));
}

IntVec2 const IntVec2::GetRotated90Degrees() const
{
	IntVec2 rotatedVec(x, y);
	int oldX = rotatedVec.x;
	rotatedVec.x = -rotatedVec.y;
	rotatedVec.y = oldX;
	return rotatedVec;
}

IntVec2 const IntVec2::GetRotatedMinus90Degrees() const
{
	IntVec2 rotatedVec(x, y);
	int oldX = rotatedVec.x;
	rotatedVec.x = rotatedVec.y;
	rotatedVec.y = -oldX;
	return rotatedVec;
}

std::string IntVec2::GetAsText() const
{
	return Stringf("%i,%i", x, y);
}

void IntVec2::SetFromText(char const* text)
{
	Strings numsFromText = SplitStringOnDelimiter(text, ',');
	x = atoi(numsFromText[0].c_str());
	y = atoi(numsFromText[1].c_str());
}

//Mutators (non-const methods)
//-----------------------------------------------------------------------------------------------
void IntVec2::Rotate90Degrees()
{
	int oldX = x;
	x = -y;
	y = oldX;
}

void IntVec2::RotateMinus90Degrees()
{
	int oldX = x;
	x = y;
	y = -oldX;
}


//Operators
//-----------------------------------------------------------------------------------------------
const IntVec2 IntVec2::operator + ( const IntVec2& vecToAdd ) const
{
	return IntVec2(x + vecToAdd.x, y + vecToAdd.y);
}


//-----------------------------------------------------------------------------------------------
const IntVec2 IntVec2::operator-( const IntVec2& vecToSubtract ) const
{
	return IntVec2(x - vecToSubtract.x, y - vecToSubtract.y);
}


//------------------------------------------------------------------------------------------------
const IntVec2 IntVec2::operator-() const
{
	return IntVec2( -x, -y);
}


//-----------------------------------------------------------------------------------------------
const IntVec2 IntVec2::operator*( int uniformScale ) const
{
	return IntVec2(x * uniformScale, y * uniformScale);
}


//------------------------------------------------------------------------------------------------
const IntVec2 IntVec2::operator*( const IntVec2& vecToMultiply ) const
{
	return IntVec2( x * vecToMultiply.x, y * vecToMultiply.y);
}


//-----------------------------------------------------------------------------------------------
const IntVec2 IntVec2::operator/( int inverseScale ) const
{
	return IntVec2( x / inverseScale, y / inverseScale );
}


//-----------------------------------------------------------------------------------------------
void IntVec2::operator+=( const IntVec2& vecToAdd )
{
	x += vecToAdd.x;
	y += vecToAdd.y;
}


//-----------------------------------------------------------------------------------------------
void IntVec2::operator-=( const IntVec2& vecToSubtract )
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
}


//-----------------------------------------------------------------------------------------------
void IntVec2::operator*=( const int uniformScale )
{
	x *= uniformScale;
	y *= uniformScale;
}


//-----------------------------------------------------------------------------------------------
void IntVec2::operator/=( const int uniformDivisor )
{
	x /= uniformDivisor;
	y /= uniformDivisor;
}


//-----------------------------------------------------------------------------------------------
void IntVec2::operator=( const IntVec2& copyFrom )
{
	x = copyFrom.x;
	y = copyFrom.y;
}



//-----------------------------------------------------------------------------------------------
const IntVec2 operator*( int uniformScale, const IntVec2& vecToScale )
{
	return IntVec2(vecToScale.x * uniformScale, vecToScale.y * uniformScale);
}


//-----------------------------------------------------------------------------------------------
bool IntVec2::operator==( const IntVec2& compare ) const
{
	return (x == compare.x && y == compare.y);
}


//-----------------------------------------------------------------------------------------------
bool IntVec2::operator!=( const IntVec2& compare ) const
{
	return (x != compare.x || y != compare.y);
}



