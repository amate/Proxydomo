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

using namespace boost::property_tree;

CMainDlg::CMainDlg() : m_listChangeWatcher(FILE_NOTIFY_CHANGE_LAST_WRITE), m_bVisibleOnDestroy(true)
{
}

// ILogTrace
void CMainDlg::ProxyEvent(LogProxyEvent Event, const IPv4Address& addr)
{
	CString text;
	text.Format(_T("アクティブな接続数: %02d"), CLog::GetActiveRequestCount());
	GetDlgItem(IDC_STATIC_ACTIVEREQUESTCOUNT).SetWindowText(text);
}


LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

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
		CMenu menu;
		menu.CreatePopupMenu();
		menu.AppendMenu(MF_STRING, 1U, _T("終了"));
		CPoint pt;
		::GetCursorPos(&pt);
		::SetForegroundWindow(m_hWnd);
		BOOL bRet = menu.TrackPopupMenu(TPM_RETURNCMD, pt.x, pt.y, m_hWnd, NULL);
		if (bRet == 1) {
			CloseDialog(0);
		}
	}
	return 0;
}


LRESULT CMainDlg::OnShowLogWindow(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (m_logView.IsWindow() == FALSE)
		m_logView.Create(m_hWnd);
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


void CMainDlg::CloseDialog(int nVal)
{
	if (CLog::GetActiveRequestCount() > 0) {
		int ret = MessageBox(_T("まだ接続中のリクエストがあります。\r\n終了してもいいですか？"), _T("確認 - ") APP_NAME, MB_ICONWARNING | MB_OKCANCEL | MB_DEFBUTTON2);
		if (ret == IDCANCEL)
			return ;
	}
	m_bVisibleOnDestroy	= IsWindowVisible() != 0;
	DestroyWindow();
	::PostQuitMessage(0);
}















