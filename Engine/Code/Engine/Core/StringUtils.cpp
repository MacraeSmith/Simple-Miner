#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include <stdarg.h>


//-----------------------------------------------------------------------------------------------
constexpr int STRINGF_STACK_LOCAL_TEMP_LENGTH = 2048;


//-----------------------------------------------------------------------------------------------
const std::string Stringf( char const* format, ... )
{
	char textLiteral[ STRINGF_STACK_LOCAL_TEMP_LENGTH ];
	va_list variableArgumentList;
	va_start( variableArgumentList, format );
	vsnprintf_s( textLiteral, STRINGF_STACK_LOCAL_TEMP_LENGTH, _TRUNCATE, format, variableArgumentList );	
	va_end( variableArgumentList );
	textLiteral[ STRINGF_STACK_LOCAL_TEMP_LENGTH - 1 ] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

	return std::string( textLiteral );
}


//-----------------------------------------------------------------------------------------------
const std::string Stringf( int maxLength, char const* format, ... )
{
	char textLiteralSmall[ STRINGF_STACK_LOCAL_TEMP_LENGTH ];
	char* textLiteral = textLiteralSmall;
	if( maxLength > STRINGF_STACK_LOCAL_TEMP_LENGTH )
		textLiteral = new char[ maxLength ];

	va_list variableArgumentList;
	va_start( variableArgumentList, format );
	vsnprintf_s( textLiteral, maxLength, _TRUNCATE, format, variableArgumentList );	
	va_end( variableArgumentList );
	textLiteral[ maxLength - 1 ] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

	std::string returnValue( textLiteral );
	if( maxLength > STRINGF_STACK_LOCAL_TEMP_LENGTH )
		delete[] textLiteral;

	return returnValue;
}


Strings SplitStringOnDelimiter(std::string const& originalString, char delimiterToSplitOn, bool cutOutLeadingAndTrailingWhiteSpace)
{
	Strings splitStrings;
	size_t startPos = 0;
	size_t nextDelimPos = 0;
	size_t length;
	std::string stringToAdd;

	while (nextDelimPos < originalString.length())
	{
		nextDelimPos = originalString.find_first_of(delimiterToSplitOn, startPos);
		length = nextDelimPos - startPos;
		stringToAdd = std::string(originalString, startPos, length);
		splitStrings.push_back(stringToAdd);
		startPos = nextDelimPos + 1;
	}

	if (cutOutLeadingAndTrailingWhiteSpace)
	{
		CutOutLeadingAndTrailingWhiteSpace(splitStrings);
	}

	return splitStrings;
}

Strings SplitStringOnFirstDelimiter(std::string const& originalString, char delimiterToSplitOn, bool cutOutLeadingAndTrailingWhiteSpace)
{
	Strings splitStrings;

	size_t firstDelimPos = originalString.find_first_of(delimiterToSplitOn);
	std::string stringToAdd = std::string(originalString, 0, firstDelimPos);
	splitStrings.push_back(stringToAdd);

	if (firstDelimPos < originalString.length()) // checks to make sure delimiter actually existed in the string
	{
		stringToAdd = std::string(originalString, firstDelimPos + 1, originalString.length());
		splitStrings.push_back(stringToAdd);
	}

	if (cutOutLeadingAndTrailingWhiteSpace)
	{
		CutOutLeadingAndTrailingWhiteSpace(splitStrings);
	}

	return splitStrings;
}

Strings SplitStringOnDelimiter(std::string const& originalString, char delimiterToSplitOn, char delimiterShelfToSkip, bool cutOutLeadingAndTrailingWhiteSpace)
{
	Strings splitStrings;
	size_t startPos = 0;
	size_t nextDelimPos = 0;
	size_t startShelfPos = 0;
	size_t nextShelfPos = 0;
	size_t length;
	std::string stringToAdd;
	bool shouldUpdateShelfLocations = true;

	//splits up strings based on delimiter, but skips sections shelved by delimiterToSkip
	while (nextDelimPos < originalString.length())
	{
		if (shouldUpdateShelfLocations)
		{
			startShelfPos = originalString.find_first_of(delimiterShelfToSkip, startShelfPos);
			nextShelfPos = originalString.find_first_of(delimiterShelfToSkip, startShelfPos + 1);
			shouldUpdateShelfLocations = false;
		}

		nextDelimPos = originalString.find_first_of(delimiterToSplitOn, startPos);

		if (nextDelimPos > startShelfPos && nextDelimPos < nextShelfPos)
		{
			nextDelimPos = originalString.find_first_of(delimiterToSplitOn, nextShelfPos);
			startShelfPos = nextShelfPos + 1;
			shouldUpdateShelfLocations = true;
		}

		length = nextDelimPos - startPos;
		stringToAdd = std::string(originalString, startPos, length);
		splitStrings.push_back(stringToAdd);
		startPos = nextDelimPos + 1;
	}


	if (cutOutLeadingAndTrailingWhiteSpace)
	{
		CutOutLeadingAndTrailingWhiteSpace(splitStrings);
	}

	//Removes all characters of charShelfToSkip
	for (int stringNum = 0; stringNum < (int)splitStrings.size(); ++stringNum)
	{
		RemoveAllCharactersOfType(splitStrings[stringNum], delimiterShelfToSkip);
	}

	return splitStrings;
}


void CutOutLeadingAndTrailingWhiteSpace(Strings& stringsToEdit)
{
	for (int stringIndex = 0; stringIndex < (int)stringsToEdit.size(); ++stringIndex)
	{
		std::string& currentString = stringsToEdit[stringIndex];

		int index = 0;
		while (currentString.size() > 0 && isspace(currentString[index]))
		{
			index++;
		}
		
		if (index > 0)
		{
			currentString.erase(0, index);
		}

		int endIndex = (int)currentString.size() - 1;
		index = endIndex;
		while (currentString.size() > 0 && isspace(currentString[index]))
		{
			index--;
		}

		if (currentString.size() <= 0)
		{
			stringsToEdit.erase(stringsToEdit.begin() + stringIndex);
			stringIndex--;
			continue;
		}
		
		if (index < endIndex)
		{
			currentString.erase(index + 1, endIndex);
		}
	}
}

void CutOutLeadingAndTrailingWhiteSpace(std::string& stringToEdit)
{
	int index = 0;
	while (stringToEdit.size() > 0 && isspace(stringToEdit[index]))
	{
		index++;
	}

	if (index > 0)
	{
		stringToEdit.erase(0, index);
	}

	int endIndex = (int)stringToEdit.size() - 1;
	index = endIndex;
	while (stringToEdit.size() > 0 && isspace(stringToEdit[index]))
	{
		index--;
	}


	if (index < endIndex)
	{
		stringToEdit.erase(index + 1, endIndex);
	}
}

void CutOutLeadingAndTrailingCharacters(Strings& stringsToEdit, char charToRemove)
{
	for (int stringIndex = 0; stringIndex < (int)stringsToEdit.size(); ++stringIndex)
	{
		std::string& currentString = stringsToEdit[stringIndex];

		int index = 0;
		while (currentString.size() > 0 && currentString[index] == charToRemove)
		{
			index++;
		}

		if (index > 0)
		{
			currentString.erase(0, index);
		}

		int endIndex =(int)currentString.size() - 1;
		index = endIndex;
		while (currentString.size() > 0 && currentString[index] == charToRemove)
		{
			index--;
		}

		if (currentString.size() <= 0)
		{
			stringsToEdit.erase(stringsToEdit.begin() + stringIndex);
			stringIndex--;
			continue;
		}

		if (index < endIndex)
		{
			currentString.erase(index + 1, endIndex);
		}
	}
}

void RemoveAllCharactersOfType(std::string& stringToEdit, char charToRemove)
{
	for (int index = 0; index < (int)stringToEdit.size(); ++index)
	{
		if (stringToEdit[index] == charToRemove)
		{
			stringToEdit.erase(index, 1);
			index--;
		}
	}

}

std::string GetLowercase(std::string const& inputString)
{
	std::string stringLower = inputString;
	for (int i = 0; i < (int)inputString.length(); ++i)
	{
		stringLower[i] = (char)tolower(inputString[i]);
	}

	return stringLower;
}

std::string GetUpperCase(std::string const& inputString)
{
	std::string stringUpper = inputString;
	for (int i = 0; i < (int)inputString.length(); ++i)
	{
		stringUpper[i] = (char)toupper(inputString[i]);
	}

	return stringUpper;
}

int GetIndexOfLastChar(std::string const& inputString, char charToFind)
{
	size_t index = inputString.find_last_of(charToFind);
	return (int)index;
}

float GetStringAsFloat(std::string const& inputString)
{
	return (float)atof(inputString.c_str());
}

int GetStringAsInt(std::string const& inputString)
{
	return (int)atoi(inputString.c_str());
}





