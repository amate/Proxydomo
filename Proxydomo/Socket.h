/**
*	@file	Socket.h
*	@brief	ソケットクラス
*/

#pragma once

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <memory>
#include <string>
#include <boost\lexical_cast.hpp>


class GeneralException
{
public:
	GeneralException(const char* errmsg, int e = 0) : err(e)
	{
		strcpy_s(msg, errmsg);
	}

	char	msg[128];
	int		err;
};

class SocketException : public GeneralException
{
public:
	SocketException(const char* errmsg = "socket error", int e = ::WSAGetLastError());
};

// 
struct IPv4Address
{
	sockaddr_in	addr;

	IPv4Address()
	{
		::SecureZeroMemory(&addr, sizeof(addr));
		addr.sin_family	= AF_INET;
	}

	IPv4Address& operator = (sockaddr_in sockaddr)
	{
		addr = sockaddr;
		return *this;
	}

	operator sockaddr*() { return (sockaddr*)&addr; }

	void SetPortNumber(unsigned short port)
	{
		addr.sin_port = htons(port);
	}

	uint16_t	GetPort() const { return ::ntohs(addr.sin_port); }

	bool SetService(const std::string& protocol)
	{
		if (protocol == "http") {
			SetPortNumber(80);
			return true;
		} else if (protocol == "https") {
			SetPortNumber(443);
			return true;
		}
		try {
			SetPortNumber(boost::lexical_cast<uint16_t>(protocol));
			return true;
		} catch (...) {

			return false;
		}
	}

	bool SetHostName(const std::string& IPorHost)
	{
		auto ipret = ::inet_addr(IPorHost.c_str());
		if (ipret != INADDR_NONE) {
			addr.sin_addr.S_un.S_addr = ipret;
			return true;
		}
		addrinfo* result = nullptr;
		addrinfo hinsts = {};
		hinsts.ai_socktype	= SOCK_STREAM;
		hinsts.ai_family	= AF_INET;
		if (::getaddrinfo(IPorHost.c_str(), nullptr, &hinsts, &result) != 0)
			return false;
		sockaddr_in sockaddr = *(sockaddr_in*)result->ai_addr;
		addr.sin_addr.S_un.S_addr = sockaddr.sin_addr.S_un.S_addr;
		::freeaddrinfo(result);
		return true;
	}	
};


class CSocket
{
public:
	CSocket();

	// プログラムの開始と終了前に必ず呼び出してください
	static bool Init();
	static void Term();

	bool	IsConnected() const { return m_sock != 0; }
	IPv4Address GetFromAddress() const;

	void	Bind();
	std::unique_ptr<CSocket>	Accept();

	bool	Connect(IPv4Address addr);

	void	Close();

	bool	IsDataAvailable();
	bool	IsConnectionKilledFromPeer() const { return m_bConnectionKilledFromPeer; }
	bool	Read(char* buffer, int length);
	int		GetLastReadCount() const { return m_nLastReadCount; }

	bool	Write(const char* buffer, int length);
	int		GetLastWriteCount() const { return m_nLastWriteCount; }

private:
	void	_SetBlocking(bool yes);
	void	_SetReuse(bool yes);

	// Data members
	SOCKET	m_sock;
	IPv4Address	m_addrFrom;
	int		m_nLastReadCount;
	int		m_nLastWriteCount;
	bool	m_bConnectionKilledFromPeer;
};

































