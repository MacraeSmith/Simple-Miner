#include "Engine/Network/ChatSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Network/NetworkSystem.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/StringUtils.hpp"

ChatSystem::ChatSystem(DevConsole* devConsole)
	:m_devConsole(devConsole)
{
}

ChatSystem::~ChatSystem()
{
	m_devConsole = nullptr;
}

void ChatSystem::BeginFrame()
{
	if (!g_networkSystem || !g_networkSystem->IsConnected())
		return;

	//Looks at all recvQueue commands, consumes "Echo" commands and pushes rest to be consumed elsewhere
	const int NUM_COMMANDS_TO_PARSE = g_networkSystem->GetNumReceivedCommands();
	for (int i = 0; i < (NUM_COMMANDS_TO_PARSE); ++i)
	{
		std::string recvMessage = g_networkSystem->PullFirstReceiveCommand();
		Strings recvWords = SplitStringOnDelimiter(recvMessage, ' ');
		if (GetLowercase(recvWords[0]) == "echo" && recvWords.size() > 1)
		{
			std::string messageToPrint = recvWords[1];
			for (int n = 2; n < (int)recvWords.size(); ++n)
			{
				messageToPrint.append(Stringf(" %s", recvWords[n].c_str()));
			}

			SubmitChatMessage(messageToPrint, true);
		}

		else
		{
			g_networkSystem->PushBackToReceiveQueue(recvMessage);
		}
	}
}

void ChatSystem::SubmitChatMessage(std::string const& message, bool remoteMessage)
{
	if (!remoteMessage)
	{	
		if(message.size() < 1)
			return;

		m_devConsole->AddLine(CHAT_MESSAGE, message);

		if (g_networkSystem && g_networkSystem->IsConnected())
		{
			std::string sendMessage = message;
			sendMessage.erase(0, 1);
			g_devConsole->Execute(Stringf("RemoteCmd cmd=Echo <%s> %s", m_name.c_str(), sendMessage.c_str()));
		}
	}

	else
	{
		m_devConsole->AddLine(CHAT_MESSAGE_REMOTE, message);
	}
}

void ChatSystem::SetName(std::string const& name)
{
	m_name = name;
}


