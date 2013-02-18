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
#ifdef _DEBUG
	std::string	ip;
	uint16_t	port;	
#endif

	IPv4Address();

	IPv4Address& operator = (sockaddr_in sockaddr)
	{
		addr = sockaddr;
		return *this;
	}

	operator sockaddr*() { return (sockaddr*)&addr; }

	void		SetPortNumber(uint16_t port);
	uint16_t	GetPortNumber() const { return ::ntohs(addr.sin_port); }

	bool SetService(const std::string& protocol);

	bool SetHostName(const std::string& IPorHost);
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

































