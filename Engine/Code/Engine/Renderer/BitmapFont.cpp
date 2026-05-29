#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/IntRange.hpp"
#include "Engine/Math/Mat44.hpp"

#ifdef RENDERER_DX12
#include "Engine/Renderer/TextureDX12.hpp"
BitmapFont::BitmapFont(char const* fontFilePathNameWithNoExtension, TextureDX12& fontTexture, IntVec2 const& layout)
	:m_fontFilePathNameWithNoExtension(fontFilePathNameWithNoExtension)
	, m_fontGlyphsSpriteSheet(SpriteSheet(fontTexture, layout))
{
	//#TODO: test to make sure aspect is actually doing something
	IntVec2 textureDims = fontTexture.GetDimensions();
	m_fontDefaultAspect = (float)(textureDims.y) / (float)(textureDims.x);
}

TextureDX12& BitmapFont::GetTexture() const
{
	return m_fontGlyphsSpriteSheet.GetTexture();
}


#else
#include "Engine/Renderer/Texture.hpp"

BitmapFont::BitmapFont(char const* fontFilePathNameWithNoExtension, Texture& fontTexture, IntVec2 const& layout)
	:m_fontFilePathNameWithNoExtension(fontFilePathNameWithNoExtension)
	, m_fontGlyphsSpriteSheet(SpriteSheet(fontTexture, layout))
{
	//#TODO: test to make sure aspect is actually doing something
	IntVec2 textureDims = fontTexture.GetDimensions();
	m_fontDefaultAspect = (float)(textureDims.y) / (float)(textureDims.x);
}

Texture& BitmapFont::GetTexture() const
{
	return m_fontGlyphsSpriteSheet.GetTexture();
}
#endif // RENDERER_DX12


void BitmapFont::AddVertsForText2D(std::vector<Vertex_PCU>& vertexArray, Vec2 const& textMins, float cellHeight, std::string const& text, Rgba8 const& tint, float cellAspectScale)
{
    float letterWidth = cellHeight * cellAspectScale;
    for (int letterNum = 0; letterNum < (float)(text.size()); ++letterNum)
    {
        char letter = text[letterNum];
        float xOffset = letterNum * letterWidth;
        AABB2 letterBounds(textMins.x + xOffset, textMins.y, (textMins.x + xOffset + letterWidth), (textMins.y + cellHeight));
        AABB2 uvs = GetUVsForLetter(letter);
        AddVertsForAABB2D(vertexArray, letterBounds, tint, GetUVsForLetter(letter));
    }
}

void BitmapFont::AddVertsForCenteredText2D(std::vector<Vertex_PCU>& vertexArray, Vec2 const& textCenter, float cellHeight, std::string const& text, Rgba8 const& tint, float cellAspectScale)
{
    float textWidth = GetTextWidth(cellHeight, text, cellAspectScale);
    float letterWidth = cellHeight * cellAspectScale;
    Vec2 textMins(textCenter.x - (textWidth * 0.5f), textCenter.y - (cellHeight * 0.5f));

	for (int letterNum = 0; letterNum < (float)(text.size()); ++letterNum)
	{
		char letter = text[letterNum];
		float xOffset = letterNum * letterWidth;
		AABB2 letterBounds(textMins.x + xOffset, textMins.y, (textMins.x + xOffset + letterWidth), (textMins.y + cellHeight));
		AddVertsForAABB2D(vertexArray, letterBounds, tint, GetUVsForLetter(letter));
	}
}

float BitmapFont::AddVertsForTextInBox2D(std::vector<Vertex_PCU>& vertexArray, std::string const& text, AABB2 const& box, float cellHeight, Rgba8 const& tint, float cellAspectScale, Vec2 const& alignment, TextBoxMode mode, int maxGlyphsToDraw)
{
    float letterWidth = cellHeight * cellAspectScale;
    float boxWidth = box.m_maxs.x - box.m_mins.x;
    float boxHeight = box.m_maxs.y - box.m_mins.y;
    Vec2 alignedMins;

    Strings textLines = SplitStringOnDelimiter(text, '\n', false);
    const int NUM_LINES = (int)(textLines.size());
    
	float yOffset = Lerp((float)(NUM_LINES - 1) * cellHeight, 0.f, alignment.y); // offsets top y pos so all lines fit inside box
	// **in overrun mode if alignment.y = 1 but there are too many lines it will still run off the bottom or run off the top if the alignment.y = 0

    float topYPos = box.m_mins.y + ((boxHeight - cellHeight) * alignment.y) + yOffset;

    if (mode == OVERRUN)
    {
        for (int lineNum = 0; lineNum < NUM_LINES; ++lineNum)
        {
            std::string& textOnLine = textLines[lineNum];
            int numGlyphs = GetMin((int) (textOnLine.size()), maxGlyphsToDraw);
            float lineWidth = GetTextWidth(cellHeight, textOnLine, cellAspectScale);

            alignedMins.x = box.m_mins.x + ((boxWidth - lineWidth) * alignment.x);
            alignedMins.y = topYPos - (cellHeight * lineNum);

			for (int letterNum = 0; letterNum < numGlyphs; ++letterNum)
			{
				char letter = textOnLine[letterNum];
				float xOffset = letterNum * letterWidth;
				AABB2 letterBounds(alignedMins.x + xOffset, alignedMins.y, (alignedMins.x + xOffset + letterWidth), (alignedMins.y + cellHeight));

				AddVertsForAABB2D(vertexArray, letterBounds, tint, GetUVsForLetter(letter));
			}
        }

        return cellHeight;
    }

    else // Shrink to fit mode
    {
        float longestLineWidth = 0;

        //find the longest width of lines to base scale correction off of
        for (int lineNum = 0; lineNum < NUM_LINES; ++lineNum)
        {
            std::string& textOnLine = textLines[lineNum];
            float lineWidth = GetTextWidth(cellHeight, textOnLine, cellAspectScale);
            if (lineWidth > longestLineWidth)
            {
                longestLineWidth = lineWidth;
            }
        }

        float correctedCellHeight = cellHeight;

        //if text extends bounds, find bigger overrun (x or y) and correct scale so that dimension fits.
        if (longestLineWidth > boxWidth || (NUM_LINES * cellHeight) > boxHeight)
        {
            float overrunPercentageX = GetClampedFractionWithinRange(boxWidth, 0.f, longestLineWidth);
            float overrunPercentageY = GetClampedFractionWithinRange(boxHeight, 0.f, (cellHeight * NUM_LINES));
            correctedCellHeight *= GetMin(overrunPercentageX, overrunPercentageY);
        }

        //readjust y offset based on new scale
		yOffset = Lerp((float)(NUM_LINES - 1) * correctedCellHeight, 0.f, alignment.y);
		topYPos = box.m_mins.y + ((boxHeight - correctedCellHeight) * alignment.y) + yOffset;
        letterWidth = cellAspectScale * correctedCellHeight;

		for (int lineNum = 0; lineNum < NUM_LINES; ++lineNum)
		{
			std::string& textOnLine = textLines[lineNum];
			int numGlyphs = GetMin((int) (textOnLine.size()), maxGlyphsToDraw);
			float lineWidth = GetTextWidth(correctedCellHeight, textOnLine, cellAspectScale);

			alignedMins.x = box.m_mins.x + ((boxWidth - lineWidth) * alignment.x);
			alignedMins.y = topYPos - (correctedCellHeight * lineNum);

			for (int letterNum = 0; letterNum < numGlyphs; ++letterNum)
			{
				char letter = textOnLine[letterNum];
				float xOffset = letterNum * letterWidth;
				AABB2 letterBounds(alignedMins.x + xOffset, alignedMins.y, (alignedMins.x + xOffset + letterWidth), (alignedMins.y + correctedCellHeight));

				AddVertsForAABB2D(vertexArray, letterBounds, tint, GetUVsForLetter(letter));
			}
		}
		return correctedCellHeight;
    }
}

void BitmapFont::AddVertsForText3DAtOriginXForward(std::vector<Vertex_PCU>& vertexArray, float cellHeight, std::string const& text, Rgba8 const& tint, float cellAspectScale, Vec2 const& alignment, int maxGlyphsToDraw)
{
    const int START_INDEX = (int)vertexArray.size();
    float width = GetTextWidth(cellHeight, text, cellAspectScale);
    Vec2 mins = Vec2(-alignment.x * width, -alignment.y * cellHeight);
    AABB2 textBounds = AABB2(mins, Vec2(mins.x + width, mins.y + cellHeight));
    AddVertsForTextInBox2D(vertexArray, text, textBounds, cellHeight, tint, cellAspectScale, Vec2(0.0f, 0.5f), SHRINK_TO_FIT, maxGlyphsToDraw);

    Mat44 transform = Mat44(Vec3::LEFT, Vec3::UP, Vec3::FORWARD, Vec3::ZERO);
	//flip bases to orient in 3D space
	   // i= (0,1,0)
	   // j= (0,0,1)
	   // k= (1,0,0)

    TransformVertexArray3D(vertexArray, transform, IntRange(START_INDEX, (int)vertexArray.size() - 1));
}

float BitmapFont::GetTextWidth(float cellHeight, std::string const& text, float cellAspectScale)
{
    float textLength = (float)(text.size());
    return cellHeight * cellAspectScale * textLength;
}

AABB2 const BitmapFont::GetUVsForLetter(char const& letter) const
{
    return m_fontGlyphsSpriteSheet.GetSpriteUVs(letter);
}

float BitmapFont::GetFontAspect() const
{
    return m_fontDefaultAspect;
}

float BitmapFont::GetGlyphAspect(int glyphUnicode) const
{
    UNUSED(glyphUnicode);
    return m_fontDefaultAspect;
}
