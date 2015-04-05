/**
*	@file	Proxy.h
*	@breif	プロクシクラス
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

#include <thread>
#include <list>
#include <atlsync.h>
#include "Socket.h"
#include "ThreadPool.h"

class CRequestManager;

class CProxy
{
public:
	CProxy();
	~CProxy();

	bool	OpenProxyPort(uint16_t port);
	void	CloseProxyPort();

	void	AbortAllConnection();

private:
	void	_ServerThread();

	// Data members
	CSocket	m_sockServer;
	std::thread	m_threadServer;
	bool	m_bServerActive;

#ifdef _WIN64
	CThreadPool	m_threadPool;
#endif
	CCriticalSection	m_csRequestManager;
	std::list<std::unique_ptr<CRequestManager>>	m_requestManagerList;
};

