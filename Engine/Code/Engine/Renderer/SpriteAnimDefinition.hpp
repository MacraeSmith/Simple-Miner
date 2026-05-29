#pragma once
#include "Engine/Renderer/SpriteSheet.hpp"

struct IntRange;

enum SpriteAnimPlaybackType : int 
{
	ONCE,
	LOOP,
	PINGPONG,
};
class SpriteAnimDefinition
{
public:
	explicit SpriteAnimDefinition(SpriteSheet const& sheet, int startSpriteIndex, int endSpriteIndex, float framesPerSecond, SpriteAnimPlaybackType playBackType = SpriteAnimPlaybackType::LOOP);
	~SpriteAnimDefinition() {}

	SpriteDefinition const& GetSpriteDefAtTime(float secondsSinceStarted) const;
	IntRange GetFrameRange() const;
	int GetNumFrames() const;
	SpriteAnimPlaybackType GetPlaybackType() const;

private:
	SpriteSheet const& m_spriteSheet;
	int m_startSpriteIndex = -1;
	int m_endSpriteIndex = -1;
	float m_framesPerSecond = 1.f;
	SpriteAnimPlaybackType m_playbackType = LOOP;
};

