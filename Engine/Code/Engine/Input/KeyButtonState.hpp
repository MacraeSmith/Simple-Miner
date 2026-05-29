#pragma once
class KeyButtonState
{
public:
	void UpdateKeyLastFrame();

public:
	bool m_isDown = false;
	bool m_wasDownLastFrame = false;
};

