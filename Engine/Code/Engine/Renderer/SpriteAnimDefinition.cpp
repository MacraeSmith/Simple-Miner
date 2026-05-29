#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/IntRange.hpp"

SpriteAnimDefinition::SpriteAnimDefinition(SpriteSheet const& sheet, int startSpriteIndex, int endSpriteIndex, float framesPerSecond, SpriteAnimPlaybackType playBackType)
	:m_spriteSheet(sheet)
	,m_startSpriteIndex(startSpriteIndex)
	,m_endSpriteIndex(endSpriteIndex)
	,m_framesPerSecond(framesPerSecond)
	,m_playbackType(playBackType)
{
}

SpriteDefinition const& SpriteAnimDefinition::GetSpriteDefAtTime(float secondsSinceStarted) const
{
	int numFrames = m_endSpriteIndex - m_startSpriteIndex + 1;
	int currentSpriteIndex = -1;

	switch (m_playbackType)
	{
	case ONCE:

		currentSpriteIndex = RoundDownToInt(secondsSinceStarted * m_framesPerSecond) + m_startSpriteIndex;
		currentSpriteIndex = GetClampedInt(currentSpriteIndex, m_startSpriteIndex, m_endSpriteIndex);
		break;

	case LOOP:

		currentSpriteIndex = RoundDownToInt(secondsSinceStarted * m_framesPerSecond);
		currentSpriteIndex = (currentSpriteIndex % numFrames) + m_startSpriteIndex;
		break;

	case PINGPONG:

		int frameNum = 0;
		int numUpFrames = numFrames;
		numFrames = numFrames + (numFrames - 2);
		
		frameNum = RoundDownToInt(secondsSinceStarted * m_framesPerSecond);
		frameNum = (frameNum % numFrames);

		if (frameNum >= numUpFrames)
		{
			frameNum = numFrames - frameNum;
		}

		currentSpriteIndex = m_startSpriteIndex + frameNum;
		
		break;
	}
	
	return m_spriteSheet.GetSpriteDef(currentSpriteIndex);
}

IntRange SpriteAnimDefinition::GetFrameRange() const
{
	return IntRange(m_startSpriteIndex, m_endSpriteIndex);
}

int SpriteAnimDefinition::GetNumFrames() const
{
	return (m_endSpriteIndex + 1) - m_startSpriteIndex;
}

SpriteAnimPlaybackType SpriteAnimDefinition::GetPlaybackType() const
{
	return m_playbackType;
}

