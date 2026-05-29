#pragma once
#include <string>
class RandomNumberGenerator;
struct Rgba8
{
public:
	unsigned char r = 255;
	unsigned char g = 255;
	unsigned char b = 255;
	unsigned char a = 255;

	static const Rgba8 RED;
	static const Rgba8 GREEN;
	static const Rgba8 BLUE;
	static const Rgba8 WHITE;
	static const Rgba8 CYAN;
	static const Rgba8 MAGENTA;
	static const Rgba8 YELLOW;
	static const Rgba8 ORANGE;
	static const Rgba8 GREY;
	static const Rgba8 DARK_GREY;
	static const Rgba8 BLACK;
	static const Rgba8 TRANSLUCENT_BLACK;
	static const Rgba8 TRANSLUCENT_GREY;
	static const Rgba8 DEFAULT_NORMAL_MAP;
	static const Rgba8 DEFAULT_SPEC_GLOSS_EMIT_MAP;

public:
	//Construction / Destruction
	~Rgba8() {}
	Rgba8() {}
	Rgba8(const Rgba8& copyFrom);
	explicit Rgba8(unsigned char redByte, unsigned char greenByte, unsigned char blueByte, unsigned char alphaByte = 255);

	//Accessors
	//-----------------------------------------------------------------------------------------------
	void GetAsFloats(float* colorAsFloats) const;
	std::string GetAsText() const;
	bool IsEqualIgnoringAlpha(Rgba8 const& compareColor);
	static Rgba8 GetAsDenormalizedColor(float const* colorAsFloats);
	static Rgba8 GetRandomColor(RandomNumberGenerator* rng = nullptr);

	//Mutators
	//-----------------------------------------------------------------------------------------------
	void SetFromText(char const* text);

	static Rgba8 ColorLerp(Rgba8 const& start, Rgba8 const& end, float fraction);
	static Rgba8 Average4(Rgba8 a, Rgba8 b, Rgba8 c, Rgba8 d);
	

	//Operator
	//-----------------------------------------------------------------------------------------------
	bool		operator==(Rgba8 const& compareColor) const;
	bool		operator!=(Rgba8 const& compareColor) const;
	void		operator=(Rgba8 const& copyFrom);
	void		operator+=(Rgba8 const& colorToAdd);
	void		operator*=(float scale);
	void		operator-=(Rgba8 const& colorToSubtract);
	Rgba8		operator-(Rgba8 const& colorToSubtract) const;
	Rgba8		operator*(Rgba8 const& colorToMultiply) const;
	
};

