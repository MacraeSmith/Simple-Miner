#pragma once
#include "Game/EngineBuildPreferences.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"
#include "Engine/Math/IntVec2.hpp"
#include <vector>

struct AABB2;
class SpriteDefinition;

#ifdef RENDERER_DX12
class TextureDX12;
#else
class Texture;
#endif

class SpriteSheet
{
public:
#ifdef RENDERER_DX12
	explicit SpriteSheet(TextureDX12& texture, IntVec2 const& simpleGridLayout);
	TextureDX12& GetTexture() const;
#else
	explicit SpriteSheet(Texture& texture, IntVec2 const& simpleGridLayout);
	Texture& GetTexture() const;
#endif

	~SpriteSheet() {}

	int							GetNumSprites() const;
	SpriteDefinition const&		GetSpriteDef(int spriteIndex) const;
	void						GetSpriteUVs(Vec2& out_uvAtMins, Vec2& out_uvAtMax, int spriteIndex) const;
	AABB2						GetSpriteUVs(int spriteIndex) const;
	AABB2						GetSpriteUVs(IntVec2 const& spriteCoords) const;


protected:
#ifdef RENDERER_DX12
	TextureDX12& m_texture;
#else
	Texture& m_texture;
#endif

	std::vector<SpriteDefinition> m_spriteDefs;
	IntVec2 m_gridDimensions;
};

