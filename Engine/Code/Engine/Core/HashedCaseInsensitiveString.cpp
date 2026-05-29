#include "Engine/Core/HashedCaseInsensitiveString.hpp"

HashedCaseInsensitiveString::HashedCaseInsensitiveString()
	:m_hash(0)
	,m_originalString("")
{
}

HashedCaseInsensitiveString::HashedCaseInsensitiveString(char const* text)
	:m_originalString(text)
	,m_hash(GetHashForText(text))
{
}

HashedCaseInsensitiveString::HashedCaseInsensitiveString(std::string const& text)
	:m_originalString(text)
	,m_hash(GetHashForText(text))
{
}

bool HashedCaseInsensitiveString::operator<(HashedCaseInsensitiveString const& compare) const
{
	if (m_hash < compare.m_hash)
	{
		return true;
	}

	if (m_hash > compare.m_hash)
	{
		return false;
	}

	return (_stricmp(m_originalString.c_str(), compare.m_originalString.c_str()) < 0);
}

void HashedCaseInsensitiveString::operator=(std::string const& text)
{
	m_originalString = text;
	m_hash = GetHashForText(text);
}

bool HashedCaseInsensitiveString::operator!=(std::string const& text) const
{
	return (_stricmp(m_originalString.c_str(), text.c_str()) != 0);
}

bool HashedCaseInsensitiveString::operator==(std::string const& text) const
{
	return (_stricmp(m_originalString.c_str(), text.c_str()) == 0);
}

void HashedCaseInsensitiveString::operator=(char const* text)
{
	m_originalString = text;
	m_hash = GetHashForText(text);
}

bool HashedCaseInsensitiveString::operator!=(char const* text) const
{
	return (_stricmp(m_originalString.c_str(), text) != 0);
}

bool HashedCaseInsensitiveString::operator==(char const* text) const
{
	return (_stricmp(m_originalString.c_str(), text) == 0);
}

void HashedCaseInsensitiveString::operator=(HashedCaseInsensitiveString const& assignFrom)
{
	m_hash = assignFrom.m_hash;
	m_originalString = assignFrom.m_originalString;
}

bool HashedCaseInsensitiveString::operator!=(HashedCaseInsensitiveString const& compare) const
{
	if(m_hash != compare.m_hash)
		return true;

	return (_stricmp(m_originalString.c_str(), compare.m_originalString.c_str()) != 0);
}

bool HashedCaseInsensitiveString::operator==(HashedCaseInsensitiveString const& compare) const
{
	if (m_hash != compare.m_hash)
	{
		return false;
	}

	return (_stricmp(m_originalString.c_str(), compare.m_originalString.c_str()) == 0);
}

unsigned int HashedCaseInsensitiveString::GetHashForText(std::string const& text) const
{
	return GetHashForText(text.c_str());
}

unsigned int HashedCaseInsensitiveString::GetHashForText(char const* text) const
{
	unsigned int hash = 0;
	for (const char* scan = text; *scan != '\0'; ++scan)
	{
		hash *= 31;
		hash += (unsigned int) tolower(*scan);
	}

	return hash;
}
