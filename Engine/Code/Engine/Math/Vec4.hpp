#pragma once
#include <string>
struct Rgba8;
struct Vec4
{
public:
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
	float w = 0.f;

public:
	~Vec4() {}
	Vec4() {}
	Vec4(const Vec4& copyFrom);
	explicit Vec4(float initialX, float initialY, float initialZ, float initialW);
	explicit Vec4(Rgba8 const& color);

	std::string GetAsText(int numDecimals = 2) const;

	//Mutators
	//-----------------------------------------------------------------------------------------------
	void SetFromText(char const* text);

	// Operators (const)
	//-----------------------------------------------------------------------------------------------
	bool		operator==(const Vec4& compare) const;		
	bool		operator!=(const Vec4& compare) const;		
	const Vec4	operator+(const Vec4& vecToAdd) const;		
	const Vec4	operator-(const Vec4& vecToSubtract) const;	
	const Vec4	operator-() const;							
	const Vec4	operator*(float uniformScale) const;
	const Vec4	operator/(float inverseScale) const;
	const Vec4	operator*(const Vec4& vecToMultiply) const;

	// Operators (self-mutating / non-const)
	//-----------------------------------------------------------------------------------------------
	void		operator+=(const Vec4& vecToAdd);			
	void		operator-=(const Vec4 & vecToSubtract);
	void		operator*=(const float uniformScale);		
	void		operator/=(const float uniformDivisor);
	void		operator=(const Vec4& copyFrom);

	friend const Vec4 operator*(float uniformScale, const Vec4& vecToScale);	
};

