#include "Engine/Input/KeyButtonState.hpp"

void KeyButtonState::UpdateKeyLastFrame()
{
	m_wasDownLastFrame = m_isDown;
}
