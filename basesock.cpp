#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#endif
#include <sys/types.h>
#include <errno.h>
#include "basesock.h"
#ifndef WIN32
#define strnicmp strncasecmp
#endif
#include <memory>

bool InitSocketSystem()
{
#ifdef WIN32
	WSADATA wsadata;
	unsigned short winsock_version = MAKEWORD(1, 1);
	if (WSAStartup(winsock_version, &wsadata))
	{
		return false;
	}
#endif
	return true;
}

void clearSocketSystem()
{
#ifdef WIN32
	WSACleanup();
#endif
}
BaseSock::BaseSock()
{
	m_UDP = false;
	m_sock = -1;
	m_Port = 0;
	m_Connected = false;
}

BaseSock::~BaseSock()
{
	Close();
}

SOCKET BaseSock::GetHandle()
{
	return m_sock;
}

void BaseSock::Close()
{
	if (m_sock != -1)
	{
#ifdef WIN32
		shutdown(m_sock, SD_BOTH);
		closesocket(m_sock);
#else
		shutdown(m_sock, SHUT_RDWR);
		close(m_sock);
#endif
		m_sock = -1;
	}
	m_Connected = false;
}

bool BaseSock::Create(bool udp)
{
	m_UDP = udp;
	if (!m_UDP)
		m_sock = socket(AF_INET, SOCK_STREAM, 0);
	else
	{
		m_sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (m_sock != -1)
		{
			m_Connected = true;
		}
	}

	return (m_sock != -1);
}

bool BaseSock::Connect(const std::string& host, unsigned short port)
{
	//连接ip  
	char ip[128];
	memset(ip, 0, sizeof(ip));
	strncpy_s(ip, host.c_str(), 128);

	void* svraddr = nullptr;
	int error = -1, svraddr_len;
	bool ret = true;
	struct sockaddr_in svraddr_4;
	struct sockaddr_in6 svraddr_6;

	//获取网络协议  
	struct addrinfo *result;
	error = getaddrinfo(ip, NULL, NULL, &result);
	const struct sockaddr *sa = result->ai_addr;
	socklen_t maxlen = 128;
	switch (sa->sa_family) {
	case AF_INET://ipv4  
		if (inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),
			ip, maxlen) < 0) 
		{
			perror(ip);
			ret = false;
			break;
		}
		inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), ip, maxlen);
		svraddr_4.sin_family = AF_INET;
		svraddr_4.sin_port = htons(port);
		svraddr_len = sizeof(svraddr_4);
		svraddr = &svraddr_4;
		break;
	case AF_INET6://ipv6  
		inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr),
			ip, maxlen);

		printf("socket created ipv6\n");

		memset(&svraddr_6, 0, sizeof(svraddr_6));
		svraddr_6.sin6_family = AF_INET6;
		svraddr_6.sin6_port = htons(port);
		if (inet_pton(AF_INET6, ip, &svraddr_6.sin6_addr) < 0) {
			perror(ip);
			ret = false;
			break;
		}
		svraddr_len = sizeof(svraddr_6);
		svraddr = &svraddr_6;
		break;

	default:
		printf("Unknown AF\n");
		ret = false;
	}
	freeaddrinfo(result);
	if (!ret)
	{
		fprintf(stderr, "Cannot Connect the server!\n");
		Close();
		return false;
	}
	int nret = connect(m_sock, (struct sockaddr*)svraddr, svraddr_len);
	if (nret == SOCKET_ERROR)
	{
		Close();
		return false;
	}

	SetBlock(true);

	m_Connected = true;
	return true;
}

long BaseSock::Send(const char* buf, long buflen)
{
	//printf("Send: %.*s\r\n",buflen,buf);
	if (m_sock == -1)
	{
		return -1;
	}

	int sended = 0;
	do
	{
		int len = send(m_sock, buf + sended, buflen - sended, 0);
		if (len < 0)
		{
			break;
		}
		sended += len;
	} while (sended < buflen);
	return sended;
}

long BaseSock::Recv(char* buf, long buflen)
{
	if (m_sock == -1)
	{
		return -1;
	}

	int len = recv(m_sock, buf, buflen, 0);
	//printf("Recv: %.*s\r\n", len, buf);
	return len;
}

bool BaseSock::GetPeerName(std::string& strIP, unsigned short &nPort)
{
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	int addrlen = sizeof(addr);
#ifdef WIN32
	if (getpeername(m_sock, (struct sockaddr*)&addr, &addrlen) != 0)
#else
	if (getpeername(m_sock, (struct sockaddr*) &addr, (socklen_t*)&addrlen)
		!= 0)
#endif
		return false;
	char szIP[64];
#ifdef WIN32
	sprintf_s(szIP, "%u.%u.%u.%u", addr.sin_addr.S_un.S_addr & 0xFF, (addr.sin_addr.S_un.S_addr >> 8) & 0xFF, (addr.sin_addr.S_un.S_addr >> 16) & 0xFF, (addr.sin_addr.S_un.S_addr >> 24) & 0xFF);
#else
	sprintf_s(szIP, "%u.%u.%u.%u", addr.sin_addr.s_addr & 0xFF,
		(addr.sin_addr.s_addr >> 8) & 0xFF, (addr.sin_addr.s_addr >> 16)
		& 0xFF, (addr.sin_addr.s_addr >> 24) & 0xFF);
#endif
	strIP = szIP;
	nPort = ntohs(addr.sin_port);
	return true;
}

bool BaseSock::isConnected()
{
	return (m_sock != -1) && m_Connected;
}

bool BaseSock::isUDP()
{
	return m_UDP;
}

long BaseSock::SendTo(const char* buf, int len,
	const struct sockaddr_in* toaddr, int tolen)
{
	if (m_sock == -1)
	{
		return -1;
	}
	return sendto(m_sock, buf, len, 0, (const struct sockaddr*) toaddr, tolen);
}

long BaseSock::RecvFrom(char* buf, int len, struct sockaddr_in* fromaddr,
	int* fromlen)
{
	if (m_sock == -1)
	{
		return -1;
	}
#ifdef WIN32
	return recvfrom(m_sock, buf, len, 0, (struct sockaddr*)fromaddr, fromlen);
#else
	return recvfrom(m_sock, buf, len, 0, (struct sockaddr*) fromaddr,
		(socklen_t*)fromlen);
#endif
}

bool BaseSock::Bind(unsigned short nPort)
{
	if (m_sock == -1)
	{
		return false;
	}
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
#ifdef WIN32
	sin.sin_addr.S_un.S_addr = 0;
#else
	sin.sin_addr.s_addr = 0;
#endif
	memset(sin.sin_zero, 0, 8);
	sin.sin_port = htons(nPort);
	if (::bind(m_sock, (sockaddr*)&sin, sizeof(sockaddr_in)) != 0)
		return false;
	listen(m_sock, 1024);
	m_Connected = true;
	return true;
}

bool BaseSock::Accept(BaseSock& client)
{
	if (m_sock == -1)
	{
		return false;
	}
	client.m_sock = accept(m_sock, NULL, NULL);
	client.m_Connected = true;
	return (client.m_sock != -1);
}

int BaseSock::SetBlock(bool block)
{
	int bufsize = MAX_RECV_BUFFERSIZE;
	setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, (char*)&bufsize, sizeof(bufsize));
	setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (char*)&bufsize, sizeof(bufsize));

	int flag = 1;
	if (setsockopt(m_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag))) 
	{
		printf("Couldn't setsockopt(TCP_NODELAY)\n");
		return -1;
	}

	if (block)
	{
		unsigned long rb = 1;
#ifdef WIN32
		return ioctlsocket(m_sock, FIONBIO, &rb);
#else
		return ioctl(m_sock, FIONBIO, &rb);
#endif
	}

	return 0;
}

void BaseSock::SetSendTimeout(int timeout)
{
	setsockopt(m_sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
}

void BaseSock::SetRecvTimeout(int timeout)
{
	setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
}
