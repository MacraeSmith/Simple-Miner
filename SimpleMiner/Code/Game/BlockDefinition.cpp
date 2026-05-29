#include "Game/BlockDefinition.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Core/EngineCommon.hpp"

std::vector<BlockDefinition> BlockDefinition::s_blockDefinitions;

BlockDefinition::BlockDefinition(XmlElement const& blockDefElement)
{
	m_name = ParseXmlAttribute(blockDefElement, "name", m_name);
	m_isVisible = ParseXmlAttribute(blockDefElement, "isVisible", m_isVisible);
	m_isSolid = ParseXmlAttribute(blockDefElement, "isSolid", m_isSolid);
	m_isOpaque = ParseXmlAttribute(blockDefElement, "isOpaque", m_isOpaque);
	m_isLiquid = ParseXmlAttribute(blockDefElement, "isLiquid", false);
	m_topSpriteCoords = ParseXmlAttribute(blockDefElement, "topSpriteCoords", m_topSpriteCoords);
	m_bottomSpriteCoords = ParseXmlAttribute(blockDefElement, "bottomSpriteCoords", m_bottomSpriteCoords);
	m_sideSpriteCoords = ParseXmlAttribute(blockDefElement, "sideSpriteCoords", m_sideSpriteCoords);
	m_iconSpriteCoords = ParseXmlAttribute(blockDefElement, "iconCoords", m_iconSpriteCoords);
	m_indoorLighting = ParseXmlAttribute(blockDefElement, "indoorLighting", 0);
}

void BlockDefinition::InitBlockDefinitions()
{
	XmlDocument blockDefXml;
	char const* filePath = "Data/Definitions/BlockSpriteSheet_BlockDefinitions.xml";
	XmlResult result = blockDefXml.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("Failed to open required BlockDefs File %s", filePath));

	XmlElement* rootElement = blockDefXml.RootElement();
	GUARANTEE_OR_DIE(rootElement, "Failed to find root element of BlockDefs xml");

	XmlElement* blockDefElement = rootElement->FirstChildElement();
	while (blockDefElement)
	{
		std::string elementName = blockDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "BlockDefinition", Stringf("Root child element in %s was <%s>, must be <BlockDefinition>!", filePath, elementName.c_str()));
		BlockDefinition* newBlockDef = new BlockDefinition(*blockDefElement);
		s_blockDefinitions.push_back(*newBlockDef);
		blockDefElement = blockDefElement->NextSiblingElement();
	}
}

BlockDefinition* BlockDefinition::GetBlockDefinitionFromIndex(uint8_t index)
{
	if(index < 0 || index >= s_blockDefinitions.size())
		return nullptr;

	return &s_blockDefinitions[index];
}

BlockDefinition* BlockDefinition::GetBlockDefinitionFromName(std::string const& blockName)
{
	for (int i = 0; i < (int)(s_blockDefinitions.size()); ++i)
	{
		if (s_blockDefinitions[i].m_name == blockName)
		{
			return &s_blockDefinitions[i];
		}
	}

	BlockDefinition* failedDefinition = nullptr;
	GUARANTEE_OR_DIE(failedDefinition != nullptr, Stringf("Failed to find blockDefinition with name: \"%s\"", blockName.c_str()));
	return failedDefinition;
}

uint8_t BlockDefinition::GetBlockDefinitionIndexFromName(std::string const& blockName)
{
	for (uint8_t i = 0; i < (uint8_t)(s_blockDefinitions.size()); ++i)
	{
		if (s_blockDefinitions[i].m_name == blockName)
		{
			return i;
		}
	}

	return 0;
}

bool BlockDefinition::IsCactus() const
{
	return m_name == "CactusLog";
}

bool BlockDefinition::IsLeaves() const
{
	return  m_name == "AcaciaLeaves" || m_name == "OakLeaves" || m_name == "BirchLeaves" || m_name == "JungleLeaves" || m_name == "SpruceLeaves" || m_name == "SpruceLeavesSnow" || m_name == "WildGrass";
}

bool BlockDefinition::IsWater() const
{
	return m_name == "Water";
}

bool BlockDefinition::IsSurfaceBlock() const
{
	return m_name == "Grass" || m_name == "GrassLight" || m_name == "GrassDark" || m_name == "GrassYellow" || m_name == "Snow";
}

bool BlockDefinition::IsWildGrass() const
{
	return m_name == "WildGrass" || m_name == "WildGrassTall" || m_name == "WildGrassFrozen" || m_name == "WildGrassFrozenTall";
}
