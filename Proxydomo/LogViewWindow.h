/**
*	@file	LogViewWindow.h
*	@brief	ログ表示クラス
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

#include "resource.h"
#include <vector>
#include <list>
#include <memory>
#include <atlcrack.h>
#include <atlctrls.h>
#include <atlframe.h>
#include <atlsync.h>
#include <atlddx.h>
#include <atlctrlx.h>
#include "Log.h"

class CLogViewWindow : 
	public CDialogImpl<CLogViewWindow>, 
	public CDialogResize<CLogViewWindow>,
	public CWinDataExchange<CLogViewWindow>,
	public ILogTrace
{
public:
	enum { IDD = IDD_LOGVIEW };

	enum { WM_APPENDTEXT = WM_APP + 100 };

	CLogViewWindow();
	~CLogViewWindow();

	void	ShowWindow();

	// ILogTrace
	virtual void ProxyEvent(LogProxyEvent Event, const IPv4Address& addr) override;
	virtual void HttpEvent(LogHttpEvent Event, const IPv4Address& addr, int RequestNumber, const std::string& text) override;
	virtual void FilterEvent(LogFilterEvent Event, int RequestNumber, const std::string& title, const std::string& text) override;
	virtual void AddNewRequest(long requestNumber) override;

	BEGIN_DLGRESIZE_MAP( CLogViewWindow )
		DLGRESIZE_CONTROL(IDC_RICHEDIT_LOG, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_RICHEDIT_PARTLOG, DLSZ_SIZE_X | DLSZ_SIZE_Y)		
		DLGRESIZE_CONTROL( IDC_LIST_RECENTURLS, DLSZ_SIZE_X | DLSZ_SIZE_Y )

		DLGRESIZE_CONTROL( IDC_CHECKBOX_STOPLOG, DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL( IDC_CHECKBOX_RECENTURLS, DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL( IDC_BUTTON_SHOWACTIVEREQUESTLOG, DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL( IDC_BUTTON_CLEAR, DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL( IDC_COMBO_REQUEST, DLSZ_MOVE_Y )

		DLGRESIZE_CONTROL( IDC_CHECKBOX_BROWSERTOPROXY, DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL( IDC_CHECKBOX_PROXYTOWEB, DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL( IDC_CHECKBOX_PROXYFROMWEB, DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL( IDC_CHECKBOX_BROWSERFROMPROXY, DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL( IDC_GROUP_HTTPEVENT, DLSZ_MOVE_Y )

		DLGRESIZE_CONTROL( IDC_CHECKBOX_PROXYEVENT, DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL( IDC_CHECKBOX_FILTEREVENT, DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL( IDC_CHECKBOX_WEBFILTERDEBUG, DLSZ_MOVE_Y )
	END_DLGRESIZE_MAP()

	BEGIN_DDX_MAP( CLogViewWindow )
		DDX_CHECK(IDC_CHECKBOX_STOPLOG		, m_bStopLog	)
		DDX_CHECK(IDC_CHECKBOX_RECENTURLS	, m_bRecentURLs	)

		DDX_CHECK(IDC_CHECKBOX_BROWSERTOPROXY	,	m_bBrowserToProxy)
		DDX_CHECK(IDC_CHECKBOX_PROXYTOWEB		,	m_bProxyToWeb)
		DDX_CHECK(IDC_CHECKBOX_PROXYFROMWEB		,	m_bProxyFromWeb)
		DDX_CHECK(IDC_CHECKBOX_BROWSERFROMPROXY	,	m_bBrowserFromProxy)

		DDX_CHECK(IDC_CHECKBOX_PROXYEVENT	,	m_bProxyEvent)
		DDX_CHECK(IDC_CHECKBOX_FILTEREVENT	,	m_bFilterEvent)
		DDX_CHECK(IDC_CHECKBOX_WEBFILTERDEBUG,	m_bWebFilterDebug)

	END_DDX_MAP()

	BEGIN_MSG_MAP_EX( CLogViewWindow )
		MSG_WM_INITDIALOG( OnInitDialog )
		MSG_WM_DESTROY( OnDestroy )

		COMMAND_ID_HANDLER_EX( IDCANCEL, OnCancel )
		COMMAND_ID_HANDLER_EX( IDC_BUTTON_CLEAR, OnClear )
		COMMAND_ID_HANDLER_EX( IDC_BUTTON_SHOWACTIVEREQUESTLOG, OnShowActiveRequestLog )

		COMMAND_HANDLER_EX( IDC_COMBO_REQUEST, CBN_SELCHANGE, OnComboRequestSelChange )

		NOTIFY_HANDLER_EX( IDC_LIST_RECENTURLS, NM_RCLICK , OnRecentURLListRClick )
		NOTIFY_HANDLER_EX( IDC_LIST_RECENTURLS, NM_DBLCLK , OnRecentURLListDblClick )

		COMMAND_ID_HANDLER_EX( IDC_CHECKBOX_STOPLOG			, OnCheckBoxChanged )
		COMMAND_ID_HANDLER_EX( IDC_CHECKBOX_STOPSCROLL		, OnCheckBoxChanged)
		COMMAND_ID_HANDLER_EX( IDC_CHECKBOX_RECENTURLS		, OnCheckBoxChanged)
		COMMAND_ID_HANDLER_EX( IDC_CHECKBOX_BROWSERTOPROXY	, OnCheckBoxChanged )
		COMMAND_ID_HANDLER_EX( IDC_CHECKBOX_PROXYTOWEB		, OnCheckBoxChanged )	
		COMMAND_ID_HANDLER_EX( IDC_CHECKBOX_PROXYFROMWEB	, OnCheckBoxChanged )	
		COMMAND_ID_HANDLER_EX( IDC_CHECKBOX_BROWSERFROMPROXY, OnCheckBoxChanged )	
		COMMAND_ID_HANDLER_EX( IDC_CHECKBOX_PROXYEVENT		, OnCheckBoxChanged )	
		COMMAND_ID_HANDLER_EX( IDC_CHECKBOX_FILTEREVENT		, OnCheckBoxChanged )	
		COMMAND_ID_HANDLER_EX( IDC_CHECKBOX_WEBFILTERDEBUG	, OnCheckBoxChanged )	

		MESSAGE_HANDLER_EX(WM_APPENDTEXT, OnAppendText)
		CHAIN_MSG_MAP( CDialogResize<CLogViewWindow> )
	END_MSG_MAP()

	// void OnCommandIDHandlerEX(UINT uNotifyCode, int nID, CWindow wndCtl)

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnDestroy();

	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnClear(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnShowActiveRequestLog(UINT uNotifyCode, int nID, CWindow wndCtl);

	void OnComboRequestSelChange(UINT uNotifyCode, int nID, CWindow wndCtl);
	LRESULT OnRecentURLListRClick(LPNMHDR pnmh);
	LRESULT OnRecentURLListDblClick(LPNMHDR pnmh);

	void OnCheckBoxChanged(UINT uNotifyCode, int nID, CWindow wndCtl);

	LRESULT OnAppendText(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	void	_AppendText(const CString& text, COLORREF textColor);
	void	_AppendRequestLogText(const CString& text, COLORREF textColor);
	void	_RefreshTitle();
	void	_AddNewRequest(CLog::RecentURLData* it);

	// Data members
	CRichEditCtrl	m_editLog;
	CRichEditCtrl	m_editPartLog;
	CSortListViewCtrl	m_listRequest;
	CComboBox		m_cmbRequest;

	struct EventLog {
		uint16_t	port;
		CString		text;
		COLORREF	textColor;

		EventLog(uint16_t p, const CString& t, COLORREF tc) : port(p), text(t), textColor(tc) { }
	};
	std::vector<std::unique_ptr<EventLog>>	m_vecActiveRequestLog;
	CCriticalSection	m_csActiveRequestLog;

	struct TextLog {
		CString		text;
		COLORREF	textColor;
		TextLog(const CString& t, COLORREF c) : text(t), textColor(c) { }
	};

	struct RequestLog {
		int			RequestNumber;
		std::vector<std::unique_ptr<TextLog>>	vecLog;

		RequestLog(int rq) : RequestNumber(rq) { }
	};
	std::vector<std::unique_ptr<RequestLog>>	m_vecRquestLog;
	CCriticalSection	m_csRequestLog;

	CCriticalSection	m_csLog;
	std::list<TextLog>	m_logList;

	bool	m_bStopLog;
	bool	m_bRecentURLs;

	bool	m_bBrowserToProxy;
	bool	m_bProxyToWeb;
	bool	m_bProxyFromWeb;
	bool	m_bBrowserFromProxy;

	bool	m_bProxyEvent;
	bool	m_bFilterEvent;
	bool	m_bWebFilterDebug;
};

