#include "Engine/Input/XboxController.hpp"

#include <Windows.h>
#include <Xinput.h>
#pragma comment( lib, "xinput" )

#include "Engine/Math/MathUtils.hpp"


XboxController::XboxController()
{
}

XboxController::~XboxController()
{
}

bool XboxController::IsConnected() const
{
	return m_isConnected;
}

int XboxController::GetControllerID() const
{
	return m_controllerID;
}

AnalogJoystick const& XboxController::GetLeftStick() const
{
	return m_leftStick;
}

AnalogJoystick const& XboxController::GetRightStick() const
{
	return m_rightStick;
}

float XboxController::GetLeftTrigger() const
{
	return m_leftTrigger;
}

float XboxController::GetRightTrigger() const
{
	return m_rightTrigger;
}

KeyButtonState const& XboxController::GetButton(XboxButtonID buttonID) const
{
	return m_buttons[(int)(buttonID)];
}

bool XboxController::IsButtonDown(XboxButtonID buttonID) const
{
	return m_buttons[(int)(buttonID)].m_isDown;
}

bool XboxController::WasButtonJustPressed(XboxButtonID buttonID) const
{
	KeyButtonState currentButton = m_buttons[(int)(buttonID)];
	return (currentButton.m_isDown && (!currentButton.m_wasDownLastFrame));
}

bool XboxController::WasButtonJustReleased(XboxButtonID buttonID) const
{
	KeyButtonState currentButton = m_buttons[(int)(buttonID)];
	return ((!currentButton.m_isDown) && currentButton.m_wasDownLastFrame);
}


void XboxController::UpdateStatus()
{
	//Read raw controller state via XInput API
	XINPUT_STATE xboxControllerState;
	memset(&xboxControllerState, 0, sizeof(xboxControllerState));
	DWORD errorStatus = XInputGetState(m_controllerID, &xboxControllerState);
	if (errorStatus != ERROR_SUCCESS)
	{
		Reset();
		m_isConnected = false;
		return;
	}

	m_isConnected = true;
	XINPUT_GAMEPAD const& gamepadState = xboxControllerState.Gamepad;
	UpdateJoystick(m_leftStick, gamepadState.sThumbLX, gamepadState.sThumbLY);
	UpdateJoystick(m_rightStick, gamepadState.sThumbRX, gamepadState.sThumbRY);

	UpdateTrigger(m_leftTrigger, gamepadState.bLeftTrigger);
	UpdateTrigger(m_rightTrigger, gamepadState.bRightTrigger);

	UpdateButton(XboxButtonID::BUTTON_A,			gamepadState.wButtons, XINPUT_GAMEPAD_A);
	UpdateButton(XboxButtonID::BUTTON_B,			gamepadState.wButtons, XINPUT_GAMEPAD_B);
	UpdateButton(XboxButtonID::BUTTON_X,			gamepadState.wButtons, XINPUT_GAMEPAD_X);
	UpdateButton(XboxButtonID::BUTTON_Y,			gamepadState.wButtons, XINPUT_GAMEPAD_Y);
	UpdateButton(XboxButtonID::BUTTON_BACK,			gamepadState.wButtons, XINPUT_GAMEPAD_BACK);
	UpdateButton(XboxButtonID::BUTTON_START,		gamepadState.wButtons, XINPUT_GAMEPAD_START);
	UpdateButton(XboxButtonID::BUTTON_LSHOULDER,	gamepadState.wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);
	UpdateButton(XboxButtonID::BUTTON_RSHOULDER,	gamepadState.wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER);
	UpdateButton(XboxButtonID::BUTTON_LTHUMB,		gamepadState.wButtons, XINPUT_GAMEPAD_LEFT_THUMB);
	UpdateButton(XboxButtonID::BUTTON_RTHUMB,		gamepadState.wButtons, XINPUT_GAMEPAD_RIGHT_THUMB);
	UpdateButton(XboxButtonID::BUTTON_DPAD_RIGHT,	gamepadState.wButtons, XINPUT_GAMEPAD_DPAD_RIGHT);
	UpdateButton(XboxButtonID::BUTTON_DPAD_UP,		gamepadState.wButtons, XINPUT_GAMEPAD_DPAD_UP);
	UpdateButton(XboxButtonID::BUTTON_DPAD_LEFT,	gamepadState.wButtons, XINPUT_GAMEPAD_DPAD_LEFT);
	UpdateButton(XboxButtonID::BUTTON_DPAD_DOWN,	gamepadState.wButtons, XINPUT_GAMEPAD_DPAD_DOWN);

	UpdateVirtualTriggerButton(XboxButtonID::BUTTON_VIRTUAL_LEFT_TRIGGER_BUTTON, m_leftTrigger);
	UpdateVirtualTriggerButton(XboxButtonID::BUTTON_VIRTUAL_RIGHT_TRIGGER_BUTTON, m_rightTrigger);
}

void XboxController::Reset()
{
	for (int buttonIndex = 0; buttonIndex < (int)(XboxButtonID::NUM_BUTTONS); buttonIndex++)
	{
		m_buttons[buttonIndex].m_isDown = false;
	}

	m_controllerID = -1;
	m_isConnected = false;
	m_leftTrigger = 0.f;
	m_rightTrigger = 0.f;
	m_leftStick.Reset();
	m_rightStick.Reset();
}

void XboxController::UpdateJoystick(AnalogJoystick& out_joystick, short rawX, short rawY)
{
	float rawNormalizedX = RangeMapClamped(
		
		(rawX), MIN_ANALOG_RAW_VALUE, MAX_ANALOG_RAW_VALUE, -1.f, 1.f);
	float rawNormalizedY = RangeMapClamped((float)(rawY), MIN_ANALOG_RAW_VALUE, MAX_ANALOG_RAW_VALUE, -1.f, 1.f);
	out_joystick.UpdatePosition(rawNormalizedX, rawNormalizedY);
}

void XboxController::UpdateTrigger(float& out_triggerValue, unsigned char rawValue)
{
	out_triggerValue = GetFractionWithinRange(rawValue, 0, 255); //range map here to 0 - 255
}

void XboxController::UpdateButton(XboxButtonID buttonID, unsigned short wButtons, unsigned short buttonFlag)
{
	KeyButtonState& button = m_buttons[(int) (buttonID)];
	button.m_wasDownLastFrame = button.m_isDown;
	button.m_isDown = (wButtons & buttonFlag) == buttonFlag;
}

void XboxController::UpdateVirtualTriggerButton(XboxButtonID virtualButtonID, float triggerAnalogValue)
{
	KeyButtonState& button = m_buttons[(int)(virtualButtonID)];
	button.m_wasDownLastFrame = button.m_isDown;
	if (button.m_wasDownLastFrame && triggerAnalogValue < m_triggerVirtualButtonReleasedValue)
	{
		button.m_isDown = false;
	}

	if (!button.m_wasDownLastFrame && triggerAnalogValue > m_triggerVirtualButtonPressedValue)
	{
		button.m_isDown = true;
	}
}

void XboxController::SetVirtualTriggerButtonSensitivity(float pressedValue, float releasedValue)
{
	m_triggerVirtualButtonPressedValue = pressedValue;
	m_triggerVirtualButtonReleasedValue = releasedValue;
}
