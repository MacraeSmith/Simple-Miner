#pragma once
#include <deque>
#include <vector>
#include <string>

typedef std::vector<std::string> Strings;
enum class NetworkState : int
{
	UNINITIALIZED,
	IDLE,
	SERVER_LISTENING,
	CLIENT_CONNECTING,
	CLIENT_CONNECTED,
	COUNT,
};


struct NetworkSystemConfig
{
	char const* m_hostAddress = "127.0.0.1";
	char const* m_hostPort = "3100";
	size_t m_recvBufferSize = 2048;
	size_t m_sendBufferSize = 2048;
};

constexpr size_t BUFFER_SIZE = 2048;

class NetworkSystem
{
public:
	explicit NetworkSystem(NetworkSystemConfig const& config);
	~NetworkSystem(){};

	//Universal functions for both Clients and Servers
	//-----------------------------------------------------------------------------------------------------------------
	void Startup();
	void Disconnect();
	void Shutdown();
	void BeginFrame();
	void Send(std::string const& string);

	int GetNumReceivedCommands() const{return (int)m_recvQueue.size();}
	Strings GetReceiveQueue();
	std::string PullFirstReceiveCommand();
	void PushBackToReceiveQueue(std::string const& message);

	NetworkState GetStatus() const {return m_state;}
	std::string GetStatusAsString();
	int GetNumClients();
	std::string GetLocalIPv4Address();
	void SetHostAddress(std::string const& ip);
	void SetHostPort(std::string const& port);


	//Server Functions
	//-----------------------------------------------------------------------------------------------------------------
	void StartServer(std::string const& hostPort = "INVALID_PORT");

	//Client Functions
	//-----------------------------------------------------------------------------------------------------------------
	void StartClient(std::string const& ipAddress = "INVALID_ADDRESS", std::string const& portNumber = "INVALID_PORT");
	bool IsConnected() const;
	bool IsServer() const;
	bool IsClient() const;


private:

	void AppendToBuffer(char* buffer, size_t& bufferOffset, size_t bufferSize, char const* dataToAppend);
	void AddToReceiveQueue(std::string const& string);

public:
	std::string m_hostAddress = "127.0.0.1";
	std::string m_hostPort = "3100";

private:
	NetworkSystemConfig m_config;
	NetworkState m_state = NetworkState::UNINITIALIZED;
	char* m_sendBuffer = nullptr;
	char* m_receiveBuffer = nullptr;	
	size_t m_sendBufferOffset = 0;
	std::deque<std::string> m_recvQueue;

	//Server Variables
	//-----------------------------------------------------------------------------------------------------------------
	uint64_t m_listenSocket = (uint64_t)~0; //currently invalid socket
	std::vector<uint64_t> m_clientSockets;


	//Client Variables
	//-----------------------------------------------------------------------------------------------------------------
	uint64_t m_connectionToServer = (uint64_t)~0; //currently invalid socket

};

