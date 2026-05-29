#include "Engine/Math/FloatRange.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"

FloatRange const FloatRange::ZERO = FloatRange();
FloatRange const FloatRange::ONE = FloatRange(1.f, 1.f);
FloatRange const FloatRange::ZERO_TO_ONE = FloatRange(0.f, 1.f);
FloatRange const FloatRange::ONE_TO_ZERO = FloatRange(1.f, 0.f);

FloatRange::FloatRange(float min, float max)
	:m_min(min),
	m_max(max)
{
}

FloatRange::FloatRange(Vec2 const& vectorToConvert)
	:m_min(vectorToConvert.x)
	,m_max(vectorToConvert.y)
{
}

FloatRange::FloatRange(float defaultValue)
	:m_min(defaultValue)
	,m_max(defaultValue)
{
}

bool FloatRange::IsWithinZeroToOne(float number)
{
	return (number > 0.f && number < 1.f );
}

bool FloatRange::IsOnZeroToOneRange(float number)
{
	return (number >= 0.f && number <= 1.f);
}

bool const FloatRange::IsOnRange(float number) const
{
	return (number >= m_min && number <= m_max) || (number <= m_min && number >= m_max);
}

bool const FloatRange::IsWithinRange(float number) const
{
	return (number > m_min && number < m_max) || (number < m_min && number > m_max);
}

bool const FloatRange::IsOverlapping(FloatRange const& range) const
{
	return (IsOnRange(range.m_min) || IsOnRange(range.m_max) || range.IsOnRange(m_min) || range.IsOnRange(m_max));
}

float FloatRange::GetRandomValueInRange(RandomNumberGenerator* randomNumberGenerator) const
{
	if (randomNumberGenerator)
	{
		return randomNumberGenerator->RollRandomFloatInRange(m_min, m_max);
	}

	RandomNumberGenerator rng;
	return rng.RollRandomFloatInRange(m_min, m_max);
}

std::string FloatRange::GetAsText(int numDecimals) const
{
	numDecimals = GetClampedInt(numDecimals, 0, 10);
	char format[64];
	std::snprintf(format, sizeof(format), "%%.%df~%%.%df", numDecimals, numDecimals);
	return Stringf(format, m_min, m_max);
}

void FloatRange::SetFromText(char const* text)
{
	Strings numsFromText = SplitStringOnDelimiter(text, '~');
	m_min = (float)(atof(numsFromText[0].c_str()));
	m_max = (float)(atof(numsFromText[1].c_str()));
}

void FloatRange::StretchToIncludeValue(float value)
{
	if (value < m_min)
	{
		m_min = value;
	}

	if (value > m_max)
	{
		m_max = value;
	}
}

float FloatRange::FloatRangeLerp(float t) const
{
	return Lerp(m_min, m_max, t);
}

bool FloatRange::operator==(const FloatRange& compare) const
{
	return (m_min == compare.m_min && m_max == compare.m_max);
}

bool FloatRange::operator!=(const FloatRange& compare) const
{
	return (m_min != compare.m_min || m_max != compare.m_max);
}

const FloatRange FloatRange::operator+(const FloatRange& floatRangeToAdd) const
{
	return FloatRange(m_min + floatRangeToAdd.m_min, m_max + floatRangeToAdd.m_max);
}

const FloatRange FloatRange::operator-(const FloatRange& floatRangeToSubtract) const
{
	return FloatRange(m_min - floatRangeToSubtract.m_min, m_max - floatRangeToSubtract.m_max);
}

const FloatRange FloatRange::operator-() const
{
	return FloatRange(-m_min,-m_max);
}

const FloatRange FloatRange::operator*(float uniformScale) const
{
	return FloatRange(m_min * uniformScale, m_max * uniformScale);
}

const FloatRange FloatRange::operator*(const FloatRange& floatRangeToMultiply) const
{
	return FloatRange(m_min * floatRangeToMultiply.m_min, m_max * floatRangeToMultiply.m_max);
}

const FloatRange FloatRange::operator/(float inverseScale) const
{
	return FloatRange(m_min / inverseScale, m_max / inverseScale);
}

void FloatRange::operator+=(const FloatRange& floatRangeToAdd)
{
	m_min += floatRangeToAdd.m_min;
	m_max += floatRangeToAdd.m_max;
}

void FloatRange::operator-=(const FloatRange& floatRangeToSubtract)
{
	m_min -= floatRangeToSubtract.m_min;
	m_max -= floatRangeToSubtract.m_max;
}

void FloatRange::operator*=(const float uniformScale)
{
	m_min *= uniformScale;
	m_max *= uniformScale;
}

void FloatRange::operator/=(const float uniformDivisor)
{
	m_min /= uniformDivisor;
	m_max /= uniformDivisor;
}

void FloatRange::operator=(const FloatRange& copyFrom)
{
	m_min = copyFrom.m_min;
	m_max = copyFrom.m_max;
}

