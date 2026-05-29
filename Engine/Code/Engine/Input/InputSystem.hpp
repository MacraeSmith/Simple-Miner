#pragma once
#include "Engine/Input/XboxController.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Math/IntVec2.hpp"
extern unsigned char const KEYCODE_F1;
extern unsigned char const KEYCODE_F2;
extern unsigned char const KEYCODE_F3;
extern unsigned char const KEYCODE_F4;
extern unsigned char const KEYCODE_F5;
extern unsigned char const KEYCODE_F6;
extern unsigned char const KEYCODE_F7;
extern unsigned char const KEYCODE_F8;
extern unsigned char const KEYCODE_F9;
extern unsigned char const KEYCODE_F10;
extern unsigned char const KEYCODE_F11;
extern unsigned char const KEYCODE_F12;
extern unsigned char const KEYCODE_ESC;
extern unsigned char const KEYCODE_UPARROW;
extern unsigned char const KEYCODE_DOWNARROW;
extern unsigned char const KEYCODE_LEFTARROW;
extern unsigned char const KEYCODE_RIGHTARROW;
extern unsigned char const KEYCODE_LEFT_MOUSE_BUTTON;
extern unsigned char const KEYCODE_RIGHT_MOUSE_BUTTON;
extern unsigned char const KEYCODE_TILDE;
extern unsigned char const KEYCODE_LEFT_BRACKET;
extern unsigned char const KEYCODE_RIGHT_BRACKET;
extern unsigned char const KEYCODE_ENTER;
extern unsigned char const KEYCODE_RETURN;
extern unsigned char const KEYCODE_HOME; 
extern unsigned char const KEYCODE_END;
extern unsigned char const KEYCODE_INSERT;
extern unsigned char const KEYCODE_BACKSPACE; 
extern unsigned char const KEYCODE_DELETE;
extern unsigned char const KEYCODE_TAB;
extern unsigned char const KEYCODE_SHIFT;
extern unsigned char const KEYCODE_CTRL;

//-------------------------------------------------------------------------------------------------------------
constexpr int NUM_KEYCODES = 256;
constexpr int NUM_XBOX_CONTROLLERS = 4;

//-------------------------------------------------------------------------------------------------------------
enum class CursorMode
{
	POINTER,
	FPS
};

struct CursorState
{
	IntVec2 m_cursorClientDelta;
	IntVec2 m_cursorClientPosition;
	CursorMode m_cursorMode = CursorMode::POINTER;
};

struct InputConfig
{

};

class InputSystem
{
public:
	explicit InputSystem(InputConfig const& config);
	~InputSystem();

	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();
	bool WasKeyJustPressed(unsigned char keyCode);
	bool WasKeyJustReleased(unsigned char keyCode);
	bool IsKeyDown(unsigned char keyCode);
	void HandleKeyPressed(unsigned char keyCode);
	bool Event_KeyPressed(EventArgs& args);
	bool Event_KeyReleased(EventArgs& args);
	bool Event_MouseWheelTurned(EventArgs& args);
	void HandleKeyReleased(unsigned char keyCode);
	XboxController const& GetController(int controllerID);

	void SetWheelDelta(float delta);
	void SetCursorMode(CursorMode cursorMode);
	CursorMode GetCursorMode() const;
	Vec2 GetCursorClientDelta() const;
	Vec2 GetCursorClientPosition() const;
	Vec2 GetCursorNormalizedPosition() const;
	float GetWheelDelta() const;


	//static bool Event_KeyPressed(EventArgs& args);
	//static bool Event_KeyReleased(EventArgs& args);
	//static bool Event_MouseWheelTurned(EventArgs& args);

public:
	CursorState m_cursorState;
protected:
	KeyButtonState m_keyStates[NUM_KEYCODES];
	XboxController m_xBoxControllers[NUM_XBOX_CONTROLLERS];
	float m_wheelDelta;

private:
	InputConfig m_config;
};

