#include "socketclient.h"
#include <thread>
#include <mutex>

#if defined(_ALLBSD_SOURCE) || defined(__APPLE__)
#include <machine/endian.h>
#elif defined(__linux__) || defined(__CYGWIN__)
#include <endian.h>
#endif

#include "basesock.h"

#define SAFE_DELETE(p) do { delete (p); (p) = nullptr; } while (0)
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define IS_LITTLE_ENDIAN
#endif

const int SocketClient::BUFFER_SIZE = 8192;

const int SocketClient::PACKET_LEN_SIZE = sizeof(PacketLenType);

const int SocketClient::PACKET_ID_SIZE = sizeof(PacketIDType);

const int SocketClient::PACKET_HEADER_SIZE = PACKET_LEN_SIZE + PACKET_ID_SIZE;

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
	m_SendCondition = new std::condition_variable();
}

SocketClient::~SocketClient()
{
	Disconnect();

	SAFE_DELETE(m_SendCondition);
	SAFE_DELETE(m_SendMutex);
	SAFE_DELETE(m_SendQueue);
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
		m_SendCondition->notify_one();
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
	int buffSize = BUFFER_SIZE;
	char* buffer = new char[buffSize];

	int contentSize = 0;

	while (!m_Abort)
	{
		size_t recvSize;
		auto recvRet = m_Connection->Recv(buffer, buffSize, &recvSize);
		if (recvRet == IO_DONE)
		{

		}
	}
}

void SocketClient::ThreadSend()
{
	int buffSize = BUFFER_SIZE;
	char* buffer = new char[buffSize];

	while (!m_Abort)
	{
		PacketSender packetSender;
		{
			std::unique_lock<std::mutex> lock(*m_SendMutex);
			if (m_SendQueue->empty())
			{
				m_SendCondition->wait(lock);
			}

			if (m_SendQueue->empty())
			{
				continue;
			}

			packetSender = m_SendQueue->front();
			m_SendQueue->pop();
		}

		PacketIDType packetId = packetSender.packetId;
		const char* data = packetSender.buff;
		
		int messageLen = packetSender.buffLen;
		PacketLenType packetSize = messageLen + PACKET_HEADER_SIZE;
		if (packetSize > buffSize)
		{
			delete[] buffer;
			for (; buffSize < packetSize; buffSize <<= 1)
			{
			}
			buffer = new char[buffSize];
		}
		PacketLenType packetSizeBytes = packetSize;
#ifndef IS_LITTLE_ENDIAN
		packetSizeBytes = htobe16(packetSize);
		packetId = htobe16(packetId);
#endif
		memcpy_s(buffer, buffSize, &packetSizeBytes, PACKET_LEN_SIZE);
		buffer += PACKET_LEN_SIZE;
		memcpy_s(buffer, buffSize - PACKET_LEN_SIZE, &packetId, PACKET_ID_SIZE);
		buffer += PACKET_ID_SIZE;
		memcpy_s(buffer, buffSize - PACKET_HEADER_SIZE, packetSender.buff, messageLen);

		if (m_Connection->Send(buffer, packetSize) != IO_DONE)
		{
			m_ConnectState = Failed;
			break;
		}
	}
}
