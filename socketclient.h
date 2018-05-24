#ifndef __SOCKET_CLIENT_H__
#define  __SOCKET_CLIENT_H__

#include <string>

class BaseSock;
namespace std
{
	class thread;
	class mutex;
}

class SocketClient
{
public:
	static const int BUFFER_SIZE;
	typedef unsigned short PacketIDType;
	typedef unsigned short PacketLenType;

	enum ConnectState
	{
		Unknown,
		Success,
		Failed,
	};

protected:
	static const int PACKET_LEN_SIZE;
	static const int PACKET_ID_SIZE;
	static const int PACKET_HEADER_SIZE;

	BaseSock* m_Connection;
	std::thread* m_ConnectThread;
	std::thread* m_ReceiveThread;
	std::thread* m_SendThread;

	typedef struct
	{
		PacketIDType packetId;
		const char* buff;
		long buffLen;
	} PacketSender;

	std::queue<PacketSender>* m_SendQueue;
	std::mutex* m_SendMutex;

	bool m_Abort;

	ConnectState m_ConnectState;

public:
	SocketClient();
	~SocketClient();

	void CreateConnection(const std::string& host, unsigned short port);
	void Disconnect();

	void SendMessageToServer(PacketIDType packetId, const char* buff, long buffLen);

	bool IsConnected();

	ConnectState GetConnectState();

protected:
	void ThreadConnect(const std::string& host, unsigned short port);
	void ThreadReceive();
	void ThreadSend();
};

#endif // __BASE_SOCK_H__