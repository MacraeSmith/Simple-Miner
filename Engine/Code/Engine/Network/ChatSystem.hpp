#pragma once
#include "Engine/Core/EventSystem.hpp"
#include <string>
class DevConsole;

class ChatSystem
{
	friend class DevConsole;
protected:
	explicit ChatSystem(DevConsole* devConsole);
	~ChatSystem();
	void BeginFrame();
	void SubmitChatMessage(std::string const& message, bool remoteMessage = false);
	void SetName(std::string const& name);
	std::string GetName() const{return m_name;}
	static constexpr char CHAT_MESSAGE_MARKER = '>';

private:

private:
	DevConsole* m_devConsole = nullptr;
	std::string m_name = "Private Player";
};

