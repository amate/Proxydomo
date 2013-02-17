/**
*	@file	Proxy.h
*	@breif	プロクシクラス
*/

#pragma once

#include <thread>
#include <vector>
#include "Socket.h"


class CProxy
{
public:
	CProxy();
	~CProxy();

	bool	OpenProxyPort();
	void	CloseProxyPort();

private:
	void	_ServerThread();

	// Data members
	CSocket	m_sockServer;
	std::thread	m_threadServer;
	bool	m_bServerActive;
	std::vector<std::unique_ptr<std::thread>>	m_vecpRequestManagerThread;
};

