#pragma once
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include <string>
#include <vector>

struct BlockDefinition
{
	static std::vector<BlockDefinition> s_blockDefinitions;
	std::string m_name = "UNAMED_BLOCK";
	bool m_isVisible = true;
	bool m_isSolid = true;
	bool m_isOpaque = true;
	bool m_isLiquid = false;
	IntVec2 m_topSpriteCoords;
	IntVec2 m_bottomSpriteCoords;
	IntVec2 m_sideSpriteCoords;
	IntVec2 m_iconSpriteCoords = IntVec2(6,6);
	int m_indoorLighting = 0;

	explicit BlockDefinition(XmlElement const& blockDefElement);
	~BlockDefinition() {}
	static void InitBlockDefinitions();
	static BlockDefinition* GetBlockDefinitionFromIndex(uint8_t index);
	static BlockDefinition*GetBlockDefinitionFromName(std::string const& blockName);
	static uint8_t GetBlockDefinitionIndexFromName(std::string const& blockName);
	bool IsCactus() const;
	bool IsLeaves() const;
	bool IsWater() const;
	bool IsSurfaceBlock() const;
	bool IsWildGrass() const;

};

