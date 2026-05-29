#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/IntRange.hpp"

int ParseXmlAttribute(XmlElement const& element, char const* attributeName, int defaultValue)
{
	char const* value = element.Attribute(attributeName);
	if (value == nullptr)
	{
		return defaultValue;
	}

	return atoi(value);
}

char ParseXmlAttribute(XmlElement const& element, char const* attributeName, char defaultValue)
{
	char const* value = element.Attribute(attributeName);
	if (value == nullptr)
	{
		return defaultValue;
	}

	return *value;
}

bool ParseXmlAttribute(XmlElement const& element, char const* attributeName, bool defaultValue)
{
	char const* value = element.Attribute(attributeName);
	if (value == nullptr)
	{
		return defaultValue;
	}

	if (_stricmp(value, "false") == 0)
	{
		return false;
	}

	else if (_stricmp(value, "true") == 0)
	{
		return true;
	}
	
	else
	{
		return defaultValue;
	}
}

float ParseXmlAttribute(XmlElement const& element, char const* attributeName, float defaultValue)
{
	char const* value = element.Attribute(attributeName);
	if (value == nullptr)
	{
		return defaultValue;
	}

	return (float)atof(value);
}

Rgba8 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Rgba8 const& defaultValue)
{
	char const* value = element.Attribute(attributeName);
	if (value == nullptr)
	{
		return defaultValue;
	}

	Rgba8 color;
	color.SetFromText(value);
	return color;
}

Vec2 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vec2 const& defaultValue)
{
	char const* value = element.Attribute(attributeName);
	if (value == nullptr)
	{
		return defaultValue;
	}

	Vec2 newVec2;
	newVec2.SetFromText(value);
	return newVec2;
}

Vec3 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vec3 const& defaultValue)
{
	char const* value = element.Attribute(attributeName);
	if (value == nullptr)
	{
		return defaultValue;
	}
	Vec3 newVec3;
	newVec3.SetFromText(value);
	return newVec3;
}

EulerAngles ParseXmlAttribute(XmlElement const& element, char const* attributeName, EulerAngles const& defaultValue)
{
	char const* value = element.Attribute(attributeName);
	if (value == nullptr)
	{
		return defaultValue;
	}

	EulerAngles newOrientation;
	newOrientation.SetFromText(value);
	return newOrientation;
}

FloatRange ParseXmlAttribute(XmlElement const& element, char const* attributeName, FloatRange const& defaultValue)
{
	char const* value = element.Attribute(attributeName);
	if (value == nullptr)
	{
		return defaultValue;
	}

	FloatRange newRange;
	newRange.SetFromText(value);
	return newRange;
}

IntRange ParseXmlAttribute(XmlElement const& element, char const* attributeName, IntRange const& defaultValue)
{
	char const* value = element.Attribute(attributeName);
	if (value == nullptr)
	{
		return defaultValue;
	}

	IntRange newRange;
	newRange.SetFromText(value);
	return newRange;
}

IntVec2 ParseXmlAttribute(XmlElement const& element, char const* attributeName, IntVec2 const& defaultValue)
{
	char const* value = element.Attribute(attributeName);
	if (value == nullptr)
	{
		return defaultValue;
	}
	
	IntVec2 newIntVec2;
	newIntVec2.SetFromText(value);

	return newIntVec2;
}

IntVec3 ParseXmlAttribute(XmlElement const& element, char const* attributeName, IntVec3 const& defaultValue)
{
	char const* value = element.Attribute(attributeName);
	if (value == nullptr)
	{
		return defaultValue;
	}

	IntVec3 newIntVec3;
	newIntVec3.SetFromText(value);

	return newIntVec3;
}

std::string ParseXmlAttribute(XmlElement const& element, char const* attributeName, std::string const& defaultValue)
{
	char const* value = element.Attribute(attributeName);
	if (value == nullptr)
	{
		return defaultValue;
	}
	
	return Stringf(value);
}

Strings ParseXmlAttribute(XmlElement const& element, char const* attributeName, Strings const& defaultValues, char delimiter)
{
	char const* value = element.Attribute(attributeName);
	if (value == nullptr)
	{
		return defaultValues;
	}

	Strings newStrings = SplitStringOnDelimiter(Stringf(value), delimiter);
	return newStrings;
}

std::string ParseXmlAttribute(XmlElement const& element, char const* attributeName, char const* defaultValue)
{
	char const* value = element.Attribute(attributeName);
	if (value == nullptr)
	{
		return Stringf(defaultValue);
	}

	return Stringf(value);
}
