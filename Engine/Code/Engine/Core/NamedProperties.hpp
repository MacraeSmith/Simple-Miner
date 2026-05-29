#pragma once
#include "HashedCaseInsensitiveString.hpp"
#include "ThirdParty/TinyXML2/tinyxml2.h"
#include "Engine/Core/XmlUtils.hpp"
#include <map>
#include <string>
#include <typeinfo>

struct Rgba8;
struct Vec2;
struct IntVec2;
struct Vec3;
struct EulerAngles;

struct TypedPropertyBase
{
	virtual ~TypedPropertyBase() {};
};

template<typename T>
struct TypedProperty : public TypedPropertyBase
{
	TypedProperty(T const& data)
		:m_data(data) {
	};

	T m_data;
};

class NamedProperties
{
public:
	
	~NamedProperties();
	template<typename T>
	void	SetValue(HashedCaseInsensitiveString const& key, T const& newValue)
	{
		auto found = m_keyValuePairs.find(key);
		if (found == m_keyValuePairs.end())
		{
			m_keyValuePairs[key] = new TypedProperty<T>(newValue);
			return;
		}

		else
		{
			TypedProperty<T>* asMatchingType = dynamic_cast<TypedProperty<T>*>(found->second);
			if (asMatchingType)
			{
				asMatchingType->m_data = newValue;
				return;
			}

			else
			{
				delete(found->second);
				found->second = new TypedProperty<T>(newValue);
			}
		}
	}

	void	SetValue(HashedCaseInsensitiveString const& key, char const* newValue);

	template<typename T>
	T		GetValue(HashedCaseInsensitiveString const& key, T defaultValue) const
	{
		auto found = m_keyValuePairs.find(key);
		if (found != m_keyValuePairs.end())
		{
			TypedProperty<T>* asMatchingType = dynamic_cast<TypedProperty<T>*>(found->second);
			if (asMatchingType)
			{
				return asMatchingType->m_data;
			}
		}

		return defaultValue;
	}

	std::string		GetValue(HashedCaseInsensitiveString const& key, char const* defaultValue) const;

	bool			GetValue(HashedCaseInsensitiveString const& key, bool defaultValue, bool allowConversion = true) const;

	int				GetValue(HashedCaseInsensitiveString const& key, int defaultValue, bool allowConversion = true) const;

	float			GetValue(HashedCaseInsensitiveString const& key, float defaultValue, bool allowConversion = true) const;

	Rgba8			GetValue(HashedCaseInsensitiveString const& key, Rgba8 const& defaultValue, bool allowConversion = true) const;

	Vec2			GetValue(HashedCaseInsensitiveString const& key, Vec2 const& defaultValue, bool allowConversion = true) const;

	Vec3			GetValue(HashedCaseInsensitiveString const& key, Vec3 const& defaultValue, bool allowConversion = true) const;

	IntVec2			GetValue(HashedCaseInsensitiveString const& key, IntVec2 const& defaultValue, bool allowConversion = true) const;

	EulerAngles		GetValue(HashedCaseInsensitiveString const& key, EulerAngles const& defaultValue, bool allowConversion = true) const;

	bool HasKey(HashedCaseInsensitiveString const& key) const;

	void PopulateFromXmlElementAttributes(XmlElement const& element);
	std::string	ConcatenateKeysAsString(char concatenateChar = ' ');
	Strings		GetMapStringsAsEventArgsStrings() const;

private:
	std::map<HashedCaseInsensitiveString, TypedPropertyBase*> m_keyValuePairs;
};

