#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

Rgba8 const Rgba8::RED = Rgba8(255,0,0);
Rgba8 const Rgba8::GREEN = Rgba8(0, 255, 0);
Rgba8 const Rgba8::BLUE = Rgba8(0, 0, 255);
Rgba8 const Rgba8::WHITE = Rgba8(255, 255, 255);
Rgba8 const Rgba8::CYAN = Rgba8(0, 255, 226);
Rgba8 const Rgba8::MAGENTA = Rgba8(255, 0, 255);
Rgba8 const Rgba8::YELLOW = Rgba8(255, 255, 0);
Rgba8 const Rgba8::ORANGE = Rgba8(255, 190, 0);
Rgba8 const Rgba8::GREY = Rgba8(125, 125, 125);
Rgba8 const Rgba8::DARK_GREY = Rgba8(50, 50, 50);
Rgba8 const Rgba8::BLACK = Rgba8(0, 0, 0);
Rgba8 const Rgba8::TRANSLUCENT_BLACK = Rgba8(0, 0, 0, 125);
Rgba8 const Rgba8::TRANSLUCENT_GREY = Rgba8(125, 125, 125, 125);
Rgba8 const Rgba8::DEFAULT_NORMAL_MAP = Rgba8(127, 127, 255);
Rgba8 const Rgba8::DEFAULT_SPEC_GLOSS_EMIT_MAP = Rgba8(127, 127, 0);

Rgba8::Rgba8(const Rgba8& copyFrom)
	: r(copyFrom.r)
	, g(copyFrom.g)
	, b(copyFrom.b)
	, a(copyFrom.a)
{
}

Rgba8::Rgba8(unsigned char redByte, unsigned char greenByte, unsigned char blueByte, unsigned char alphaByte)
	: r(redByte)
	, g(greenByte)
	, b(blueByte)
	, a(alphaByte)
{
}

void Rgba8::GetAsFloats(float* colorAsFloats) const
{
	colorAsFloats[0] = NormalizeByte(r);
	colorAsFloats[1] = NormalizeByte(g);
	colorAsFloats[2] = NormalizeByte(b);
	colorAsFloats[3] = NormalizeByte(a);
}

std::string Rgba8::GetAsText() const
{
	return Stringf("%u,%u,%u,%u", r, g, b, a);
}


bool Rgba8::IsEqualIgnoringAlpha(Rgba8 const& compareColor)
{
	return r == compareColor.r && g == compareColor.g && b == compareColor.b;
}

Rgba8 Rgba8::GetAsDenormalizedColor(float const* colorAsFloats)
{
	Rgba8 newColor;
	newColor.r = DenormalizeByte(colorAsFloats[0]);
	newColor.g = DenormalizeByte(colorAsFloats[1]);
	newColor.b = DenormalizeByte(colorAsFloats[2]);
	newColor.a = DenormalizeByte(colorAsFloats[3]);
	return newColor;
}

Rgba8 Rgba8::GetRandomColor(RandomNumberGenerator* rng)
{
	RandomNumberGenerator randomNumberGenerator;
	if (rng != nullptr)
	{
		randomNumberGenerator = *rng;
	}

	Rgba8 randomColor(DenormalizeByte(randomNumberGenerator.RollRandomFloatZeroToOne()),
		DenormalizeByte(randomNumberGenerator.RollRandomFloatZeroToOne()),
		DenormalizeByte(randomNumberGenerator.RollRandomFloatZeroToOne()),
		DenormalizeByte(randomNumberGenerator.RollRandomFloatZeroToOne()));

	return randomColor;
}

void Rgba8::SetFromText(char const* text)
{
	Strings numsFromText = SplitStringOnDelimiter(text, ',');
	size_t numValues = numsFromText.size();
	r = static_cast<unsigned char>(GetClampedInt(atoi(numsFromText[0].c_str()), 0, 255));
	g = static_cast<unsigned char>(GetClampedInt(atoi(numsFromText[1].c_str()), 0, 255));
	b = static_cast<unsigned char>(GetClampedInt(atoi(numsFromText[2].c_str()), 0, 255));

	if (numValues > 3)
	{
		a = static_cast<unsigned char>(GetClampedInt(atoi(numsFromText[3].c_str()), 0, 255));
	}
}

Rgba8 Rgba8::ColorLerp(Rgba8 const& start, Rgba8 const& end, float fraction)
{
	float rFloat = Lerp(NormalizeByte(start.r), NormalizeByte(end.r), fraction);
	float gFloat = Lerp(NormalizeByte(start.g), NormalizeByte(end.g), fraction);
	float bFloat = Lerp(NormalizeByte(start.b), NormalizeByte(end.b), fraction);
	float aFloat = Lerp(NormalizeByte(start.a), NormalizeByte(end.a), fraction);
	return Rgba8(DenormalizeByte(rFloat), DenormalizeByte(gFloat), DenormalizeByte(bFloat), DenormalizeByte(aFloat));
}

Rgba8 Rgba8::Average4(Rgba8 a, Rgba8 b, Rgba8 c, Rgba8 d)
{
	Rgba8 out;
	out.r = (unsigned char)(((int)a.r + (int)b.r + (int)c.r + (int)d.r) / 4);
	out.g = (unsigned char)(((int)a.g + (int)b.g + (int)c.g + (int)d.g) / 4);
	out.b = (unsigned char)(((int)a.b + (int)b.b + (int)c.b + (int)d.b) / 4);
	out.a = (unsigned char)(((int)a.a + (int)b.a + (int)c.a + (int)d.a) / 4);
	return out;
}

bool Rgba8::operator==(Rgba8 const& compareColor) const
{
	return r == compareColor.r && g == compareColor.g && b == compareColor.b && a == compareColor.a;
}

bool Rgba8::operator!=(Rgba8 const& compareColor) const
{
	return r != compareColor.r || g != compareColor.g || b != compareColor.b || a != compareColor.a;
}

void Rgba8::operator=(Rgba8 const& copyFrom)
{
	r = copyFrom.r;
	g = copyFrom.g;
	b = copyFrom.b;
	a = copyFrom.a;
}

void Rgba8::operator+=(Rgba8 const& colorToAdd)
{
	float rFloat = GetClampedZeroToOne(NormalizeByte(r) + NormalizeByte(colorToAdd.r));
	float gFloat = GetClampedZeroToOne(NormalizeByte(g) + NormalizeByte(colorToAdd.g));
	float bFloat = GetClampedZeroToOne(NormalizeByte(b) + NormalizeByte(colorToAdd.b));
	float aFloat = GetClampedZeroToOne(NormalizeByte(a) + NormalizeByte(colorToAdd.a));

	r = DenormalizeByte(rFloat);
	g = DenormalizeByte(gFloat);
	b = DenormalizeByte(bFloat);
	a = DenormalizeByte(aFloat);
}

void Rgba8::operator-=(Rgba8 const& colorToSubtract)
{
	float rFloat = GetClampedZeroToOne(NormalizeByte(r) - NormalizeByte(colorToSubtract.r));
	float gFloat = GetClampedZeroToOne(NormalizeByte(g) - NormalizeByte(colorToSubtract.g));
	float bFloat = GetClampedZeroToOne(NormalizeByte(b) - NormalizeByte(colorToSubtract.b));
	float aFloat = GetClampedZeroToOne(NormalizeByte(a) - NormalizeByte(colorToSubtract.a));

	r = DenormalizeByte(rFloat);
	g = DenormalizeByte(gFloat);
	b = DenormalizeByte(bFloat);
	a = DenormalizeByte(aFloat);
}

Rgba8 Rgba8::operator-(Rgba8 const& colorToSubtract) const
{
	float rFloat = GetClampedZeroToOne(NormalizeByte(r) - NormalizeByte(colorToSubtract.r));
	float gFloat = GetClampedZeroToOne(NormalizeByte(g) - NormalizeByte(colorToSubtract.g));
	float bFloat = GetClampedZeroToOne(NormalizeByte(b) - NormalizeByte(colorToSubtract.b));
	float aFloat = GetClampedZeroToOne(NormalizeByte(a) - NormalizeByte(colorToSubtract.a));

	Rgba8 newColor;
	newColor.r = DenormalizeByte(rFloat);
	newColor.g = DenormalizeByte(gFloat);
	newColor.b = DenormalizeByte(bFloat);
	newColor.a = DenormalizeByte(aFloat);

	return newColor;
}

Rgba8 Rgba8::operator*(Rgba8 const& colorToMultiply) const
{
	Rgba8 newColor;
	newColor.r = r * colorToMultiply.r;
	newColor.g = g * colorToMultiply.g;
	newColor.b = b * colorToMultiply.b;
	newColor.a = a * colorToMultiply.a;

	return newColor;
}

void Rgba8::operator*=(float scale)
{
	float rFloat = NormalizeByte(r) * scale;
	float gFloat = NormalizeByte(g) * scale;
	float bFloat = NormalizeByte(b) * scale;
	float aFloat = NormalizeByte(a) * scale;

	r = DenormalizeByte(rFloat);
	g = DenormalizeByte(gFloat);
	b = DenormalizeByte(bFloat);
	a = DenormalizeByte(aFloat);
}


