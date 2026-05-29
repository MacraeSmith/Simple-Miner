#pragma once
#include <string>

struct IntVec2;
//-----------------------------------------------------------------------------------------------
struct Vec2
{
public: // NOTE: this is one of the few cases where we break both the "m_" naming rule AND the avoid-public-members rule
	float x = 0.f;
	float y = 0.f;

	static const Vec2 ZERO;
	static const Vec2 ONE;
	static const Vec2 ZERO_TO_ONE;
	static const Vec2 ONE_TO_ZERO;
	static const Vec2 NORTH;
	static const Vec2 EAST;
	static const Vec2 SOUTH;
	static const Vec2 WEST;

public:
	// Construction/Destruction
	~Vec2() {}												// destructor (do nothing)
	Vec2() {}												// default constructor (do nothing)
	Vec2( const Vec2& copyFrom );							// copy constructor (from another vec2)
	explicit Vec2(IntVec2 const& copyFromIntVec2);
	explicit Vec2( float initialX, float initialY );		// explicit constructor (from x, y)
	explicit Vec2(int initialX, int initialY);
	
	//Static methods (e.g creation functions)
	//-----------------------------------------------------------------------------------------------

	static Vec2 const MakeFromPolarRadians(float orientationRadians, float length = 1.f);
	static Vec2 const MakeFromPolarDegrees(float orientationDegrees, float length = 1.f);

	//Static methods for getting orientation from an input x and y without having to make a new vec2
	static float const GetOrientationRadians(float inputX, float inputY);
	static float const GetOrientationDegrees(float inputX, float inputY); 
	static float const GetPolarAngleAboutPoint(Vec2 const& point, Vec2 const& origin);
	static bool ArePointsColinear(Vec2 const& prev, Vec2 const& curr, Vec2 const& next);

	//Accessors (const methods)
	//-----------------------------------------------------------------------------------------------

	float const GetLength() const;
	float const GetLengthSquared() const;

	float const GetOrientationRadians() const;

	float const GetOrientationDegrees() const;

	Vec2 const GetRotated90Degrees() const;
	Vec2 const GetRotatedMinus90Degrees() const;
	Vec2 const GetRotatedDegrees(float deltaDegrees) const;
	Vec2 const GetRotatedRadians(float deltaRadians) const;


	Vec2 const GetClamped(float maxLength) const;
	Vec2 const GetClamped(float minLength, float maxLength) const; //clamp with max and min

	Vec2 const GetNormalized() const;
	Vec2 const GetReflected(Vec2 const& normalOfSurfaceToReflectOffOf) const;

	std::string GetAsText(int numDecimals = 2) const;

	//Mutators (non-const methods)
	//-----------------------------------------------------------------------------------------------
	void SetFromText(char const* text, char delimiterToSplitOn = ',');
	void SetOrientationRadians(float newOrientationRadians);
	void SetOrientationDegrees(float newOrientationDegrees);
	void SetPolarRadians(float newOrientationRadians, float newLength);
	void SetPolarDegrees(float newOrientationDegrees, float newLength);

	void Rotate90Degrees();
	void RotateMinus90Degrees();
	void RotateRadians(float deltaRadians);
	void RotateDegrees(float deltaDegrees);

	void SetLength(float newLength);
	void ClampLength(float maxLength);
	void ClampLength(float minLength, float maxLength); //clamp with max and min
	void Normalize();
	float NormalizeAndGetPreviousLength();
	void Reflect(Vec2 const& normalOfSurfaceToReflectOffOf);


	// Operators (const)
	//-----------------------------------------------------------------------------------------------

	bool		operator==( const Vec2& compare ) const;		// vec2 == vec2
	bool		operator!=( const Vec2& compare ) const;		// vec2 != vec2
	const Vec2	operator+( const Vec2& vecToAdd ) const;		// vec2 + vec2
	const Vec2	operator-( const Vec2& vecToSubtract ) const;	// vec2 - vec2
	const Vec2	operator-() const;								// -vec2, i.e. "unary negation"
	const Vec2	operator*( float uniformScale ) const;			// vec2 * float
	const Vec2	operator*( const Vec2& vecToMultiply ) const;	// vec2 * vec2
	const Vec2	operator/( float inverseScale ) const;			// vec2 / float

	// Operators (self-mutating / non-const)
	//-----------------------------------------------------------------------------------------------

	void		operator+=( const Vec2& vecToAdd );				// vec2 += vec2
	void		operator-=( const Vec2& vecToSubtract );		// vec2 -= vec2
	void		operator*=( const float uniformScale );			// vec2 *= float
	void		operator/=( const float uniformDivisor );		// vec2 /= float
	void		operator=( const Vec2& copyFrom );				// vec2 = vec2

	// Standalone "friend" functions that are conceptually, but not actually, part of Vec2::
	//-----------------------------------------------------------------------------------------------

	friend const Vec2 operator*( float uniformScale, const Vec2& vecToScale );	// float * vec2

	


};


