#pragma once
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Core/Rgba8.hpp"
#include <vector>
#include <string>

#include "Game/EngineBuildPreferences.hpp"

#ifdef RENDERER_DX12
class TextureDX12;
#else
class Texture;
#endif

struct Vertex_PCU;
struct Vec2;
struct IntVec2;
struct AABB2;

enum TextBoxMode : int 
{
	SHRINK_TO_FIT,
	OVERRUN,
	NUM_TEXT_BOX_MODES,
};
class BitmapFont
{
	friend class Renderer;
	friend class RendererDX11;
	friend class RendererDX12;

private:
#ifdef RENDERER_DX12
	BitmapFont(char const* fontFilePathNameWithNoExtension, TextureDX12& fontTexture, IntVec2 const& layout);
#else
	BitmapFont(char const* fontFilePathNameWithNoExtension, Texture& fontTexture, IntVec2 const& layout);
#endif

public:

	void AddVertsForText2D(std::vector<Vertex_PCU>& vertexArray, Vec2 const& textMins, float cellHeight, std::string const& text, Rgba8 const& tint = Rgba8::WHITE, float cellAspectScale = 1.f);
	void AddVertsForCenteredText2D(std::vector<Vertex_PCU>& vertexArray, Vec2 const& textCenter, float cellHeight, std::string const& text, Rgba8 const& tint = Rgba8::WHITE, float cellAspectScale = 1.f);
	float AddVertsForTextInBox2D(std::vector<Vertex_PCU>& vertexArray, std::string const& text, AABB2 const& box, float cellHeight, Rgba8 const& tint = Rgba8::WHITE, float cellAspectScale = 1.f, Vec2 const& alignment = Vec2(0.5f, 0.5f), TextBoxMode mode = TextBoxMode::SHRINK_TO_FIT, int maxGlyphsToDraw = 99999999);
	void AddVertsForText3DAtOriginXForward(std::vector<Vertex_PCU>& vertexArray, float cellHeight, std::string const& text, Rgba8 const& tint = Rgba8::WHITE, float cellAspectScale = 1.f, Vec2 const& alignment = Vec2(0.5f, 0.5f), int maxGlyphsToDraw = 99999999);
	static float GetTextWidth(float cellHeight, std::string const& text, float cellAspectScale = 1.f);
	AABB2 const GetUVsForLetter(char const& letter) const;
	float GetFontAspect() const;

#ifdef RENDERER_DX12
	TextureDX12& GetTexture() const;
#else
	Texture& GetTexture() const;
#endif

protected:
	float GetGlyphAspect(int glyphUnicode) const;

protected:
	std::string m_fontFilePathNameWithNoExtension;
	SpriteSheet m_fontGlyphsSpriteSheet;
	float m_fontDefaultAspect = 1.0f;
};

