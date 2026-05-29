#pragma once
#include "ThirdParty/TinyXML2/tinyxml2.h"
#include "Engine/Core/XmlUtils.hpp"
#include <map>
#include <string>

struct Rgba8;
struct Vec2;
struct IntVec2;
struct Vec3;

class NamedStrings
{
public:
	void PopulateFromXmlElementAttributes(XmlElement const& element);
	Strings GetMapAsEventArgsStrings() const;
	std::string GetMapAsString() const;
	std::string	ConcatenateKeysAsString(char concatenateChar = ' ');
	void		SetValue(std::string const& keyName, std::string const& newValue, bool defaultLowerCase = true);

	std::string GetValue(std::string const& keyName, std::string const& defaultValue, bool defaultLowerCase = true) const;
	bool		GetValue(std::string const& keyName, bool defaultValue, bool defaultLowerCase = true) const;
	int			GetValue(std::string const& keyName, int defaultValue, bool defaultLowerCase = true) const;
	float		GetValue(std::string const& keyName, float defaultValue, bool defaultLowerCase = true) const;
	std::string GetValue(std::string const& keyName, char const* defaultValue, bool defaultLowerCase = true) const;
	Rgba8		GetValue(std::string const& keyName, Rgba8 const& defaultValue, bool defaultLowerCase = true) const;
	Vec2		GetValue(std::string const& keyName, Vec2 const& defaultValue, bool defaultLowerCase = true) const;
	Vec3		GetValue(std::string const& keyName, Vec3 const& defaultValue, bool defaultLowerCase = true) const;
	IntVec2		GetValue(std::string const& keyName, IntVec2 const& defaultValue, bool defaultLowerCase = true) const;
	EulerAngles GetValue(std::string const& keyName, EulerAngles const& defaultValue, bool defaultLowerCase = true) const;
	bool		HasKey(std::string const& keyName, bool defaultLowerCase = true) const;

private:
	std::map<std::string, std::string> m_keyValuePairs;
};

