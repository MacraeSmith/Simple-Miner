#pragma once
#include<string>
struct Vec3;
struct IntVec3
{
public:
	int x = 0;
	int y = 0;
	int z = 0;

	static const IntVec3 ZERO;
	static const IntVec3 ONE;
	static const IntVec3 NORTH;
	static const IntVec3 EAST;
	static const IntVec3 SOUTH;
	static const IntVec3 WEST;
	static const IntVec3 UP;
	static const IntVec3 DOWN;
	static const IntVec3 LEFT;
	static const IntVec3 FORWARD;
	static const IntVec3 RIGHT;
	static const IntVec3 BACKWARD;
	static const int NUM_CARDINAL_DIRECTIONS;
	static const IntVec3 CARDINAL_DIRECTIONS[6];

	IntVec3(){};
	~IntVec3() {};
	IntVec3(int initialX, int initialY, int initialZ);
	IntVec3(Vec3 const& copyFromVec3);
	std::string GetAsText() const;

	void SetFromText(char const* text, char delimiterToSplitOn = ',');
	// Operators (const)
	//-----------------------------------------------------------------------------------------------

	bool		operator==(const IntVec3& compare) const;		// IntVec2 == IntVec2
	bool		operator!=(const IntVec3& compare) const;		// IntVec2 != IntVec2
	const IntVec3	operator+(const IntVec3& vecToAdd) const;		// IntVec2 + IntVec2
	const IntVec3	operator-(const IntVec3& vecToSubtract) const;	// IntVec2 - IntVec2
	const IntVec3	operator-() const;								// -IntVec2, i.e. "unary negation"
	const IntVec3	operator*(int uniformScale) const;			// IntVec2 * float
	const IntVec3	operator*(const IntVec3& vecToMultiply) const;	// IntVec2 * IntVec2
	const IntVec3	operator/(int inverseScale) const;			// IntVec2 / float

	// Operators (self-mutating / non-const)
	//-----------------------------------------------------------------------------------------------

	void		operator+=(const IntVec3& vecToAdd);				// IntVec2 += IntVec2
	void		operator-=(const IntVec3& vecToSubtract);		// IntVec2 -= IntVec2
	void		operator*=(const int uniformScale);			// IntVec2 *= float
	void		operator/=(const int uniformDivisor);		// IntVec2 /= float
	void		operator=(const IntVec3& copyFrom);				// IntVec2 = IntVec2

	// Standalone "friend" functions that are conceptually, but not actually, part of IntVec2::
	//-----------------------------------------------------------------------------------------------

	friend const IntVec3 operator*(int uniformScale, const IntVec3& vecToScale);	// float * IntVec2

};

