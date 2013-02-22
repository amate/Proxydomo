/**
*	@file	Proxy.cpp
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
#include "stdafx.h"
#include "Proxy.h"
#include <deque>
#include "RequestManager.h"
#include "Log.h"

CProxy::CProxy(void) : m_bServerActive(false)
{
}


CProxy::~CProxy(void)
{
}


bool CProxy::OpenProxyPort(uint16_t port)
{
	m_sockServer.Bind(port);
	m_bServerActive = true;
	m_threadServer = std::thread(&CProxy::_ServerThread, this);
	return true;
}


void CProxy::CloseProxyPort()
{
	if (m_threadServer.joinable()) {
		m_bServerActive = false;
		
		{
			CCritSecLock	lock(m_csRequestManager);
			for (auto& manager : m_vecpRequestManager)
				manager->SwitchToInvalid();
		}

		m_threadServer.join();

		enum { kMaxRetryCount = 20 };
		int Count = 0;
		while (m_vecpRequestManager.size() != 0 && Count < kMaxRetryCount) {
			::Sleep(50);
			++Count;
		}
	}
}


void CProxy::_ServerThread()
{
	enum { kMaxActiveRequestThread = 30 };

	auto funcCreateRequestManagerThread = [this](CRequestManager* manager) {
		{
			CCritSecLock	lock(m_csRequestManager);
			m_vecpRequestManager.emplace_back(std::move(manager));
		}
		std::thread([this, manager]() {
			try {
				manager->Manage();
			} catch (GeneralException& e) {
				ATLTRACE(e.msg);
			}

			CCritSecLock	lock(m_csRequestManager);
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
						CCritSecLock	lock(m_csRequestManager);
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








