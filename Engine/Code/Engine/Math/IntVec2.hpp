#pragma once
#include <string>

struct Vec2;
//-----------------------------------------------------------------------------------------------
struct IntVec2
{
public: // NOTE: this is one of the few cases where we break both the "m_" naming rule AND the avoid-public-members rule
	int x = 0;
	int y = 0;

	static const IntVec2 ZERO;
	static const IntVec2 ONE;
	static const IntVec2 ZERO_TO_ONE;
	static const IntVec2 ONE_TO_ZERO;
	static const IntVec2 NORTH;
	static const IntVec2 EAST;
	static const IntVec2 SOUTH;
	static const IntVec2 WEST;
	static const int NUM_DIRECTIONS_N_E_S_W_NE_SE_SW_NW;
	static const IntVec2 DIRECTIONS_N_E_S_W_NE_SE_SW_NW[8];
	static const int NUM_DIRECTIONS_N_E_S_W;
	static const IntVec2 DIRECTIONS_N_E_S_W[4];

public:
	// Construction/Destruction
	~IntVec2() {}												// destructor (do nothing)
	IntVec2() {}												// default constructor (do nothing)
	IntVec2( const IntVec2& copyFrom );							// copy constructor (from another IntVec2)
	explicit IntVec2(Vec2 const& copyFromVec2);
	explicit IntVec2( int initialX, int initialY );		// explicit constructor (from x, y)
	explicit IntVec2(size_t initialX, size_t initialY);
	
	//Accessors (const methods)
	//-----------------------------------------------------------------------------------------------
	float const GetLength() const;
	int const GetLengthSquared() const;
	int const GetTaxicabLength() const;
	float const GetOrientationRadians() const;
	float const GetOrientationDegrees() const;
	IntVec2 const GetRotated90Degrees() const;
	IntVec2 const GetRotatedMinus90Degrees() const;
	std::string GetAsText() const;

	//Mutators (non-const methods)
	//-----------------------------------------------------------------------------------------------
	void SetFromText(char const* text);
	void Rotate90Degrees();
	void RotateMinus90Degrees();

	// Operators (const)
	//-----------------------------------------------------------------------------------------------

	bool		operator==( const IntVec2& compare ) const;		// IntVec2 == IntVec2
	bool		operator!=( const IntVec2& compare ) const;		// IntVec2 != IntVec2
	const IntVec2	operator+( const IntVec2& vecToAdd ) const;		// IntVec2 + IntVec2
	const IntVec2	operator-( const IntVec2& vecToSubtract ) const;	// IntVec2 - IntVec2
	const IntVec2	operator-() const;								// -IntVec2, i.e. "unary negation"
	const IntVec2	operator*( int uniformScale ) const;			// IntVec2 * float
	const IntVec2	operator*( const IntVec2& vecToMultiply ) const;	// IntVec2 * IntVec2
	const IntVec2	operator/( int inverseScale ) const;			// IntVec2 / float

	// Operators (self-mutating / non-const)
	//-----------------------------------------------------------------------------------------------

	void		operator+=( const IntVec2& vecToAdd );				// IntVec2 += IntVec2
	void		operator-=( const IntVec2& vecToSubtract );		// IntVec2 -= IntVec2
	void		operator*=( const int uniformScale );			// IntVec2 *= float
	void		operator/=( const int uniformDivisor );		// IntVec2 /= float
	void		operator=( const IntVec2& copyFrom );				// IntVec2 = IntVec2

	// Standalone "friend" functions that are conceptually, but not actually, part of IntVec2::
	//-----------------------------------------------------------------------------------------------

	friend const IntVec2 operator*( int uniformScale, const IntVec2& vecToScale );	// float * IntVec2

	


};


