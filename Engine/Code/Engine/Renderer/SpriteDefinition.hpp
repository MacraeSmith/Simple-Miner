#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Game/EngineBuildPreferences.hpp"

#ifdef RENDERER_DX12
class TextureDX12;
#else
class Texture;

#endif // RENDERER_DX12

class SpriteSheet;
struct AABB2;

class SpriteDefinition
{
public:
	explicit SpriteDefinition(SpriteSheet const& spriteSheet, int spriteIndex, Vec2 const& uvAtMins, Vec2 const& uvAtMaxs);
	void					GetUVS(Vec2& out_uvAtMins, Vec2& out_uvAtMaxs) const;
	AABB2					GetUVS() const;
	SpriteSheet const&		GetSpriteSheet() const;
	float					GetAspect() const;

#ifdef RENDERER_DX12
	TextureDX12&			GetTexture() const;
#else
	Texture&				GetTexture() const;
#endif // RENDERER_DX12

protected:
	SpriteSheet const& m_spriteSheet;
	int m_spriteIndex = -1;
	Vec2 m_uvAtMins = Vec2::ZERO;
	Vec2 m_uvAtMaxs = Vec2::ONE;

};

