/**
*	@file	MainDlg.cpp
*	@brief	メインフレーム
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
#include "MainDlg.h"
#include <fstream>
#include <boost\property_tree\ptree.hpp>
#include <boost\property_tree\ini_parser.hpp>
#include "AboutDlg.h"
#include "OptionDialog.h"
#include "Misc.h"
#include "Proxy.h"
#include "UITranslator.h"
using namespace UITranslator;

using namespace boost::property_tree;

CMainDlg::CMainDlg(CProxy* proxy) : m_proxy(proxy), m_listChangeWatcher(FILE_NOTIFY_CHANGE_LAST_WRITE), m_bVisibleOnDestroy(true)
{
}

// ILogTrace
void CMainDlg::ProxyEvent(LogProxyEvent Event, const IPv4Address& addr)
{
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_STATIC_ACTIVEREQUESTCOUNT, CLog::GetActiveRequestCount());
}

void CMainDlg::FilterEvent(LogFilterEvent Event, int RequestNumber, const std::string& title, const std::string& text)
{
	if (Event == kLogFilterLogCommand) {
		if (text.length() > 0 && text[0] == '!') {
			if (m_logView.IsWindowVisible() == FALSE) {
				m_logView.ShowWindow();
				m_logView.FilterEvent(Event, RequestNumber, title, text);
			}
		}
	}
}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	ChangeControlTextForTranslateMessage(m_hWnd, IDC_BUTTON_SHOWFILTERMANAGE);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_BUTTON_SHOWLOGWINDOW);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_BUTTON_SHOWOPTION);
	ChangeControlTextForTranslateMessage(m_hWnd, ID_APP_ABOUT);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_GROUPBOX_ACTIVEFILTER);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_CHECKBOX_WEBPAGE);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_CHECKBOX_OUTHEADER);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_CHECKBOX_INHEADER);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_CHECKBOX_USEREMOTEPROXY);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_CHECK_BYPASS);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_BUTTON_ABORT);	
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_STATIC_ACTIVEREQUESTCOUNT, 0);


	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	UIAddChildWindowContainer(m_hWnd);

	CLog::RegisterLogTrace(this);

	m_listChangeWatcher.SetCallbackFunction([](const CString& filePath) {
		enum { kMaxRetry = 30, kSleepTime = 100 };
		for (int i = 0; i < kMaxRetry; ++i) {
			HANDLE hTestOpen = CreateFile(filePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hTestOpen == INVALID_HANDLE_VALUE) {
				::Sleep(kSleepTime);
				continue;
			}
			CloseHandle(hTestOpen);
			break;
		}
		CSettings::LoadList(filePath);
	});

	m_listChangeWatcher.WatchDirectory(Misc::GetExeDirectory() + _T("lists\\"));

	DoDataExchange(DDX_LOAD);

	{	// トレイアイコンを作成
		NOTIFYICONDATA	nid = { sizeof(NOTIFYICONDATA) };
		nid.uFlags	= NIF_ICON | NIF_MESSAGE | NIF_TIP;
		nid.hWnd	= m_hWnd;
		nid.hIcon	= hIconSmall;
		nid.uID		= kTrayIconId;
		nid.uCallbackMessage	= WM_TRAYICONNOTIFY;
		::_tcscpy_s(nid.szTip, APP_NAME);
		::Shell_NotifyIcon(NIM_ADD, &nid);
	}

	std::ifstream fs(Misc::GetExeDirectory() + _T("settings.ini"));
	if (fs) {
		ptree pt;
		read_ini(fs, pt);
		int	top  = pt.get("MainWindow.top", -1);
		int left = pt.get("MainWindow.left", -1);
		if (top != -1 && left != -1) {
			SetWindowPos(NULL, left, top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		}
		if (pt.get("MainWindow.ShowWindow", true))
			ShowWindow(TRUE);
	} else {
		ShowWindow(TRUE);
		// center the dialog on the screen
		CenterWindow();
	}

	m_logView.Create(m_hWnd);

	return TRUE;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	CLog::RemoveLogTrace(this);

	m_listChangeWatcher.StopWatchDirectory();

	DoDataExchange(DDX_SAVE);

	if (m_filterManagerWindow.IsWindow())
		m_filterManagerWindow.DestroyWindow();

	{	// トレイアイコンを削除
		NOTIFYICONDATA	nid = { sizeof(NOTIFYICONDATA) };
		nid.hWnd	= m_hWnd;
		nid.uID		= kTrayIconId;
		::Shell_NotifyIcon(NIM_DELETE, &nid);
	}

	_SaveMainDlgWindowPos();

	return 0;
}

void	CMainDlg::_SaveMainDlgWindowPos()
{
	std::string settingsPath = CT2A(Misc::GetExeDirectory() + _T("settings.ini"));
	ptree pt;
	try {
		read_ini(settingsPath, pt);
	} catch (...) {

	}

	pt.put("MainWindow.ShowWindow", m_bVisibleOnDestroy);
	if (m_bVisibleOnDestroy) {
		RECT rc;
		GetWindowRect(&rc);
		pt.put("MainWindow.top", rc.top);
		pt.put("MainWindow.left", rc.left);
	}

	write_ini(settingsPath, pt);
}

void	CMainDlg::OnEndSession(BOOL bEnding, UINT uLogOff)
{
	if (bEnding) {
		m_bVisibleOnDestroy	= IsWindowVisible() != 0;
		_SaveMainDlgWindowPos();
	}
}

LRESULT CMainDlg::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}


LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CloseDialog(wID);
	return 0;
}

void CMainDlg::OnSize(UINT nType, CSize size)
{
	if (nType == SIZE_MINIMIZED) {
		ShowWindow(FALSE);
	}
}

LRESULT CMainDlg::OnTrayIconNotify(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	if (lParam == WM_LBUTTONDOWN) {
		ShowWindow(TRUE);
		SetForegroundWindow(m_hWnd);
	} else if (lParam == WM_RBUTTONUP) {
		enum {
			kProxydomoOpen		= ID_TASKTRAYMENUBEGIN,
			kWebPageFilter		= ID_TASKTRAYMENUBEGIN + 1,
			kOutHeaderFilter	= ID_TASKTRAYMENUBEGIN + 2,
			kInHeaderFileter	= ID_TASKTRAYMENUBEGIN + 3,
			kBypass				= ID_TASKTRAYMENUBEGIN + 4,
			kUseRemoteProxy		= ID_TASKTRAYMENUBEGIN + 5,
			kExit				= ID_TASKTRAYMENUBEGIN + 6,

			kBlockListNone = ID_TASKTRAYMENUBEGIN + 100,
			kBlockListEdit = ID_TASKTRAYMENUBEGIN + 101,
			kBlockListBegin = ID_TASKTRAYMENUBEGIN + 110,
			kBlockListEnd = kBlockListBegin + 500,
		};
		
		CMenu menu;
		menu.CreatePopupMenu();
		menu.AppendMenu(MF_STRING, kProxydomoOpen, GetTranslateMessage(kProxydomoOpen).c_str());
		menu.SetMenuDefaultItem(kProxydomoOpen);
		menu.AppendMenu(MF_SEPARATOR);

		menu.AppendMenu(CSettings::s_filterText ? MF_CHECKED : MF_STRING, kWebPageFilter, 
			GetTranslateMessage(kWebPageFilter).c_str());
		menu.AppendMenu(CSettings::s_filterOut ? MF_CHECKED : MF_STRING, kOutHeaderFilter, 
			GetTranslateMessage(kOutHeaderFilter).c_str());
		menu.AppendMenu(CSettings::s_filterIn ? MF_CHECKED : MF_STRING, kInHeaderFileter, 
			GetTranslateMessage(kInHeaderFileter).c_str());
		menu.AppendMenu(CSettings::s_bypass ? MF_CHECKED : MF_STRING, kBypass, 
			GetTranslateMessage(kBypass).c_str());
		menu.AppendMenu(MF_SEPARATOR);
		menu.AppendMenu(CSettings::s_useRemoteProxy ? MF_CHECKED : MF_STRING, kUseRemoteProxy, 
			GetTranslateMessage(kUseRemoteProxy).c_str());
		menu.AppendMenu(MF_SEPARATOR);

		CMenuHandle menuBlockList;
		menuBlockList.CreatePopupMenu();
		std::vector<std::wstring> vecBlockListName;
		{
			std::lock_guard<std::recursive_mutex> lock(CSettings::s_mutexHashedLists);
			for (auto& list : CSettings::s_mapHashedLists) {
				std::wstring name = (LPWSTR)CA2W((list.first + ".txt").c_str());
				vecBlockListName.emplace_back(name);
			}
		}
		std::sort(vecBlockListName.begin(), vecBlockListName.end());
		const size_t count = vecBlockListName.size();
		for (size_t i = 0; i < count; ++i) {
			CString listName = vecBlockListName[i].c_str();
			CString filePath = Misc::GetExeDirectory() + _T("lists\\") + (LPCTSTR)listName;
			BOOL bExist = ::PathFileExists(filePath);
			menuBlockList.AppendMenu(bExist ? MF_STRING : MF_GRAYED, kBlockListBegin + i, listName);
		}
		if (count == 0) {
			menuBlockList.AppendMenu(MF_STRING | MF_GRAYED, (UINT_PTR)0, GetTranslateMessage(kBlockListNone).c_str());
		}
		menu.AppendMenu(MF_POPUP, menuBlockList, GetTranslateMessage(kBlockListEdit).c_str());
		menu.AppendMenu(MF_SEPARATOR);

		menu.AppendMenu(MF_STRING, kExit, GetTranslateMessage(kExit).c_str());
		CPoint pt;
		::GetCursorPos(&pt);
		::SetForegroundWindow(m_hWnd);
		BOOL bRet = menu.TrackPopupMenu(TPM_RETURNCMD, pt.x, pt.y, m_hWnd, NULL);
		switch (bRet) {
		case kProxydomoOpen:
			ShowWindow(TRUE);
			SetForegroundWindow(m_hWnd);
			break;

		case kWebPageFilter:
			CSettings::s_filterText = !CSettings::s_filterText;
			DoDataExchange(DDX_LOAD, IDC_CHECKBOX_WEBPAGE);
			break;

		case kOutHeaderFilter:
			CSettings::s_filterOut = !CSettings::s_filterOut;
			DoDataExchange(DDX_LOAD, IDC_CHECKBOX_OUTHEADER);
			break;

		case kInHeaderFileter:
			CSettings::s_filterIn = !CSettings::s_filterIn;
			DoDataExchange(DDX_LOAD, IDC_CHECKBOX_INHEADER);
			break;

		case kBypass:
			CSettings::s_bypass = !CSettings::s_bypass;
			DoDataExchange(DDX_LOAD, IDC_CHECK_BYPASS);
			break;

		case kUseRemoteProxy:
			CSettings::s_useRemoteProxy = !CSettings::s_useRemoteProxy;
			DoDataExchange(DDX_LOAD, IDC_CHECKBOX_USEREMOTEPROXY);
			break;

		case kExit:
			CloseDialog(0);
			break;

		default:
			if (kBlockListBegin <= bRet && bRet <= kBlockListEnd) {
				size_t index = bRet - kBlockListBegin;
				CString filePath = Misc::GetExeDirectory() + _T("lists\\") + vecBlockListName[index].c_str();
				::ShellExecute(NULL, NULL, filePath, NULL, NULL, SW_NORMAL);
			}
			break;
		}
	}
	return 0;
}


LRESULT CMainDlg::OnShowLogWindow(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	m_logView.ShowWindow();
	return 0;
}

LRESULT CMainDlg::OnShowFilterManageWindow(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (m_filterManagerWindow.IsWindow() == FALSE)
		m_filterManagerWindow.Create(m_hWnd);
	m_filterManagerWindow.ShowWindow(TRUE);
	return 0;
}

LRESULT CMainDlg::OnShowOption(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	COptionDialog optionDialog;
	optionDialog.DoModal(m_hWnd);
	return 0;
}

LRESULT CMainDlg::OnFilterButtonCheck(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	DoDataExchange(DDX_SAVE, wID);
	return 0;
}

LRESULT CMainDlg::OnAbort(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	m_proxy->AbortAllConnection();
	return 0;
}


void CMainDlg::CloseDialog(int nVal)
{
	if (CLog::GetActiveRequestCount() > 0) {
		int ret = MessageBox(
			GetTranslateMessage(ID_MAINDLGCLOSECONFIRMMESSAGE).c_str(),
			GetTranslateMessage(ID_TRANS_CONFIRM).c_str(), MB_ICONWARNING | MB_OKCANCEL | MB_DEFBUTTON2);
		if (ret == IDCANCEL)
			return ;
	}
	m_bVisibleOnDestroy	= IsWindowVisible() != 0;
	DestroyWindow();
	::PostQuitMessage(0);
}















