#include "socketclient.h"
#include <thread>
#include <queue>
#include <mutex>

#include "basesock.h"

const int SocketClient::BUFFER_SIZE = 8192;

const int SocketClient::PACKET_LEN_SIZE = sizeof(PacketLenType);

const int SocketClient::PACKET_ID_SIZE = sizeof(PacketIDType);

const int SocketClient::PACKET_HEADER_SIZE = PACKET_LEN_SIZE + PACKET_ID_SIZE;

#define SAFE_DELETE(p) do { delete (p); (p) = nullptr; } while (0)

SocketClient::SocketClient()
	: m_Connection(nullptr)
	, m_ConnectThread(nullptr)
	, m_ReceiveThread(nullptr)
	, m_SendThread(nullptr)
	, m_Abort(false)
	, m_ConnectState(Unknown)
{
	m_SendQueue = new std::queue<PacketSender>();
	m_SendMutex = new std::mutex();
}

SocketClient::~SocketClient()
{
	Disconnect();

	SAFE_DELETE(m_SendQueue);
	SAFE_DELETE(m_SendMutex);
}

void SocketClient::CreateConnection(const std::string& host, unsigned short port)
{
	m_ConnectState = Unknown;

	m_Connection = new BaseSock();
	m_Connection->Create();

	m_ConnectThread = new std::thread(&SocketClient::ThreadConnect, this, host, port);
}

void SocketClient::Disconnect()
{
	m_Abort = true;

	if (m_ReceiveThread)
	{
		m_ReceiveThread->join();
		SAFE_DELETE(m_ReceiveThread);
	}

	if (m_SendThread)
	{
		m_SendThread->join();
		SAFE_DELETE(m_SendThread);
	}

	if (m_ConnectThread)
	{
		m_ConnectThread->join();
		SAFE_DELETE(m_ConnectThread);
	}

	if (m_Connection && m_Connection->isConnected())
	{
		m_Connection->Close();
		SAFE_DELETE(m_Connection);
	}
}

void SocketClient::SendMessageToServer(PacketIDType packetId, const char* buff, long buffLen)
{
	PacketSender sender;
	sender.packetId = packetId;
	sender.buff = buff;
	sender.buffLen = buffLen;

	{
		std::unique_lock<std::mutex> lock(*m_SendMutex);
		m_SendQueue->push(sender);
	}
}

bool SocketClient::IsConnected()
{
	return m_Connection && m_Connection->isConnected();
}

SocketClient::ConnectState SocketClient::GetConnectState()
{
	return m_ConnectState;
}

void SocketClient::ThreadConnect(const std::string& host, unsigned short port)
{
	if (m_Connection->Connect(host, port))
	{
		m_ReceiveThread = new std::thread(&SocketClient::ThreadReceive, this);
		m_SendThread = new std::thread(&SocketClient::ThreadSend, this);

		m_ConnectState = Success;
	}
	else
	{ 
		m_ConnectState = Failed;
	}
}

void SocketClient::ThreadReceive()
{

}

void SocketClient::ThreadSend()
{

}
