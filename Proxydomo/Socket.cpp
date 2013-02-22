/**
*	@file	Socket.cpp
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
#include "stdafx.h"
#include "Socket.h"
#include <chrono>
#pragma comment(lib, "ws2_32.lib")

#include "DebugWindow.h"


////////////////////////////////////////////////////////////////
// SocketException

SocketException::SocketException(const char* errmsg /*= "socket error"*/, int e /*= ::WSAGetLastError()*/) : GeneralException(errmsg, e)
{
	TRACEIN("SocketException (%d) : %s", e, errmsg);
}

////////////////////////////////////////////////////////////////////
// IPv4Address

IPv4Address::IPv4Address()
{
	::SecureZeroMemory(&addr, sizeof(addr));
	addr.sin_family	= AF_INET;
#ifdef _DEBUG
	port = 0;
#endif
}

void IPv4Address::SetPortNumber(uint16_t port)
{
	addr.sin_port = htons(port);
#ifdef _DEBUG
	this->port = port;
#endif
}


bool IPv4Address::SetService(const std::string& protocol)
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

bool IPv4Address::SetHostName(const std::string& IPorHost)
{
	auto ipret = ::inet_addr(IPorHost.c_str());
	if (ipret != INADDR_NONE) {
		addr.sin_addr.S_un.S_addr = ipret;
#ifdef _DEBUG
		ip = ::inet_ntoa(addr.sin_addr);
#endif
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
#ifdef _DEBUG
		ip = ::inet_ntoa(addr.sin_addr);
#endif
	return true;
}	


////////////////////////////////////////////////////////////////
// CSocket

CSocket::CSocket() : m_sock(0), m_nLastReadCount(0), m_nLastWriteCount(0), m_bConnectionKilledFromPeer(true)
{
}


bool CSocket::Init()
{
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD( 2, 2 );
	int nRet = ::WSAStartup( wVersionRequested, &wsaData );
	ATLASSERT( nRet == 0 );
	if (nRet == 0)
		return true;
	else
		return false;
}

void CSocket::Term()
{
	int nRet = ::WSACleanup();
	ATLASSERT( nRet == 0 );
}


IPv4Address CSocket::GetFromAddress() const
{
	ATLASSERT( m_sock );
	return m_addrFrom;
}

// ポートとソケットを関連付ける
void	CSocket::Bind(uint16_t port)
{
	m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_sock ==  INVALID_SOCKET)
		throw SocketException("Can`t open socket");

	_SetBlocking(false);
	_SetReuse(true);

	sockaddr_in localAddr = { 0 };
	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(port);
	localAddr.sin_addr.s_addr = INADDR_ANY;
	if( ::bind(m_sock, (sockaddr *)&localAddr, sizeof(localAddr)) == SOCKET_ERROR)
		throw SocketException("Can`t bind socket");

	if (::listen(m_sock, SOMAXCONN) == SOCKET_ERROR)
		throw SocketException("Can`t listen",WSAGetLastError());
}

// ポートからの応答を待つ
std::unique_ptr<CSocket>	CSocket::Accept()
{
	int fromSize = sizeof(sockaddr_in);
	sockaddr_in from;
	SOCKET conSock = ::accept(m_sock, (sockaddr *)&from, &fromSize);
	if (conSock ==  INVALID_SOCKET)
		return nullptr;

	// 接続があった
    CSocket*	pSocket = new CSocket;
	pSocket->m_sock = conSock;
	pSocket->m_addrFrom = from;

	//pSocket->_setBlocking(false);
	//pSocket->_setBufSize(65535);

	return std::unique_ptr<CSocket>(std::move(pSocket));;
}

bool	CSocket::Connect(IPv4Address addr)
{
	ATLASSERT( m_sock == 0 );
	m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_sock ==  INVALID_SOCKET)
		throw SocketException("Can`t open socket");
	int ret = ::connect(m_sock, addr, sizeof(sockaddr_in));
	if (ret == SOCKET_ERROR) {
		::closesocket(m_sock);
		m_sock = 0;
		return false;
	} else {
		m_bConnectionKilledFromPeer = false;
		return true;
	}
}

void	CSocket::Close()
{
	if (m_sock) {
		::shutdown(m_sock, SD_SEND);

		//setReadTimeout(2000);
		auto starttime = std::chrono::steady_clock::now();
		char c[1024];
		while (IsDataAvailable()) {
			int len = ::recv(m_sock, c, sizeof(c), 0);
			if (len == 0 || len == SOCKET_ERROR)
				break;
			if (std::chrono::steady_clock::now() - starttime > std::chrono::seconds(5))
				break;
		}
		::shutdown(m_sock, SD_RECEIVE);
		if (::closesocket(m_sock) == SOCKET_ERROR) {
			ATLASSERT( FALSE );
			throw SocketException("closesocket failed");
			//LOG_ERROR("closesocket() error");
		}
		m_sock = 0;
		m_bConnectionKilledFromPeer = true;
	}
}

bool	CSocket::IsDataAvailable()
{
	if (m_sock == 0)
		return false;

	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(m_sock, &readfds);
	TIMEVAL nonwait = {};
	int ret = ::select(0, &readfds, nullptr, nullptr, &nonwait);
	if (ret == SOCKET_ERROR)
		throw SocketException("IsDataAvailable failed");

	if (FD_ISSET(m_sock, &readfds))
		return true;
	else
		return false;

#if 0
	std::shared_ptr<std::remove_pointer<HANDLE>::type> hEvent(::WSACreateEvent(), [](HANDLE h) {
		::WSACloseEvent(h);
	});

	if (hEvent.get() == WSA_INVALID_EVENT)
		throw SocketException("WSACreateEvent failed");

	if (::WSAEventSelect(m_sock, hEvent.get(), FD_READ) == SOCKET_ERROR)
		throw SocketException("WSAEventSelect failed");

	HANDLE h[1] = { hEvent.get() };
	DWORD dwRet = ::WSAWaitForMultipleEvents(1, h, FALSE, 0, FALSE);
	::WSAEventSelect(m_sock, hEvent.get(), 0);
	DWORD dwTemp = 0;
	::ioctlsocket(m_sock, FIONBIO, &dwTemp);
	if (dwRet == WSA_WAIT_EVENT_0)
		return true;
	else
		return false;
#endif
}

bool	 CSocket::Read(char* buffer, int length)
{
	if (m_sock == 0)
		return false;

	ATLASSERT( length > 0 );
	int nLen = ::recv(m_sock, buffer, length, 0);
	m_nLastReadCount = nLen;
	if (nLen == SOCKET_ERROR) {	// 相手から接続が切断された
		throw SocketException("CSocket::Read failed");
		//Close();
		return false;
	}
	if (nLen == 0)
		m_bConnectionKilledFromPeer = true;
	return true;
}


bool	CSocket::Write(const char* buffer, int length)
{
	if (m_sock == 0)
		return false;

	ATLASSERT( length > 0 );
	int nLen = ::send(m_sock, buffer, length, 0);
	if (nLen == SOCKET_ERROR) {
		m_nLastWriteCount = 0;
		if (::WSAGetLastError() == WSAEWOULDBLOCK)
			return true;

		//throw SocketException("CSocket::Write failed");
		//Close();
		return false;
	}
	ATLASSERT( nLen == length );
	m_nLastWriteCount = nLen;
	return true;
}


// --------------------------------------------------
void CSocket::_SetBlocking(bool yes)
{
	unsigned long op = yes ? 0 : 1;
	if (ioctlsocket(m_sock, FIONBIO, &op) == SOCKET_ERROR)
		throw SocketException("Can`t set blocking");
}

// --------------------------------------------------
void CSocket::_SetReuse(bool yes)
{
	unsigned long op = yes ? 1 : 0;
	if (setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&op, sizeof(op)) == SOCKET_ERROR) 
		throw SocketException("Unable to set REUSE");
}

















