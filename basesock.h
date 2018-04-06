#ifndef __BASE_SOCK_H__
#define  __BASE_SOCK_H__
//win32
#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#else
//for ios and andriod
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/ioctl.h>
#define SOCKET int

#endif

#include <errno.h>
#include <string>

#define MAX_RECV_BUFFERSIZE 65535

/* IO error codes */
enum {
	IO_DONE = 0,        /* operation completed successfully */
	IO_TIMEOUT = -1,    /* operation timed out */
	IO_CLOSED = -2,     /* the connection has been closed */
	IO_UNKNOWN = -3
};

bool InitSocketSystem();
void clearSocketSystem();

class BaseSock
{
public:
	SOCKET m_sock;
public:
	BaseSock();
	virtual ~BaseSock();
	bool Create(bool bUDP = false);
	bool Connect(const std::string& host, unsigned short port);
	bool isConnected();
	bool isUDP();
	bool Bind(unsigned short nPort);
	bool Accept(BaseSock& client);
	void Close();
	int Send(const char* buf, long buflen);
	int Recv(char* buf, long buflen);
	int SendTo(const char* buf, int len,
		const struct sockaddr_in* toaddr, int tolen);
	int RecvFrom(char* buf, int len, struct sockaddr_in* fromaddr,
		int* fromlen);
	bool GetPeerName(std::string& strIP, unsigned short &nPort);
	SOCKET GetHandle();
	int SetBlock(bool block);
	void SetSendTimeout(int timeout);
	void SetRecvTimeout(int timeout);

private:
	bool m_UDP;
	bool m_Connected;
	std::string m_Host;
	unsigned short m_Port;
};

#endif //__BASE_SOCK_H__