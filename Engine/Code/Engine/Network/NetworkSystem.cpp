#include "Engine/Network/NetworkSystem.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/StringUtils.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

NetworkSystem::NetworkSystem(NetworkSystemConfig const& config)
	:m_config(config)
{
}

void NetworkSystem::Startup()
{
	m_sendBuffer = new char[m_config.m_sendBufferSize];
	m_receiveBuffer = new char[m_config.m_recvBufferSize];
	m_hostAddress = m_config.m_hostAddress;
	m_hostPort = m_config.m_hostPort;

	WSADATA data;
	int errorCode = WSAStartup(MAKEWORD(2,2), &data);
	if (errorCode != 0)
	{
		ERROR_RECOVERABLE(Stringf("WSAStartup failed with error: %i", errorCode));
	}

	m_state = NetworkState::IDLE;
}

void NetworkSystem::Disconnect()
{
	for (int i = 0; i < (int)m_clientSockets.size(); ++i)
	{
		closesocket(m_clientSockets[i]);
	}
	closesocket(m_listenSocket);
	closesocket(m_connectionToServer);

	m_sendBufferOffset = 0;
	m_recvQueue.clear();
	m_state = NetworkState::IDLE;
	m_clientSockets.clear();
	FireEvent("NetworkDisconnected");
}

void NetworkSystem::Shutdown()
{
	Disconnect();
	int errorCode = WSACleanup();
	if (errorCode == SOCKET_ERROR)
	{
		ERROR_RECOVERABLE(Stringf("Error Shutting down Network System. Error Code: %i", WSAGetLastError()));
	}
	m_state = NetworkState::UNINITIALIZED;
}

void NetworkSystem::BeginFrame()
{
	m_recvQueue.clear();

	switch (m_state)
	{
	case NetworkState::SERVER_LISTENING:
	{
		//listen for new connections
		SOCKET newClientSocket = accept(m_listenSocket, NULL, NULL);
		if (newClientSocket != INVALID_SOCKET)
		{
			unsigned long blockinMode = 1;
			ioctlsocket(newClientSocket, FIONBIO, &blockinMode);
			m_clientSockets.push_back(newClientSocket);
			DebugAddMessage("Client Connected", 5.f);
			FireEvent("NetworkReceivedClient");
		}
		
		for (int i = 0; i < (int)m_clientSockets.size(); ++i)
		{
			SOCKET clientSocket = m_clientSockets[i];

			if (m_sendBufferOffset > 0)
			{
				//send
				send(clientSocket, m_sendBuffer, (int)m_sendBufferOffset, 0);
			}

			//receive
			int sizeData = recv(clientSocket, m_receiveBuffer, (int)m_config.m_recvBufferSize, 0);
			if (sizeData == 0) // socket was closed
			{
				closesocket(clientSocket);
				m_clientSockets.erase(m_clientSockets.begin() + i);
				i -= 1;
				DebugAddMessage("Client Disconnected", 5.f);
				FireEvent("NetworkLostClient");
			}

			if (sizeData > 0)
			{
				std::string recievedMessage = m_receiveBuffer;
				m_recvQueue.push_back(recievedMessage);
			}
		}

		break;
	}
	case NetworkState::CLIENT_CONNECTING:
	{
		DebugAddMessage("Connecting...", 0.f);
		fd_set writeSockets;
		fd_set exceptSockets;
		FD_ZERO(&writeSockets);
		FD_ZERO(&exceptSockets);
		FD_SET(m_connectionToServer, &writeSockets);
		FD_SET(m_connectionToServer, &exceptSockets);
		timeval waitTime = {};
		int result = select(0, NULL, &writeSockets, &exceptSockets, &waitTime);
		if (result == SOCKET_ERROR)
		{
			Disconnect();
			StartClient();
			return;
		}

		result = FD_ISSET(m_connectionToServer, &exceptSockets);
		if (result > 0)
		{
			Disconnect();
			StartClient();
			return;
		}

		result = FD_ISSET(m_connectionToServer, &writeSockets);
		if (result > 0)
		{
			m_state = NetworkState::CLIENT_CONNECTED; // connection succeeded
			DebugAddMessage("Connected To Server", 5.f);
			FireEvent("NetworkConnectedToServer");
		}
		
		break;
	}
	case NetworkState::CLIENT_CONNECTED:
	{
		//send
		if (m_sendBufferOffset > 0)
		{
			send(m_connectionToServer, m_sendBuffer, (int)m_sendBufferOffset, 0);
		}

		//receive
		int sizeData = recv(m_connectionToServer, m_receiveBuffer, (int)m_config.m_recvBufferSize, 0);
		if (sizeData == 0)
		{
			Disconnect();
			DebugAddMessage("Disconnecting Client", 5.f);
			DebugAddMessage("Server Was Closed", 5.f);
		}

		if (sizeData > 0)
		{
			std::string recievedMessage = m_receiveBuffer;
			m_recvQueue.push_back(recievedMessage);
		}
		break;
	}
	default:
		break;
	}

	m_sendBufferOffset = 0;
}


void NetworkSystem::Send(std::string const& string)
{
	AppendToBuffer(m_sendBuffer, m_sendBufferOffset, m_config.m_sendBufferSize, string.c_str());
}

Strings NetworkSystem::GetReceiveQueue()
{
	Strings queue;
	for (int i = 0; i < (int)m_recvQueue.size(); ++i)
	{
		queue.push_back(m_recvQueue[i]);
	}

	return queue;
}


std::string NetworkSystem::PullFirstReceiveCommand()
{
	std::string message = m_recvQueue.front();
	m_recvQueue.pop_front();
	return message;
}

void NetworkSystem::PushBackToReceiveQueue(std::string const& message)
{
	m_recvQueue.push_back(message);
}

std::string NetworkSystem::GetStatusAsString()
{
	switch (m_state)
	{
		case NetworkState::UNINITIALIZED: return "Uninitialized";
		case NetworkState::IDLE: return "Idle";
		case NetworkState::SERVER_LISTENING: return "Server Listening";
		case NetworkState::CLIENT_CONNECTING: return "Client Connecting...";
		case NetworkState::CLIENT_CONNECTED: return "Client Connected";
	}
	return "ERROR: Could Not Receive Network Status";
}

int NetworkSystem::GetNumClients()
{
	return (int)m_clientSockets.size();
}

std::string NetworkSystem::GetLocalIPv4Address()
{
	char hostname[256];
	if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR)
	{
		return "127.0.0.1";
	}

	struct addrinfo hints, * result;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(hostname, nullptr, &hints, &result) != 0)
	{
		return "127.0.0.1";
	}

	struct sockaddr_in* sockaddr_ipv4 = (struct sockaddr_in*)result->ai_addr;

	//Convert to std::string
	char ipBuffer[INET_ADDRSTRLEN];
	std::string ipAddress = "127.0.0.1";
	if (inet_ntop(AF_INET, &sockaddr_ipv4->sin_addr, ipBuffer, INET_ADDRSTRLEN) != nullptr)
	{
		ipAddress = ipBuffer;
	}

	freeaddrinfo(result);
	return ipAddress;


}

void NetworkSystem::SetHostAddress(std::string const& ip)
{
	m_hostAddress = ip;
}

void NetworkSystem::SetHostPort(std::string const& port)
{
	m_hostPort = port;
}

void NetworkSystem::AppendToBuffer(char* buffer, size_t& bufferOffset, size_t bufferSize, char const* dataToAppend)
{
	size_t strLen = strlen(dataToAppend) + 1;
	if (bufferOffset + strLen >= bufferSize)
	{
		ERROR_RECOVERABLE("Not enough space to append to network buffer");
	}

	memcpy(buffer + bufferOffset, dataToAppend, strLen);
	bufferOffset += strLen;
}

void NetworkSystem::AddToReceiveQueue(std::string const& string)
{
	Strings dataStrings = SplitStringOnDelimiter(string,'\0');
	for (int i = 0; i < (int)dataStrings.size(); ++i)
	{
		m_recvQueue.push_back(dataStrings[i]);
	}
}

bool NetworkSystem::IsConnected() const
{
	return m_state == NetworkState::SERVER_LISTENING || m_state == NetworkState::CLIENT_CONNECTED;
}

bool NetworkSystem::IsServer() const
{
	return m_state == NetworkState::SERVER_LISTENING;
}

bool NetworkSystem::IsClient() const
{
	return m_state == NetworkState::CLIENT_CONNECTING || m_state == NetworkState::CLIENT_CONNECTED;
}

void NetworkSystem::StartClient(std::string const& ipAddress, std::string const& portNumber)
{
	if(IsConnected() || m_state == NetworkState::CLIENT_CONNECTING)
		return;

	m_connectionToServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	unsigned long blockingMode = 1;
	ioctlsocket(m_connectionToServer, FIONBIO, &blockingMode);

	m_hostAddress = ipAddress != "INVALID_ADDRESS" ? ipAddress : m_config.m_hostAddress;
	m_hostPort = portNumber != "INVALID_PORT" ? portNumber : m_config.m_hostPort;

	//begin connecting to a server
	IN_ADDR addr;
	int result = inet_pton(AF_INET, m_hostAddress.c_str(), &addr);
	if (result == 0)
	{
		DebugAddMessage("Client tried to connect to Invalid IPv4 Address", 5.f, Rgba8::RED, Rgba8::RED);
		return;
	}

	uint32_t serverIPAddressU32 = ntohl(addr.S_un.S_addr);
	uint16_t serverPortU16 = (unsigned short)atoi(m_hostPort.c_str()); 
	sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_addr.S_un.S_addr = htonl(serverIPAddressU32);
	saddr.sin_port = htons(serverPortU16);

	connect(m_connectionToServer, (sockaddr*)(&saddr), (int)sizeof(saddr));
	m_state = NetworkState::CLIENT_CONNECTING;
}

void NetworkSystem::StartServer(std::string const& hostPort)
{
	if(IsConnected())
		return;

	m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	unsigned long blockingMode = 1; //set listen socket to non-blocking
	ioctlsocket(m_listenSocket, FIONBIO, &blockingMode);
	
	uint32_t myIPAddressU32 = INADDR_ANY;

	m_hostPort = hostPort != "INVALID_PORT" ? hostPort : m_config.m_hostPort;
	uint16_t myListenPortU64 = (unsigned short)atoi(m_hostPort.c_str());

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = htonl(myIPAddressU32);
	addr.sin_port = htons(myListenPortU64);
	int result = bind(m_listenSocket, (sockaddr*)&addr, (int)sizeof(addr));
	if (result == SOCKET_ERROR)
	{
		ERROR_RECOVERABLE(Stringf("Listen Server failed to bind socket error: %i", WSAGetLastError()));
		closesocket(m_listenSocket);
		WSACleanup();
		return;
	}

	result = listen(m_listenSocket, SOMAXCONN);
	if (result == SOCKET_ERROR)
	{
		ERROR_RECOVERABLE(Stringf("Listen Server failed to start listen error: %i", WSAGetLastError()));
		closesocket(m_listenSocket);
		WSACleanup();
		return;
	}

	m_state = NetworkState::SERVER_LISTENING;
	DebugAddMessage("Server Active", 5.f);
}

