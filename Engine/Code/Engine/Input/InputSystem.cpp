#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Window/Window.hpp"

#define WIN32_LEAN_AND_MEAN	
#include <windows.h>

unsigned char const KEYCODE_F1 = VK_F1; //112 == 0x70
unsigned char const KEYCODE_F2 = VK_F2;
unsigned char const KEYCODE_F3 = VK_F3;
unsigned char const KEYCODE_F4 = VK_F4;
unsigned char const KEYCODE_F5 = VK_F5;
unsigned char const KEYCODE_F6 = VK_F6;
unsigned char const KEYCODE_F7 = VK_F7;
unsigned char const KEYCODE_F8 = VK_F8;
unsigned char const KEYCODE_F9 = VK_F9;
unsigned char const KEYCODE_F10 = VK_F10;
unsigned char const KEYCODE_F11 = VK_F11;
unsigned char const KEYCODE_F12 = VK_F12;
unsigned char const KEYCODE_ESC = VK_ESCAPE;
unsigned char const KEYCODE_UPARROW = VK_UP;
unsigned char const KEYCODE_DOWNARROW = VK_DOWN;
unsigned char const KEYCODE_LEFTARROW = VK_LEFT;
unsigned char const KEYCODE_RIGHTARROW = VK_RIGHT;
unsigned char const KEYCODE_LEFT_MOUSE_BUTTON = VK_LBUTTON;
unsigned char const KEYCODE_RIGHT_MOUSE_BUTTON = VK_RBUTTON;
unsigned char const KEYCODE_TILDE = 0xC0;
unsigned char const KEYCODE_LEFT_BRACKET = 0xDB;
unsigned char const KEYCODE_RIGHT_BRACKET = 0xDD;
unsigned char const KEYCODE_ENTER = VK_RETURN;
unsigned char const KEYCODE_RETURN = VK_RETURN;
unsigned char const KEYCODE_HOME = VK_HOME;
unsigned char const KEYCODE_END = VK_END;
unsigned char const KEYCODE_INSERT = VK_INSERT;
unsigned char const KEYCODE_BACKSPACE = VK_BACK;
unsigned char const KEYCODE_DELETE = VK_DELETE;
unsigned char const KEYCODE_TAB = VK_TAB;
unsigned char const KEYCODE_SHIFT = VK_SHIFT;
unsigned char const KEYCODE_CTRL = VK_CONTROL;


InputSystem::InputSystem(InputConfig const& config)
	:m_config(config)
{
	
}

InputSystem::~InputSystem()
{
}

void InputSystem::Startup()
{
	//set xbox controllers ids
	for (int controllerIndex = 0; controllerIndex < NUM_XBOX_CONTROLLERS; ++controllerIndex)
	{
		m_xBoxControllers[controllerIndex].m_controllerID = controllerIndex;
	}

	SubscribeEventCallbackObjectMethod("KeyPressed", this, &InputSystem::Event_KeyPressed, false);
	SubscribeEventCallbackObjectMethod("KeyReleased", this, &InputSystem::Event_KeyReleased, false);
	SubscribeEventCallbackObjectMethod("MouseWheelScrolled", this, &InputSystem::Event_MouseWheelTurned, false);
}

void InputSystem::Shutdown()
{
	UnsubscribeAllEventCallbacksForObject(this);
}

void InputSystem::BeginFrame()
{
	for (int xboxControllerIndex = 0; xboxControllerIndex < NUM_XBOX_CONTROLLERS; ++xboxControllerIndex)
	{
		m_xBoxControllers[xboxControllerIndex].UpdateStatus();
	}

	//Cursor point last frame
	IntVec2 cursorPosLastFame = m_cursorState.m_cursorClientPosition;

	//Handle cursor visibility
	PCURSORINFO cursorInfo{};
	bool visibleCursor = (GetCursorInfo(cursorInfo) && (cursorInfo->flags & CURSOR_SHOWING) != 0 );
	bool inPointerMode = m_cursorState.m_cursorMode == CursorMode::POINTER;
	if (!visibleCursor && inPointerMode)
	{
		while (ShowCursor(TRUE) < 0) {}
	}

	else if(visibleCursor && !inPointerMode)
	{
		while (ShowCursor(FALSE) >= 0) {}
	}

	
	POINT cursorPoint;	
	if (Window::s_mainWindow->IsWindowActive() && GetCursorPos(&cursorPoint) && m_cursorState.m_cursorMode == CursorMode::FPS)
	{
		IntVec2 cursorPos((int)cursorPoint.x, (int)cursorPoint.y);
		m_cursorState.m_cursorClientDelta = cursorPos - cursorPosLastFame;
		Window::s_mainWindow->SetMouseToCenter();
		
		GetCursorPos(&cursorPoint);
		m_cursorState.m_cursorClientPosition = IntVec2((int)cursorPoint.x, (int)cursorPoint.y);
	}

	else
	{
		m_cursorState.m_cursorClientDelta = IntVec2::ZERO;
	}

	
}

void InputSystem::EndFrame()
{
	for (int keyCodeIndex = 0; keyCodeIndex < NUM_KEYCODES; keyCodeIndex++)
	{
		m_keyStates[keyCodeIndex].UpdateKeyLastFrame();
	}

	m_wheelDelta = 0.f;
}

bool InputSystem::WasKeyJustPressed(unsigned char keyCode)
{
	KeyButtonState currentKey = m_keyStates[keyCode];
	bool wasPressed = (currentKey.m_isDown && (!currentKey.m_wasDownLastFrame));
	return wasPressed;

}

bool InputSystem::WasKeyJustReleased(unsigned char keyCode)
{
	KeyButtonState currentKey = m_keyStates[keyCode];
	return ((!currentKey.m_isDown) && currentKey.m_wasDownLastFrame);
}

bool InputSystem::IsKeyDown(unsigned char keyCode)
{
	return m_keyStates[keyCode].m_isDown;
}

void InputSystem::HandleKeyPressed(unsigned char keyCode)
{
	m_keyStates[keyCode].m_isDown = true;
}

void InputSystem::HandleKeyReleased(unsigned char keyCode)
{
	m_keyStates[keyCode].m_isDown = false;
}

XboxController const& InputSystem::GetController(int controllerID)
{
	return m_xBoxControllers[controllerID];
}

void InputSystem::SetWheelDelta(float delta)
{
	m_wheelDelta = delta;
}

void InputSystem::SetCursorMode(CursorMode cursorMode)
{
	m_cursorState.m_cursorMode = cursorMode;

	if (cursorMode == CursorMode::POINTER)
	{
		while (ShowCursor(TRUE) < 0) {}
	}

	//center and hide cursor when swithching to FPS
	if (cursorMode == CursorMode::FPS)
	{
		while (ShowCursor(FALSE) >= 0) {}
		POINT cursorPoint;
		IntVec2 clientDims = Window::s_mainWindow->GetClientDimensions();
		int centerX = (int)((float)clientDims.x * 0.5f);
		int centerY = (int)((float)clientDims.y * 0.5f);
		SetCursorPos(centerX, centerY);
		GetCursorPos(&cursorPoint);
		m_cursorState.m_cursorClientPosition = IntVec2(centerX, centerY);
	}
}

CursorMode InputSystem::GetCursorMode() const
{
	return m_cursorState.m_cursorMode;
}

Vec2 InputSystem::GetCursorClientDelta() const
{
	return Vec2(m_cursorState.m_cursorClientDelta);
}

Vec2 InputSystem::GetCursorClientPosition() const
{
	return Vec2(m_cursorState.m_cursorClientPosition);
}

Vec2 InputSystem::GetCursorNormalizedPosition() const
{
	return Window::s_mainWindow->GetNormalizedMouseUV();
}

float InputSystem::GetWheelDelta() const
{
	return m_wheelDelta;
}


bool InputSystem::Event_KeyPressed(EventArgs& args)
{
 	unsigned char keyCode = args.GetValue("KeyCode", (unsigned char)1);
	HandleKeyPressed(keyCode);
	return true;
}

bool InputSystem::Event_KeyReleased(EventArgs& args)
{
	unsigned char keyCode = args.GetValue("KeyCode", (unsigned char)1);
	HandleKeyReleased(keyCode);
	return true;
}

bool InputSystem::Event_MouseWheelTurned(EventArgs& args)
{
	float wheelDelta = args.GetValue("MouseWheelDelta", 0.f);
	SetWheelDelta(wheelDelta);
	return true;
}
