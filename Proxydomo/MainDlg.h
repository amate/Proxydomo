// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////
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

#include <atlchecked.h>
#include <atlddx.h>
#include "LogViewWindow.h"
#include "FilterManageWindow.h"
#include "Settings.h"
#include "DirectoryWatcher.h"

class CMainDlg : 
	public CDialogImpl<CMainDlg>, 
	public CUpdateUI<CMainDlg>,
	public CMessageFilter, 
	public CIdleHandler,
	public CWinDataExchange<CMainDlg>,
	public ILogTrace
{
public:
	enum { IDD = IDD_MAINDLG };

	enum {
		kTrayIconId = 1,
		WM_TRAYICONNOTIFY	= WM_APP + 1,
	};

	CMainDlg();

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		return CWindow::IsDialogMessage(pMsg);
	}

	virtual BOOL OnIdle()
	{
		return FALSE;
	}

	// ILogTrace
	virtual void ProxyEvent(LogProxyEvent Event, const IPv4Address& addr) override;
	virtual void HttpEvent(LogHttpEvent Event, const IPv4Address& addr, int RequestNumber, const std::string& text) override { };
	virtual void FilterEvent(LogFilterEvent Event, int RequestNumber, const std::string& title, const std::string& text) override;

	BEGIN_UPDATE_UI_MAP(CMainDlg)
	END_UPDATE_UI_MAP()

	BEGIN_DDX_MAP( CMainDlg )
		DDX_CHECK(IDC_CHECKBOX_WEBPAGE	,	CSettings::s_filterText)
		DDX_CHECK(IDC_CHECKBOX_OUTHEADER,	CSettings::s_filterOut)
		DDX_CHECK(IDC_CHECKBOX_INHEADER,	CSettings::s_filterIn)
	END_DDX_MAP()

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MSG_WM_ENDSESSION( OnEndSession	)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		MSG_WM_SIZE( OnSize )
		MESSAGE_HANDLER( WM_TRAYICONNOTIFY, OnTrayIconNotify )

		COMMAND_ID_HANDLER(IDC_BUTTON_SHOWLOGWINDOW, OnShowLogWindow )
		COMMAND_ID_HANDLER(IDC_BUTTON_SHOWFILTERMANAGE, OnShowFilterManageWindow )
		COMMAND_ID_HANDLER(IDC_BUTTON_SHOWOPTION, OnShowOption)
		COMMAND_ID_HANDLER( IDC_CHECKBOX_WEBPAGE	, OnFilterButtonCheck )
		COMMAND_ID_HANDLER( IDC_CHECKBOX_OUTHEADER	, OnFilterButtonCheck )
		COMMAND_ID_HANDLER( IDC_CHECKBOX_INHEADER	, OnFilterButtonCheck )
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	void	OnEndSession(BOOL bEnding, UINT uLogOff);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	void OnSize(UINT nType, CSize size);
	LRESULT OnTrayIconNotify(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT OnShowLogWindow(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnShowFilterManageWindow(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnShowOption(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFilterButtonCheck(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void CloseDialog(int nVal);

private:
	void	_SaveMainDlgWindowPos();

	// Data members
	CLogViewWindow	m_logView;
	CFilterManageWindow	m_filterManagerWindow;
	bool		m_bVisibleOnDestroy;
	CDirectoryWatcher	m_listChangeWatcher;
};
