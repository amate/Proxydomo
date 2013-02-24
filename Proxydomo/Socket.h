/**
*	@file	Socket.h
*	@brief	ソケットクラス
*/
/**
	this file is part of Proxydomo
	Copyright (C) amate 2013-

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
	std::shared_ptr<addrinfo>	addrinfoList;
	addrinfo*	current_addrinfo;

	IPv4Address();

	IPv4Address& operator = (sockaddr_in sockaddr);

	operator sockaddr*() { return (sockaddr*)&addr; }

	void		SetPortNumber(uint16_t port);
	uint16_t	GetPortNumber() const { return ::ntohs(addr.sin_port); }

	bool SetService(const std::string& protocol);

	bool SetHostName(const std::string& IPorHost);
	bool SetNextHost();
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

	void	Bind(uint16_t port);
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

































