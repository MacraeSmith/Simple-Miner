#include "Engine/Math/IntRange.hpp"
#include "Engine/Core/StringUtils.hpp"

IntRange::IntRange(int const& min, int const& max)
	:m_min(min),
	m_max(max)
{
}

bool const IntRange::IsOnRange(int const& number) const
{
	return (number >= m_min && number <= m_max);
}

bool const IntRange::IsWithinRange(int const& number) const
{
	return (number > m_min && number < m_max);
}

bool const IntRange::IsOverlapping(IntRange const& range) const
{
	return (IsWithinRange(range.m_min) || IsWithinRange(range.m_max));
}

std::string IntRange::GetAsText() const
{
	return Stringf("%i~%i",m_min, m_max);
}

void IntRange::SetFromText(char const* text)
{
	Strings numsFromText = SplitStringOnDelimiter(text, '~');
	m_min = atoi(numsFromText[0].c_str());
	m_max = atoi(numsFromText[1].c_str());
}

bool IntRange::operator==(const IntRange& compare) const
{
	return (m_min == compare.m_min && m_max == compare.m_max);
}

bool IntRange::operator!=(const IntRange& compare) const
{
	return (m_min != compare.m_min || m_max != compare.m_max);
}

const IntRange IntRange::operator+(const IntRange& intRangeToAdd) const
{
	return IntRange(m_min + intRangeToAdd.m_min, m_max + intRangeToAdd.m_max);
}

const IntRange IntRange::operator-(const IntRange& intRangeToSubtract) const
{
	return IntRange(m_min - intRangeToSubtract.m_min, m_max - intRangeToSubtract.m_max);
}

const IntRange IntRange::operator-() const
{
	return IntRange(-m_min, -m_max);
}

const IntRange IntRange::operator*(int uniformScale) const
{
	return IntRange(m_min * uniformScale, m_max * uniformScale);
}

const IntRange IntRange::operator*(const IntRange& intRangeToMultiply) const
{
	return IntRange(m_min * intRangeToMultiply.m_min, m_max * intRangeToMultiply.m_max);
}

const IntRange IntRange::operator/(int inverseScale) const
{
	return IntRange(m_min / inverseScale, m_max / inverseScale);
}

void IntRange::operator+=(const IntRange& intRangeToAdd)
{
	m_min += intRangeToAdd.m_min;
	m_max += intRangeToAdd.m_max;
}

void IntRange::operator-=(const IntRange& intRangeToSubtract)
{
	m_min -= intRangeToSubtract.m_min;
	m_max -= intRangeToSubtract.m_max;
}

void IntRange::operator*=(const int uniformScale)
{
	m_min *= uniformScale;
	m_max *= uniformScale;
}

void IntRange::operator/=(const int uniformDivisor)
{
	m_min /= uniformDivisor;
	m_max /= uniformDivisor;
}

void IntRange::operator=(const IntRange& copyFrom)
{
	m_min = copyFrom.m_min;
	m_max = copyFrom.m_max;
}
