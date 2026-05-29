#pragma once
#include <string>
struct IntRange
{
public:
	int m_min = 0;
	int m_max = 0;

public:
	IntRange() {}
	~IntRange() {}
	explicit IntRange(int const& min, int const& max);

	// Accessors (const)
	//-----------------------------------------------------------------------------------------------
	bool const IsOnRange(int const& number) const;
	bool const IsWithinRange(int const& number) const;
	bool const IsOverlapping(IntRange const& range) const;
	std::string GetAsText() const;
	//Mutators
	//-----------------------------------------------------------------------------------------------
	void SetFromText(char const* text);

	// Operators (const)
	//-----------------------------------------------------------------------------------------------

	bool				operator==(const IntRange& compare) const;
	bool				operator!=(const IntRange& compare) const;
	const IntRange		operator+(const IntRange& intRangeToAdd) const;
	const IntRange		operator-(const IntRange& intRangeToSubtract) const;
	const IntRange		operator-() const;
	const IntRange		operator*(int uniformScale) const;
	const IntRange		operator*(const IntRange& intRangeToMultiply) const;
	const IntRange		operator/(int inverseScale) const;

	// Operators (self-mutating / non-const)
	//-----------------------------------------------------------------------------------------------

	void		operator+=(const IntRange& intRangeToAdd);
	void		operator-=(const IntRange& intRangeToSubtract);
	void		operator*=(const int uniformScale);
	void		operator/=(const int uniformDivisor);
	void		operator=(const IntRange& copyFrom);
};

