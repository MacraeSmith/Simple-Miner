#pragma once
#include "ThirdParty/TinyXML2/tinyxml2.h"
#include "Engine/Core/StringUtils.hpp"
#include <string>

typedef tinyxml2::XMLDocument XmlDocument;
typedef tinyxml2::XMLElement XmlElement;
typedef tinyxml2::XMLAttribute XmlAttribute;
typedef tinyxml2::XMLError XmlResult;

struct Rgba8;
struct IntVec2;
struct IntVec3;
struct Vec2;
struct Vec3;
struct EulerAngles;
struct FloatRange;
struct IntRange;

int ParseXmlAttribute(XmlElement const& element, char const* attributeName, int defaultValue);
char ParseXmlAttribute(XmlElement const& element, char const* attributeName, char defaultValue);
bool ParseXmlAttribute(XmlElement const& element, char const* attributeName, bool defaultValue);
float ParseXmlAttribute(XmlElement const& element, char const* attributeName, float defaultValue);
Rgba8 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Rgba8 const& defaultValue);
Vec2 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vec2 const& defaultValue);
Vec3 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vec3 const& defaultValue);
EulerAngles ParseXmlAttribute(XmlElement const& element, char const* attributeName, EulerAngles const& defaultValue);
FloatRange ParseXmlAttribute(XmlElement const& element, char const* attributeName, FloatRange const& defaultValue);
IntRange ParseXmlAttribute(XmlElement const& element, char const* attributeName, IntRange const& defaultValue);
IntVec2 ParseXmlAttribute(XmlElement const& element, char const* attributeName, IntVec2 const& defaultValue);
IntVec3 ParseXmlAttribute(XmlElement const& element, char const* attributeName, IntVec3 const& defaultValue);
std::string ParseXmlAttribute(XmlElement const& element, char const* attributeName, std::string const& defaultValue);
Strings ParseXmlAttribute(XmlElement const& element, char const* attributeName, Strings const& defaultValues, char delimiter = ',');
std::string ParseXmlAttribute(XmlElement const& element, char const* attributeName, char const* defaultValue);



