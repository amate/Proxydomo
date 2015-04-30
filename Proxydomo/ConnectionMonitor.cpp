/**
*	@file	ConnectionMonitor.cpp
*/

#include "stdafx.h"
#include "ConnectionMonitor.h"
#include <boost\property_tree\ini_parser.hpp>
#include <boost\property_tree\ptree.hpp>
#include "RequestManager.h"
#include "Misc.h"
#include "Logger.h"


/////////////////////////////////////////////////////////
// ConnectionData

ConnectionData::ConnectionData() : inStep(STEP::STEP_START), outStep(STEP::STEP_START)
{}

void	ConnectionData::SetVerb(const std::wstring& verb)
{
	CCritSecLock lock(cs);
	this->verb = verb;
	CConnectionManager::UpdateNotify(this);
}

void	ConnectionData::SetUrl(const std::wstring& url)
{
	CCritSecLock lock(cs);
	this->url = url;
	CConnectionManager::UpdateNotify(this);
}

void	ConnectionData::SetInStep(STEP in)
{
	CCritSecLock lock(cs);
	inStep = in;
	CConnectionManager::UpdateNotify(this);
}

void	ConnectionData::SetOutStep(STEP out)
{
	CCritSecLock lock(cs);
	outStep = out;
	CConnectionManager::UpdateNotify(this);
}


/////////////////////////////////////////////////////////
// CConnectionManager

CCriticalSection			CConnectionManager::s_csList;
std::list<ConnectionData>	CConnectionManager::s_connectionDataList;
std::function<void(ConnectionData*, CConnectionManager::UpdateCategory)> CConnectionManager::s_funcCallback;

ConnectionData*	CConnectionManager::CreateConnectionData()
{
	CCritSecLock lock(s_csList);
	s_connectionDataList.emplace_front();
	ConnectionData* conData = &s_connectionDataList.front();
	conData->itThis = s_connectionDataList.begin();
	if (s_funcCallback) {
		s_funcCallback(conData, kAddConnection);
	}
	return conData;
}
void			CConnectionManager::RemoveConnectionData(ConnectionData* conData)
{
	CCritSecLock lock(s_csList);
	if (s_funcCallback) {
		s_funcCallback(conData, kRemoveConnection);
	}
	s_connectionDataList.erase(conData->itThis);
	
}

// for CConnectionManagerWindow
void CConnectionManager::RegisterCallback(std::function<void(ConnectionData*, UpdateCategory)> callback)
{
	CCritSecLock lock(s_csList);
	s_funcCallback = callback;
	for (auto rit = s_connectionDataList.rbegin(); rit != s_connectionDataList.rend(); ++rit) {
		s_funcCallback(&*rit, kAddConnection);
	}

}
void CConnectionManager::UnregisterCallback()
{
	CCritSecLock lock(s_csList);
	s_funcCallback = nullptr;
}

// for ConnectionData
void CConnectionManager::UpdateNotify(ConnectionData* conData)
{
	CCritSecLock lock(s_csList);
	if (s_funcCallback) {
		s_funcCallback(conData, kUpdateConnection);
	}
}




///////////////////////////////////////////////////////
// CConnectionMonitorWindow

void	CConnectionMonitorWindow::ShowWindow()
{
	if (IsWindowVisible() == FALSE) {
		CConnectionManager::RegisterCallback(
			[this](ConnectionData* conData, CConnectionManager::UpdateCategory updateCategory)
		{
			auto funcSetItem = [this, conData](int iItem) {
				LVITEM	Item = {};
				Item.mask = LVIF_TEXT;
				Item.iItem = iItem;

				Item.iSubItem = 0;
				Item.pszText = (LPWSTR)STEPtoString(conData->outStep);
				m_connectionListView.SetItem(&Item);

				Item.iSubItem = 1;
				Item.pszText = (LPWSTR)STEPtoString(conData->inStep);
				m_connectionListView.SetItem(&Item);

				Item.iSubItem = 2;
				Item.pszText = (LPWSTR)conData->verb.c_str();
				m_connectionListView.SetItem(&Item);

				Item.iSubItem = 3;
				Item.pszText = (LPWSTR)conData->url.c_str();
				m_connectionListView.SetItem(&Item);
			};

			auto funcGetIndex = [this, conData]() -> int {
				int count = m_connectionListView.GetItemCount();
				for (int i = 0; i < count; ++i) {
					if (conData == (ConnectionData*)m_connectionListView.GetItemData(i)) {
						return i;
					}
				}
				ATLASSERT(FALSE);
				return -1;
			};

			switch (updateCategory) {
			case CConnectionManager::kAddConnection:
			{
				m_connectionListView.InsertItem(0, _T(""));
				m_connectionListView.SetItemData(0, (DWORD_PTR)conData);
				funcSetItem(0);

				SetWindowText((boost::wformat(L"Connection Monitor - [%02d]")
					% (int)CConnectionManager::s_connectionDataList.size()).str().c_str());
			}
			break;

			case CConnectionManager::kUpdateConnection:
			{
				int i = funcGetIndex();
				if (i != -1) {
					funcSetItem(i);
				}
			}
			break;

			case CConnectionManager::kRemoveConnection:
			{
				int i = funcGetIndex();
				if (i != -1) {
					CCritSecLock lock(m_csConnectionCloseList);
					m_connectionCloseList.emplace_front();
					DWORD_PTR itemData = (DWORD_PTR)&m_connectionCloseList.front();
					lock.Unlock();

					m_connectionListView.SetItemData(i, itemData);
					RECT rcItem;
					m_connectionListView.GetItemRect(i, &rcItem, LVIR_BOUNDS);
					m_connectionListView.InvalidateRect(&rcItem);
					SetTimer(itemData, 1500);

					SetWindowText((boost::wformat(L"Connection Monitor - [%02d]")
						% (int)(CConnectionManager::s_connectionDataList.size() - 1)).str().c_str());
				}
			}
			break;

			}
		});
	}
	__super::ShowWindow(TRUE);
}


BOOL CConnectionMonitorWindow::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	m_connectionListView = GetDlgItem(IDC_LIST_CONNECTION);
	m_connectionListView.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER);

	struct Column {
		LPCTSTR	columnName;
		int		columnFormat;
		int		defaultcolumnWidth;
	};
	Column	clm[] = {
		{ _T("outStep"), LVCFMT_LEFT, 60 },
		{ _T("InStep"), LVCFMT_LEFT, 60 },
		{ _T("Verb"),  LVCFMT_LEFT, 50 },
		{ _T("URL"),  LVCFMT_LEFT, 300 },
	};

	int nCount = _countof(clm);
	for (int i = 0; i < nCount; ++i) {
		m_connectionListView.InsertColumn(i, clm[i].columnName, clm[i].columnFormat, clm[i].defaultcolumnWidth);
	}

	DlgResize_Init(true, false, WS_THICKFRAME);

	using namespace boost::property_tree;

	CString settingsPath = Misc::GetExeDirectory() + _T("settings.ini");
	std::ifstream fs(settingsPath);
	if (fs) {
		ptree pt;
		try {
			read_ini(fs, pt);
		}
		catch (...) {
			ERROR_LOG << L"CConnectionMonitorWindow::OnInitDialog : settings.ini‚Ì“Ç‚Ýž‚Ý‚ÉŽ¸”s";
			pt.clear();
		}
		CRect rcWindow;
		rcWindow.top = pt.get("ConnectionMonitorWindow.top", 0);
		rcWindow.left = pt.get("ConnectionMonitorWindow.left", 0);
		rcWindow.right = pt.get("ConnectionMonitorWindow.right", 0);
		rcWindow.bottom = pt.get("ConnectionMonitorWindow.bottom", 0);
		if (rcWindow != CRect()) {
			MoveWindow(&rcWindow);
		}
	}

	return TRUE;
}

void CConnectionMonitorWindow::OnDestroy()
{
	CConnectionManager::UnregisterCallback();

	using namespace boost::property_tree;

	std::string settingsPath = CT2A(Misc::GetExeDirectory() + _T("settings.ini"));
	ptree pt;
	try {
		read_ini(settingsPath, pt);
	}
	catch (...) {
		ERROR_LOG << L"CConnectionMonitorWindow::OnDestroy : settings.ini‚Ì“Ç‚Ýž‚Ý‚ÉŽ¸”s";
		pt.clear();
	}

	CRect rcWindow;
	GetWindowRect(&rcWindow);

	pt.put("ConnectionMonitorWindow.top", rcWindow.top);
	pt.put("ConnectionMonitorWindow.left", rcWindow.left);
	pt.put("ConnectionMonitorWindow.right", rcWindow.right);
	pt.put("ConnectionMonitorWindow.bottom", rcWindow.bottom);

	write_ini(settingsPath, pt);
}

void CConnectionMonitorWindow::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	CConnectionManager::UnregisterCallback();
	__super::ShowWindow(FALSE);
}


void CConnectionMonitorWindow::OnTimer(UINT_PTR nIDEvent)
{
	KillTimer(nIDEvent);

	int* itemData = (int*)nIDEvent;
	int count = m_connectionListView.GetItemCount();
	for (int i = 0; i < count; ++i) {
		if (m_connectionListView.GetItemData(i) == (DWORD_PTR)nIDEvent) {
			m_connectionListView.DeleteItem(i);
			break;
		}
	}

	CCritSecLock lock(m_csConnectionCloseList);
	for (auto it = m_connectionCloseList.begin(); it != m_connectionCloseList.end(); ++it) {
		if (itemData == &*it) {
			m_connectionCloseList.erase(it);
			break;
		}
	}
}

LRESULT CConnectionMonitorWindow::OnConnectionListRClick(LPNMHDR pnmh)
{
	auto lpnmitem = (LPNMITEMACTIVATE)pnmh;
	CString url;
	m_connectionListView.GetItemText(lpnmitem->iItem, 3, url);
	Misc::SetClipboardText(url);
	return 0;
}

DWORD CConnectionMonitorWindow::OnPrePaint(int nID, LPNMCUSTOMDRAW lpnmcd)
{
	if (lpnmcd->hdr.idFrom == IDC_LIST_CONNECTION)
		return CDRF_NOTIFYITEMDRAW;
	else
		return CDRF_DODEFAULT;
}

DWORD CConnectionMonitorWindow::OnItemPrePaint(int nID, LPNMCUSTOMDRAW lpnmcd)
{
	if (lpnmcd->hdr.idFrom == IDC_LIST_CONNECTION){
		LPNMLVCUSTOMDRAW lpnmlv = (LPNMLVCUSTOMDRAW)lpnmcd;
		int* itemData = (int*)m_connectionListView.GetItemData((int)lpnmcd->dwItemSpec);
		CCritSecLock lock(m_csConnectionCloseList);
		for (int& dlItem : m_connectionCloseList) {
			if (itemData == &dlItem) {
				lpnmlv->clrText = RGB(255, 255, 255);
				lpnmlv->clrTextBk = RGB(0xDF, 0x33, 0x4E);
				break;
			}
		}
	}
	return CDRF_DODEFAULT;
}
















