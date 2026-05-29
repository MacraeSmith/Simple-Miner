#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include <sstream>

void NamedStrings::PopulateFromXmlElementAttributes(XmlElement const& element)
{
	XmlAttribute const* attribute = element.FirstAttribute();
	while (attribute)
	{
		SetValue(attribute->Name(), attribute->Value());
		attribute = attribute->Next();
	}
}

Strings NamedStrings::GetMapAsEventArgsStrings() const
{
	Strings eventArgs;
	auto iter = m_keyValuePairs.begin();
	if (iter != m_keyValuePairs.end())
	{
		eventArgs.push_back(Stringf("%s=%s", iter->first.c_str(), iter->second.c_str()));
	}

	else
	{
		return eventArgs;
	}

	for (int i = 1; i < (int)m_keyValuePairs.size(); ++i)
	{
		std::advance(iter, 1);
		if (iter != m_keyValuePairs.end())
		{
			eventArgs.push_back(Stringf("%s=%s", iter->first.c_str(), iter->second.c_str()));
		}

	}
	return eventArgs;
}

std::string NamedStrings::GetMapAsString() const
{
	std::string string;
	auto iter = m_keyValuePairs.begin();
	if (iter != m_keyValuePairs.end())
	{
		string.append(Stringf("%s %s", iter->first.c_str(), iter->second.c_str()));
	}

	else
	{
		return string;
	}

	for (int i = 1; i < (int)m_keyValuePairs.size(); ++i)
	{
		std::advance(iter, 1);
		if (iter != m_keyValuePairs.end())
		{
			string.append(Stringf("%s %s", iter->first.c_str(), iter->second.c_str()));
		}

	}
	return string;
}

std::string NamedStrings::ConcatenateKeysAsString(char concatenateChar)
{
	std::string string;
	auto iter = m_keyValuePairs.begin();
	if (iter != m_keyValuePairs.end())
	{
		string.append(Stringf("%s%c", iter->first.c_str(), concatenateChar));
	}

	else
	{
		return string;
	}

	for (int i = 1; i < (int)m_keyValuePairs.size(); ++i)
	{
		std::advance(iter, 1);
		if (iter != m_keyValuePairs.end())
		{
			string.append(Stringf("%s%c", iter->first.c_str(), concatenateChar));
		}

	}
	return string;
}

void NamedStrings::SetValue(std::string const& keyName, std::string const& newValue, bool defaultLowerCase)
{
	std::string keyNameAdjusted = keyName;
	if (defaultLowerCase)
	{
		keyNameAdjusted = GetLowercase(keyName);
	}

	auto found = m_keyValuePairs.find(keyNameAdjusted);
	if (found != m_keyValuePairs.end())
	{
		found->second = newValue;
		return;
	}

	m_keyValuePairs.insert({ keyNameAdjusted, newValue });
}


std::string NamedStrings::GetValue(std::string const& keyName, std::string const& defaultValue, bool defaultLowerCase) const
{
	std::string keyNameAdjusted = keyName;
	if (defaultLowerCase)
	{
		keyNameAdjusted = GetLowercase(keyName);
	}

	auto found = m_keyValuePairs.find(keyNameAdjusted);
	if (found == m_keyValuePairs.end())
	{
		DebuggerPrintf("WARNING: keyName \"%s\" was not found)\n", keyNameAdjusted.c_str());
		return defaultValue;
	}

	return found->second;
}

bool NamedStrings::GetValue(std::string const& keyName, bool defaultValue, bool defaultLowerCase) const
{
	std::string keyNameAdjusted = keyName;
	if (defaultLowerCase)
	{
		keyNameAdjusted = GetLowercase(keyName);
	}

	auto found = m_keyValuePairs.find(keyNameAdjusted);
	if (found == m_keyValuePairs.end())
	{
		DebuggerPrintf("WARNING: keyName \"%s\" was not found)\n", keyNameAdjusted.c_str());
		return defaultValue;
	}

	if (_stricmp(found->second.c_str(), "false") == 0)
	{
		return false;
	}

	if (_stricmp(found->second.c_str(), "true") == 0)
	{
		return true;
	}

	return defaultValue;
}

int NamedStrings::GetValue(std::string const& keyName, int defaultValue, bool defaultLowerCase) const
{
	std::string keyNameAdjusted = keyName;
	if (defaultLowerCase)
	{
		keyNameAdjusted = GetLowercase(keyName);
	}

	auto found = m_keyValuePairs.find(keyNameAdjusted);
	if (found == m_keyValuePairs.end())
	{
		DebuggerPrintf("WARNING: keyName \"%s\" was not found)\n", keyNameAdjusted.c_str());
		return defaultValue;
	}

	return atoi(found->second.c_str());
}

float NamedStrings::GetValue(std::string const& keyName, float defaultValue, bool defaultLowerCase) const
{
	std::string keyNameAdjusted = keyName;
	if (defaultLowerCase)
	{
		keyNameAdjusted = GetLowercase(keyName);
	}
	auto found = m_keyValuePairs.find(keyNameAdjusted);
	if (found == m_keyValuePairs.end())
	{
		DebuggerPrintf("WARNING: keyName \"%s\" was not found)\n", keyNameAdjusted.c_str());
		return defaultValue;
	}


	return (float)atof(found->second.c_str());
}

std::string NamedStrings::GetValue(std::string const& keyName, char const* defaultValue, bool defaultLowerCase) const
{
	std::string keyNameAdjusted = keyName;
	if (defaultLowerCase)
	{
		keyNameAdjusted = GetLowercase(keyName);
	}
	auto found = m_keyValuePairs.find(keyNameAdjusted);
	if (found == m_keyValuePairs.end())
	{
		DebuggerPrintf("WARNING: keyName \"%s\" was not found)\n", keyNameAdjusted.c_str());
		return defaultValue;
	}

	return found->second;
}

Rgba8 NamedStrings::GetValue(std::string const& keyName, Rgba8 const& defaultValue, bool defaultLowerCase) const
{
	std::string keyNameAdjusted = keyName;
	if (defaultLowerCase)
	{
		keyNameAdjusted = GetLowercase(keyName);
	}
	auto found = m_keyValuePairs.find(keyNameAdjusted);
	if (found == m_keyValuePairs.end())
	{
		DebuggerPrintf("WARNING: keyName \"%s\" was not found)\n", keyNameAdjusted.c_str());
		return defaultValue;
	}

	Rgba8 newColor;
	newColor.SetFromText(found->second.c_str());
	return newColor;
}

Vec2 NamedStrings::GetValue(std::string const& keyName, Vec2 const& defaultValue, bool defaultLowerCase) const
{
	std::string keyNameAdjusted = keyName;
	if (defaultLowerCase)
	{
		keyNameAdjusted = GetLowercase(keyName);
	}
	auto found = m_keyValuePairs.find(keyNameAdjusted);
	if (found == m_keyValuePairs.end())
	{
		DebuggerPrintf("WARNING: keyName \"%s\" was not found)\n", keyNameAdjusted.c_str());
		return defaultValue;
	}

	Vec2 newVec2;
	newVec2.SetFromText(found->second.c_str());
	return newVec2;
}

Vec3 NamedStrings::GetValue(std::string const& keyName, Vec3 const& defaultValue, bool defaultLowerCase) const
{
	std::string keyNameAdjusted = keyName;
	if (defaultLowerCase)
	{
		keyNameAdjusted = GetLowercase(keyName);
	}
	auto found = m_keyValuePairs.find(keyNameAdjusted);
	if (found == m_keyValuePairs.end())
	{
		DebuggerPrintf("WARNING: keyName \"%s\" was not found)\n", keyNameAdjusted.c_str());
		return defaultValue;
	}

	Vec3 newVec3;
	newVec3.SetFromText(found->second.c_str());
	return newVec3;
}

IntVec2 NamedStrings::GetValue(std::string const& keyName, IntVec2 const& defaultValue, bool defaultLowerCase) const
{
	std::string keyNameAdjusted = keyName;
	if (defaultLowerCase)
	{
		keyNameAdjusted = GetLowercase(keyName);
	}
	auto found = m_keyValuePairs.find(keyNameAdjusted);
	if (found == m_keyValuePairs.end())
	{
		DebuggerPrintf("WARNING: keyName \"%s\" was not found)\n", keyNameAdjusted.c_str());
		return defaultValue;
	}

	IntVec2 newIntVec2;
	newIntVec2.SetFromText(found->second.c_str());
	return newIntVec2;
}

EulerAngles NamedStrings::GetValue(std::string const& keyName, EulerAngles const& defaultValue, bool defaultLowerCase) const
{
	std::string keyNameAdjusted = keyName;
	if (defaultLowerCase)
	{
		keyNameAdjusted = GetLowercase(keyName);
	}
	auto found = m_keyValuePairs.find(keyNameAdjusted);
	if (found == m_keyValuePairs.end())
	{
		DebuggerPrintf("WARNING: keyName \"%s\" was not found)\n", keyNameAdjusted.c_str());
		return defaultValue;
	}

	Strings angleStrings = SplitStringOnDelimiter(found->second, ',');
	if(angleStrings.size() < 3)
		return defaultValue;

	EulerAngles newEulerAngles;
	newEulerAngles.SetFromText(found->second.c_str());
	return newEulerAngles;
}

bool NamedStrings::HasKey(std::string const& keyName, bool defaultLowerCase) const
{
	std::string keyNameAdjusted = keyName;
	if (defaultLowerCase)
	{
		keyNameAdjusted = GetLowercase(keyName);
	}

	auto found = m_keyValuePairs.find(keyNameAdjusted);
	if (found == m_keyValuePairs.end())
	{
		return false;
	}
	return true;
}

