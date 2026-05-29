#pragma once
#include <string>
#include <vector>
struct HashedCaseInsensitiveString
{
	HashedCaseInsensitiveString();
	HashedCaseInsensitiveString(char const* text);
	HashedCaseInsensitiveString(std::string const& text);
	HashedCaseInsensitiveString(HashedCaseInsensitiveString const& copyFrom) = default;
	~HashedCaseInsensitiveString() {};

	unsigned int GetHash() const {return m_hash;}
	std::string const& GetOriginalString() const {return m_originalString;}
	char const* c_str() const {return m_originalString.c_str();}

	bool operator<(HashedCaseInsensitiveString const& compare) const;
	bool operator== (HashedCaseInsensitiveString const& compare) const;
	bool operator!= (HashedCaseInsensitiveString const& compare) const;
	void operator=(HashedCaseInsensitiveString const& assignFrom);

	bool operator==(char const* text) const;
	bool operator!=(char const* text) const;
	void operator=(char const* text);
	bool operator==(std::string const& text) const;
	bool operator!=(std::string const& text) const;
	void operator=(std::string const& text);



private:
	unsigned int GetHashForText(std::string const& text) const;
	unsigned int GetHashForText(char const* text) const;

private:
	unsigned int m_hash = 0;
	std::string m_originalString;
};

typedef HashedCaseInsensitiveString HashedString;

typedef std::vector<HashedCaseInsensitiveString> HashedStrings;

