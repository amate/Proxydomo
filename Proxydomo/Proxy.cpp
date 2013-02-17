/**
*	@file	Proxy.cpp
*	@breif	プロクシクラス
*/

#include "stdafx.h"
#include "Proxy.h"
#include "RequestManager.h"

CProxy::CProxy(void) : m_bServerActive(false)
{
}


CProxy::~CProxy(void)
{
}


bool CProxy::OpenProxyPort()
{
	m_sockServer.Bind();
	m_bServerActive = true;
	m_threadServer = std::thread(&CProxy::_ServerThread, this);
	return true;
}


void CProxy::CloseProxyPort()
{
	if (m_threadServer.joinable()) {
		m_bServerActive = false;
		m_threadServer.join();

		for (auto& thrd : m_vecpRequestManagerThread)
			thrd->join();
		m_vecpRequestManagerThread.clear();
	}
}


void CProxy::_ServerThread()
{
	while (m_bServerActive) {
		std::unique_ptr<CSocket> pSock = m_sockServer.Accept();
		if (pSock) {
			auto manager = new CRequestManager(std::move(pSock));
			std::thread([manager]() {
				try {
				manager->Manage();
				} catch (GeneralException& e) {
					ATLTRACE(e.msg);
				}
				delete manager;
			}).detach();
#if 0
			m_vecpRequestManagerThread.emplace_back(new std::thread([manager]() {
				manager->Manage();
				delete manager;
			}));
#endif
		}
		::Sleep(50);
	}
}








