#pragma once
#include <string>

struct Vec2;
class RandomNumberGenerator;
struct FloatRange
{
public: 
	float m_min = 0.f;
	float m_max = 0.f;

	static const FloatRange ZERO;
	static const FloatRange ONE;
	static const FloatRange ZERO_TO_ONE;
	static const FloatRange ONE_TO_ZERO;

public:
	FloatRange(){}
	~FloatRange() {}
	explicit FloatRange(float min, float max);
	explicit FloatRange(Vec2 const& vectorToConvert);
	explicit FloatRange(float defaultValue);

	static bool IsWithinZeroToOne(float number);
	static bool IsOnZeroToOneRange(float number);

	// Accessors (const)
	//-----------------------------------------------------------------------------------------------
	bool const IsOnRange(float number) const;
	bool const IsWithinRange(float number) const;
	bool const IsOverlapping(FloatRange const& range) const;
	float GetRandomValueInRange(RandomNumberGenerator* randomNumberGenerator = nullptr) const;
	std::string GetAsText(int numDecimals = 2) const;
	//Mutators
	//-----------------------------------------------------------------------------------------------
	void SetFromText(char const* text);
	void StretchToIncludeValue(float value);

	float FloatRangeLerp(float t) const;

	// Operators (const)
	//-----------------------------------------------------------------------------------------------

	bool				operator==(const FloatRange& compare) const;		
	bool				operator!=(const FloatRange& compare) const;		
	const FloatRange	operator+(const FloatRange& floatRangeToAdd) const;		
	const FloatRange	operator-(const FloatRange& floatRangeToSubtract) const;	
	const FloatRange	operator-() const;								
	const FloatRange	operator*(float uniformScale) const;			
	const FloatRange	operator*(const FloatRange& floatRangeToMultiply) const;	
	const FloatRange	operator/(float inverseScale) const;			

	// Operators (self-mutating / non-const)
	//-----------------------------------------------------------------------------------------------

	void		operator+=(const FloatRange& floatRangeToAdd);			
	void		operator-=(const FloatRange & floatRangeToSubtract);		 
	void		operator*=(const float uniformScale);			
	void		operator/=(const float uniformDivisor);		
	void		operator=(const FloatRange& copyFrom);				

};

