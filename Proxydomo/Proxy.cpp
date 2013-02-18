/**
*	@file	Proxy.cpp
*	@breif	プロクシクラス
*/

#include "stdafx.h"
#include "Proxy.h"
#include <deque>
#include <atlsync.h>
#include "RequestManager.h"
#include "Log.h"

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

		for (auto& manager : m_vecpRequestManager)
			manager->SwitchToInvalid();

		m_threadServer.join();
#if 0
		for (auto& thrd : m_vecpRequestManagerThread)
			thrd->join();
		m_vecpRequestManagerThread.clear();
#endif
	}
}


void CProxy::_ServerThread()
{
	enum { kMaxActiveRequestThread = 30 };
	CCriticalSection	csRequestManager;

	auto funcCreateRequestManagerThread = [this, &csRequestManager](CRequestManager* manager) {
		{
			CCritSecLock	lock(csRequestManager);
			m_vecpRequestManager.emplace_back(std::move(manager));
		}
		std::thread([this, manager, &csRequestManager]() {
			try {
				manager->Manage();
			} catch (GeneralException& e) {
				ATLTRACE(e.msg);
			}

			CCritSecLock	lock(csRequestManager);
			for (auto it = m_vecpRequestManager.begin(); it != m_vecpRequestManager.end(); ++it) {
				if (it->get() == manager) {
					m_vecpRequestManager.erase(it);
					break;
				}
			}
			//delete manager;
		}).detach();
	};
	
	while (m_bServerActive) {
		std::unique_ptr<CSocket> pSock = m_sockServer.Accept();
		if (pSock) {
			auto manager = new CRequestManager(std::move(pSock));

			// 最大接続数を超えた
			// 最大接続数以下になるまでこのスレッドはロックしちゃってもよい
			if (CLog::GetActiveRequestCount() > kMaxActiveRequestThread) {
				do {
					{
						CCritSecLock	lock(csRequestManager);
						for (auto& processingmanager : m_vecpRequestManager) {
							if (processingmanager->IsValid() == false)	// このマネージャーは終了予定なので待っとく...
								break;
							if (processingmanager->IsValid() && processingmanager->IsDoInvalidPossible()) {
								processingmanager->SwitchToInvalid();
								break;
							}
						}
					}
					::Sleep(50);
				} while (CLog::GetActiveRequestCount() > kMaxActiveRequestThread);
			}

			funcCreateRequestManagerThread(manager);

		}
		::Sleep(50);
	}
}








