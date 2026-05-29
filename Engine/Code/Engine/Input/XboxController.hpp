#pragma once
#include "Engine/Input/KeyButtonState.hpp"
#include "Engine/Input/AnalogJoystick.hpp"

enum class XboxButtonID
{
	BUTTON_A,
	BUTTON_B,
	BUTTON_X,
	BUTTON_Y,
	BUTTON_BACK,
	BUTTON_START,
	BUTTON_LSHOULDER,
	BUTTON_RSHOULDER,
	BUTTON_LTHUMB,
	BUTTON_RTHUMB,
	BUTTON_DPAD_RIGHT,
	BUTTON_DPAD_UP,
	BUTTON_DPAD_LEFT,
	BUTTON_DPAD_DOWN,
	BUTTON_VIRTUAL_LEFT_TRIGGER_BUTTON,
	BUTTON_VIRTUAL_RIGHT_TRIGGER_BUTTON,
	NUM_BUTTONS,
};

constexpr float MAX_ANALOG_RAW_VALUE = 32767.f;
constexpr float MIN_ANALOG_RAW_VALUE = -32768.f;

class XboxController
{
	friend class InputSystem;
public:
	XboxController();
	~XboxController();

	bool					IsConnected() const;
	int						GetControllerID() const;
	AnalogJoystick const&	GetLeftStick() const;
	AnalogJoystick const&	GetRightStick() const;
	float					GetLeftTrigger() const;
	float					GetRightTrigger() const;

	KeyButtonState const&	GetButton(XboxButtonID buttonID) const;
	bool					IsButtonDown(XboxButtonID buttonID) const;
	bool					WasButtonJustPressed(XboxButtonID buttonID) const;
	bool					WasButtonJustReleased(XboxButtonID buttonID) const;
	void					UpdateVirtualTriggerButton(XboxButtonID buttonId, float triggerAnalogValue);

	void					SetVirtualTriggerButtonSensitivity(float pressedValue, float releasedValue);

private:
	void UpdateStatus();
	void Reset();
	void UpdateJoystick(AnalogJoystick& out_joystick, short rawX, short rawY);
	void UpdateTrigger(float& out_triggerValue, unsigned char rawValue);
	void UpdateButton(XboxButtonID buttonID, unsigned short wButtons, unsigned short buttonFlag);

private:
	int	m_controllerID = -1;
	bool m_isConnected = false;
	float m_leftTrigger = 0.f;
	float m_rightTrigger = 0.f;
	KeyButtonState m_buttons[ (int)XboxButtonID::NUM_BUTTONS ] = {};
	AnalogJoystick m_leftStick;
	AnalogJoystick m_rightStick;

	//Can be adjusted 
	float m_triggerVirtualButtonPressedValue = 0.75f;
	float m_triggerVirtualButtonReleasedValue = 0.25f;
};

