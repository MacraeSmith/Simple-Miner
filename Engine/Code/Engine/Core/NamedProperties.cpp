#include "Engine/Core/NamedProperties.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include <limits>
#include <cmath>
#include <typeinfo>


NamedProperties::~NamedProperties()
{
	for (const auto& [key, value] : m_keyValuePairs)
	{
		auto found = m_keyValuePairs.find(key);
		if (found != m_keyValuePairs.end())
		{
			delete found->second;
			found->second = nullptr;
		}
	}
}

void NamedProperties::SetValue(HashedCaseInsensitiveString const& key, char const* newValue)
{
	auto found = m_keyValuePairs.find(key);

	if (found == m_keyValuePairs.end())
	{
		m_keyValuePairs[key] = new TypedProperty<std::string>(newValue);
		return;
	}

	else
	{
		TypedProperty<std::string>* asMatchingType = dynamic_cast<TypedProperty<std::string>*>(found->second);
		if (asMatchingType)
		{
			asMatchingType->m_data = newValue;
			return;
		}

		else
		{
			delete found->second;
			found->second = new TypedProperty<std::string>(newValue);
		}
	}
}


std::string NamedProperties::GetValue(HashedCaseInsensitiveString const& key, char const* defaultValue) const
{
	auto found = m_keyValuePairs.find(key);
	if (found != m_keyValuePairs.end())
	{
		TypedProperty<std::string>* asMatchingType = dynamic_cast<TypedProperty<std::string>*>(found->second);
		if (asMatchingType)
		{
			return asMatchingType->m_data;
		}
	}

	return defaultValue;
}

bool NamedProperties::GetValue(HashedCaseInsensitiveString const& key, bool defaultValue, bool allowConversion) const
{
	auto found = m_keyValuePairs.find(key);
	if (found != m_keyValuePairs.end())
	{
		TypedProperty<bool>* asMatchingType = dynamic_cast<TypedProperty<bool>*>(found->second);
		if (asMatchingType)
		{
			return asMatchingType->m_data;
		}

		else if (allowConversion)
		{
			TypedProperty<std::string>* asStringType = dynamic_cast<TypedProperty<std::string>*>(found->second);
			if (asStringType)
			{
				if (_stricmp(asStringType->m_data.c_str(), "false") == 0)
				{
					return false;
				}

				if (_stricmp(asStringType->m_data.c_str(), "true") == 0)
				{
					return true;
				}
			}

			TypedProperty<HashedCaseInsensitiveString>* asHashedStringType = dynamic_cast<TypedProperty<HashedCaseInsensitiveString>*>(found->second);
			if (asHashedStringType)
			{
				HashedCaseInsensitiveString const& hashedString = asHashedStringType->m_data;
				std::string const& originalString = hashedString.GetOriginalString();
				if (_stricmp(originalString.c_str(), "false") == 0)
				{
					return false;
				}

				if (_stricmp(originalString.c_str(), "true") == 0)
				{
					return true;
				}
			}
		}
	}

	return defaultValue;
}

int NamedProperties::GetValue(HashedCaseInsensitiveString const& key, int defaultValue, bool allowConversion) const
{
	auto found = m_keyValuePairs.find(key);
	if (found != m_keyValuePairs.end())
	{
		// int
		{
			TypedProperty<int>* asMatchingType = dynamic_cast<TypedProperty<int>*>(found->second);
			if (asMatchingType)
			{
				return asMatchingType->m_data;
			}
		}

		if (allowConversion)
		{
			// std::string
			{
				TypedProperty<std::string>* asType = dynamic_cast<TypedProperty<std::string>*>(found->second);
				if (asType)
				{
					return atoi(asType->m_data.c_str());
				}
			}

			// HashedCaseInsensitiveString
			{
				TypedProperty<HashedCaseInsensitiveString>* asType = dynamic_cast<TypedProperty<HashedCaseInsensitiveString>*>(found->second);
				if (asType)
				{
					return atoi(asType->m_data.GetOriginalString().c_str());
				}
			}

			// signed char
			{
				TypedProperty<signed char>* asType = dynamic_cast<TypedProperty<signed char>*>(found->second);
				if (asType)
				{
					return static_cast<int>(asType->m_data);
				}
			}

			// unsigned char
			{
				TypedProperty<unsigned char>* asType = dynamic_cast<TypedProperty<unsigned char>*>(found->second);
				if (asType)
				{
					return static_cast<int>(asType->m_data);
				}
			}

			// short
			{
				TypedProperty<short>* asType = dynamic_cast<TypedProperty<short>*>(found->second);
				if (asType)
				{
					return static_cast<int>(asType->m_data);
				}
			}

			// unsigned short
			{
				TypedProperty<unsigned short>* asType = dynamic_cast<TypedProperty<unsigned short>*>(found->second);
				if (asType)
				{
					return static_cast<int>(asType->m_data);
				}
			}

			// unsigned int
			{
				TypedProperty<unsigned int>* asType = dynamic_cast<TypedProperty<unsigned int>*>(found->second);
				if (asType)
				{
					if (asType->m_data <= static_cast<unsigned int>(std::numeric_limits<int>::max()))
					{
						return static_cast<int>(asType->m_data);
					}

					DebuggerPrintf("WARNING: value at key: %s could not convert to int without losing data: %s\n", key.GetOriginalString().c_str(), typeid(*found->second).name());
					return defaultValue;
				}
			}

			// long
			{
				TypedProperty<long>* asType = dynamic_cast<TypedProperty<long>*>(found->second);
				if (asType)
				{
					if (asType->m_data >= static_cast<long>(std::numeric_limits<int>::min()) &&
						asType->m_data <= static_cast<long>(std::numeric_limits<int>::max()))
					{
						return static_cast<int>(asType->m_data);
					}

					DebuggerPrintf("WARNING: value at key: %s could not convert to int without losing data: %s\n", key.GetOriginalString().c_str(), typeid(*found->second).name());
					return defaultValue;
				}
			}

			// unsigned long
			{
				TypedProperty<unsigned long>* asType = dynamic_cast<TypedProperty<unsigned long>*>(found->second);
				if (asType)
				{
					if (asType->m_data <= static_cast<unsigned long>(std::numeric_limits<int>::max()))
					{
						return static_cast<int>(asType->m_data);
					}

					DebuggerPrintf("WARNING: value at key: %s could not convert to int without losing data: %s\n", key.GetOriginalString().c_str(), typeid(*found->second).name());
					return defaultValue;
				}
			}

			// long long
			{
				TypedProperty<long long>* asType = dynamic_cast<TypedProperty<long long>*>(found->second);
				if (asType)
				{
					if (asType->m_data >= static_cast<long long>(std::numeric_limits<int>::min()) &&
						asType->m_data <= static_cast<long long>(std::numeric_limits<int>::max()))
					{
						return static_cast<int>(asType->m_data);
					}

					DebuggerPrintf("WARNING: value at key: %s could not convert to int without losing data: %s\n", key.GetOriginalString().c_str(), typeid(*found->second).name());
					return defaultValue;
				}
			}

			// unsigned long long
			{
				TypedProperty<unsigned long long>* asType = dynamic_cast<TypedProperty<unsigned long long>*>(found->second);
				if (asType)
				{
					if (asType->m_data <= static_cast<unsigned long long>(std::numeric_limits<int>::max()))
					{
						return static_cast<int>(asType->m_data);
					}

					DebuggerPrintf("WARNING: value at key: %s could not convert to int without losing data: %s\n", key.GetOriginalString().c_str(), typeid(*found->second).name());
					return defaultValue;
				}
			}

			// float
			{
				TypedProperty<float>* asType = dynamic_cast<TypedProperty<float>*>(found->second);
				if (asType)
				{
					float value = asType->m_data;

					if (value >= static_cast<float>(std::numeric_limits<int>::min()) &&
						value <= static_cast<float>(std::numeric_limits<int>::max()))
					{
						int converted = static_cast<int>(value);
						if (static_cast<float>(converted) == value)
						{
							return converted;
						}
					}

					DebuggerPrintf("WARNING: value at key: %s could not convert to int without losing data: %s\n", key.GetOriginalString().c_str(), typeid(*found->second).name());
					return defaultValue;
				}
			}

			// double
			{
				TypedProperty<double>* asType = dynamic_cast<TypedProperty<double>*>(found->second);
				if (asType)
				{
					double value = asType->m_data;

					if (value >= static_cast<double>(std::numeric_limits<int>::min()) &&
						value <= static_cast<double>(std::numeric_limits<int>::max()))
					{
						int converted = static_cast<int>(value);
						if (static_cast<double>(converted) == value)
						{
							return converted;
						}
					}

					DebuggerPrintf("WARNING: value at key: %s could not convert to int without losing data: %s\n", key.GetOriginalString().c_str(), typeid(*found->second).name());
					return defaultValue;
				}
			}

			// long double
			{
				TypedProperty<long double>* asType = dynamic_cast<TypedProperty<long double>*>(found->second);
				if (asType)
				{
					long double value = asType->m_data;

					if (value >= static_cast<long double>(std::numeric_limits<int>::min()) &&
						value <= static_cast<long double>(std::numeric_limits<int>::max()))
					{
						int converted = static_cast<int>(value);
						if (static_cast<long double>(converted) == value)
						{
							return converted;
						}
					}

					DebuggerPrintf("WARNING: value at key: %s could not convert to int without losing data: %s\n", key.GetOriginalString().c_str(), typeid(*found->second).name());
					return defaultValue;
				}
			}
		}

		DebuggerPrintf("WARNING: value at key: %s had non-convertible-to-int type: %s\n",key.GetOriginalString().c_str(),typeid(*found->second).name());
	}

	return defaultValue;
}

float NamedProperties::GetValue(HashedCaseInsensitiveString const& key, float defaultValue, bool allowConversion) const
{
	auto found = m_keyValuePairs.find(key);
	if (found != m_keyValuePairs.end())
	{
		// float
		{
			TypedProperty<float>* asType = dynamic_cast<TypedProperty<float>*>(found->second);
			if (asType)
			{
				return asType->m_data;
			}
		}

		if (allowConversion)
		{
			// std::string
			{
				TypedProperty<std::string>* asType = dynamic_cast<TypedProperty<std::string>*>(found->second);
				if (asType)
				{
					return static_cast<float>(atof(asType->m_data.c_str()));
				}
			}

			// HashedCaseInsensitiveString
			{
				TypedProperty<HashedCaseInsensitiveString>* asType = dynamic_cast<TypedProperty<HashedCaseInsensitiveString>*>(found->second);
				if (asType)
				{
					return static_cast<float>(atof(asType->m_data.GetOriginalString().c_str()));
				}
			}

			// int
			{
				TypedProperty<int>* asType = dynamic_cast<TypedProperty<int>*>(found->second);
				if (asType)
				{
					return static_cast<float>(asType->m_data);
				}
			}

			// signed char
			{
				TypedProperty<signed char>* asType = dynamic_cast<TypedProperty<signed char>*>(found->second);
				if (asType)
				{
					return static_cast<float>(asType->m_data);
				}
			}

			// unsigned char
			{
				TypedProperty<unsigned char>* asType = dynamic_cast<TypedProperty<unsigned char>*>(found->second);
				if (asType)
				{
					return static_cast<float>(asType->m_data);
				}
			}

			// short
			{
				TypedProperty<short>* asType = dynamic_cast<TypedProperty<short>*>(found->second);
				if (asType)
				{
					return static_cast<float>(asType->m_data);
				}
			}

			// unsigned short
			{
				TypedProperty<unsigned short>* asType = dynamic_cast<TypedProperty<unsigned short>*>(found->second);
				if (asType)
				{
					return static_cast<float>(asType->m_data);
				}
			}

			// unsigned int (only if exactly representable)
			{
				TypedProperty<unsigned int>* asType = dynamic_cast<TypedProperty<unsigned int>*>(found->second);
				if (asType)
				{
					float converted = static_cast<float>(asType->m_data);
					if (static_cast<unsigned int>(converted) == asType->m_data)
					{
						return converted;
					}

					DebuggerPrintf("WARNING: value at key: %s could not convert to float without losing data: %s\n", key.GetOriginalString().c_str(), typeid(*found->second).name());
					return defaultValue;
				}
			}

			// long long
			{
				TypedProperty<long long>* asType = dynamic_cast<TypedProperty<long long>*>(found->second);
				if (asType)
				{
					float converted = static_cast<float>(asType->m_data);
					if (static_cast<long long>(converted) == asType->m_data)
					{
						return converted;
					}

					DebuggerPrintf("WARNING: value at key: %s could not convert to float without losing data: %s\n", key.GetOriginalString().c_str(), typeid(*found->second).name());
					return defaultValue;
				}
			}

			// unsigned long long
			{
				TypedProperty<unsigned long long>* asType = dynamic_cast<TypedProperty<unsigned long long>*>(found->second);
				if (asType)
				{
					float converted = static_cast<float>(asType->m_data);
					if (static_cast<unsigned long long>(converted) == asType->m_data)
					{
						return converted;
					}

					DebuggerPrintf("WARNING: value at key: %s could not convert to float without losing data: %s\n", key.GetOriginalString().c_str(), typeid(*found->second).name());
					return defaultValue;
				}
			}

			// double (only if exactly representable)
			{
				TypedProperty<double>* asType = dynamic_cast<TypedProperty<double>*>(found->second);
				if (asType)
				{
					float converted = static_cast<float>(asType->m_data);
					if (static_cast<double>(converted) == asType->m_data)
					{
						return converted;
					}

					DebuggerPrintf("WARNING: value at key: %s could not convert to float without losing data: %s\n", key.GetOriginalString().c_str(), typeid(*found->second).name());
					return defaultValue;
				}
			}

			// long double
			{
				TypedProperty<long double>* asType = dynamic_cast<TypedProperty<long double>*>(found->second);
				if (asType)
				{
					float converted = static_cast<float>(asType->m_data);
					if (static_cast<long double>(converted) == asType->m_data)
					{
						return converted;
					}

					DebuggerPrintf("WARNING: value at key: %s could not convert to float without losing data: %s\n", key.GetOriginalString().c_str(), typeid(*found->second).name());
					return defaultValue;
				}
			}
		}

		DebuggerPrintf("WARNING: value at key: %s had non-convertible-to-float type: %s\n",key.GetOriginalString().c_str(),typeid(*found->second).name());
	}

	return defaultValue;
}

Rgba8 NamedProperties::GetValue(HashedCaseInsensitiveString const& key, Rgba8 const& defaultValue, bool allowConversion) const
{
	auto found = m_keyValuePairs.find(key);
	if (found != m_keyValuePairs.end())
	{
		// int
		{
			TypedProperty<Rgba8>* asMatchingType = dynamic_cast<TypedProperty<Rgba8>*>(found->second);
			if (asMatchingType)
			{
				return asMatchingType->m_data;
			}
		}

		if (allowConversion)
		{
			// std::string
			{
				TypedProperty<std::string>* asType = dynamic_cast<TypedProperty<std::string>*>(found->second);
				if (asType)
				{
					Rgba8 newColor;
					newColor.SetFromText(asType->m_data.c_str());
					return newColor;
				}
			}

			// HashedCaseInsensitiveString
			{
				TypedProperty<HashedCaseInsensitiveString>* asType = dynamic_cast<TypedProperty<HashedCaseInsensitiveString>*>(found->second);
				if (asType)
				{

					Rgba8 newColor;
					newColor.SetFromText(asType->m_data.GetOriginalString().c_str());
					return newColor;
				}
			}
		}
	}

	return defaultValue;
}

Vec2 NamedProperties::GetValue(HashedCaseInsensitiveString const& key, Vec2 const& defaultValue, bool allowConversion) const
{
	auto found = m_keyValuePairs.find(key);
	if (found != m_keyValuePairs.end())
	{
		// int
		{
			TypedProperty<Vec2>* asMatchingType = dynamic_cast<TypedProperty<Vec2>*>(found->second);
			if (asMatchingType)
			{
				return asMatchingType->m_data;
			}
		}

		if (allowConversion)
		{
			// std::string
			{
				TypedProperty<std::string>* asType = dynamic_cast<TypedProperty<std::string>*>(found->second);
				if (asType)
				{
					Vec2 value;
					value.SetFromText(asType->m_data.c_str());
					return value;
				}
			}

			// HashedCaseInsensitiveString
			{
				TypedProperty<HashedCaseInsensitiveString>* asType = dynamic_cast<TypedProperty<HashedCaseInsensitiveString>*>(found->second);
				if (asType)
				{

					Vec2 value;
					value.SetFromText(asType->m_data.GetOriginalString().c_str());
					return value;
				}
			}
		}
	}

	return defaultValue;
}

Vec3 NamedProperties::GetValue(HashedCaseInsensitiveString const& key, Vec3 const& defaultValue, bool allowConversion) const
{
	auto found = m_keyValuePairs.find(key);
	if (found != m_keyValuePairs.end())
	{
		// int
		{
			TypedProperty<Vec3>* asMatchingType = dynamic_cast<TypedProperty<Vec3>*>(found->second);
			if (asMatchingType)
			{
				return asMatchingType->m_data;
			}
		}

		if (allowConversion)
		{
			// std::string
			{
				TypedProperty<std::string>* asType = dynamic_cast<TypedProperty<std::string>*>(found->second);
				if (asType)
				{
					Vec3 value;
					value.SetFromText(asType->m_data.c_str());
					return value;
				}
			}

			// HashedCaseInsensitiveString
			{
				TypedProperty<HashedCaseInsensitiveString>* asType = dynamic_cast<TypedProperty<HashedCaseInsensitiveString>*>(found->second);
				if (asType)
				{

					Vec3 value;
					value.SetFromText(asType->m_data.GetOriginalString().c_str());
					return value;
				}
			}
		}
	}

	return defaultValue;
}

IntVec2 NamedProperties::GetValue(HashedCaseInsensitiveString const& key, IntVec2 const& defaultValue, bool allowConversion) const
{
	auto found = m_keyValuePairs.find(key);
	if (found != m_keyValuePairs.end())
	{
		// int
		{
			TypedProperty<IntVec2>* asMatchingType = dynamic_cast<TypedProperty<IntVec2>*>(found->second);
			if (asMatchingType)
			{
				return asMatchingType->m_data;
			}
		}

		if (allowConversion)
		{
			// std::string
			{
				TypedProperty<std::string>* asType = dynamic_cast<TypedProperty<std::string>*>(found->second);
				if (asType)
				{
					IntVec2 value;
					value.SetFromText(asType->m_data.c_str());
					return value;
				}
			}

			// HashedCaseInsensitiveString
			{
				TypedProperty<HashedCaseInsensitiveString>* asType = dynamic_cast<TypedProperty<HashedCaseInsensitiveString>*>(found->second);
				if (asType)
				{

					IntVec2 value;
					value.SetFromText(asType->m_data.GetOriginalString().c_str());
					return value;
				}
			}
		}
	}

	return defaultValue;
}

EulerAngles NamedProperties::GetValue(HashedCaseInsensitiveString const& key, EulerAngles const& defaultValue, bool allowConversion) const
{
	auto found = m_keyValuePairs.find(key);
	if (found != m_keyValuePairs.end())
	{
		// int
		{
			TypedProperty<EulerAngles>* asMatchingType = dynamic_cast<TypedProperty<EulerAngles>*>(found->second);
			if (asMatchingType)
			{
				return asMatchingType->m_data;
			}
		}

		if (allowConversion)
		{
			// std::string
			{
				TypedProperty<std::string>* asType = dynamic_cast<TypedProperty<std::string>*>(found->second);
				if (asType)
				{
					EulerAngles value;
					value.SetFromText(asType->m_data.c_str());
					return value;
				}
			}

			// HashedCaseInsensitiveString
			{
				TypedProperty<HashedCaseInsensitiveString>* asType = dynamic_cast<TypedProperty<HashedCaseInsensitiveString>*>(found->second);
				if (asType)
				{

					EulerAngles value;
					value.SetFromText(asType->m_data.GetOriginalString().c_str());
					return value;
				}
			}
		}
	}

	return defaultValue;
}

bool NamedProperties::HasKey(HashedCaseInsensitiveString const& key) const
{
	auto found = m_keyValuePairs.find(key);
	if (found == m_keyValuePairs.end())
	{
		return false;
	}
	return true;
}

void NamedProperties::PopulateFromXmlElementAttributes(XmlElement const& element)
{
	XmlAttribute const* attribute = element.FirstAttribute();
	while (attribute)
	{
		SetValue(attribute->Name(), attribute->Value());
		attribute = attribute->Next();
	}
}

std::string NamedProperties::ConcatenateKeysAsString(char concatenateChar)
{
	std::string string;
	auto iter = m_keyValuePairs.begin();
	if (iter != m_keyValuePairs.end())
	{
		string.append(Stringf("%s%c", iter->first.GetOriginalString().c_str(), concatenateChar));
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
			string.append(Stringf("%s%c", iter->first.GetOriginalString().c_str(), concatenateChar));
		}

	}
	return string;
}

Strings NamedProperties::GetMapStringsAsEventArgsStrings() const
{
	Strings eventArgStrings;
	auto iter = m_keyValuePairs.begin();
	if (iter != m_keyValuePairs.end())
	{
		TypedProperty<std::string>* asString = dynamic_cast<TypedProperty<std::string>*>(iter->second);
		if (asString)
		{
			eventArgStrings.push_back(Stringf("%s=%s", iter->first.c_str(), asString->m_data.c_str()));
		}
		else
		{
			TypedProperty<HashedCaseInsensitiveString>* asHashedString = dynamic_cast<TypedProperty<HashedCaseInsensitiveString>*>(iter->second);
			if (asHashedString)
			{
				eventArgStrings.push_back(Stringf("%s=%s", iter->first.c_str(), asHashedString->m_data.c_str()));
			}
		}
	}

	else
	{
		return eventArgStrings;
	}

	for (int i = 1; i < (int)m_keyValuePairs.size(); ++i)
	{
		std::advance(iter, 1);
		if (iter != m_keyValuePairs.end())
		{
			TypedProperty<std::string>* asString = dynamic_cast<TypedProperty<std::string>*>(iter->second);
			if (asString)
			{
				eventArgStrings.push_back(Stringf("%s=%s", iter->first.c_str(), asString->m_data.c_str()));
				continue;
			}

			TypedProperty<HashedCaseInsensitiveString>* asHashedString = dynamic_cast<TypedProperty<HashedCaseInsensitiveString>*>(iter->second);
			if (asHashedString)
			{
				eventArgStrings.push_back(Stringf("%s=%s", iter->first.c_str(), asHashedString->m_data.c_str()));
				continue;
			}
		}

	}
	return eventArgStrings;
}
