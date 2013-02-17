/**
*	@file	LogViewWindow.h
*	@brief	ログ表示クラス
*/

#pragma once

#include "resource.h"
#include <vector>
#include <memory>
#include <atlcrack.h>
#include <atlctrls.h>
#include <atlframe.h>
#include <atlsync.h>
#include "Log.h"

class CLogViewWindow : 
	public CDialogImpl<CLogViewWindow>, 
	public CDialogResize<CLogViewWindow>,
	public ILogTrace
{
public:
	enum { IDD = IDD_LOGVIEW };

	CLogViewWindow();
	~CLogViewWindow();

	void	ShowWindow();

	// ILogTrace
	virtual void ProxyEvent(LogProxyEvent Event, const IPv4Address& addr) override;
	virtual void HttpEvent(LogHttpEvent Event, const IPv4Address& addr, int RequestNumber, const std::string& text) override;

	BEGIN_DLGRESIZE_MAP( CLogViewWindow )
		DLGRESIZE_CONTROL( IDC_RICHEDIT_LOG, DLSZ_SIZE_X | DLSZ_SIZE_Y )
		DLGRESIZE_CONTROL( IDC_BUTTON_CLEAR, DLSZ_MOVE_X | DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL( IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL( IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL( IDC_BUTTON_SHOWACTIVEREQUESTLOG, DLSZ_MOVE_X | DLSZ_MOVE_Y )
	END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP_EX( CLogViewWindow )
		MSG_WM_INITDIALOG( OnInitDialog )
		COMMAND_ID_HANDLER_EX( IDCANCEL, OnCancel )
		COMMAND_ID_HANDLER_EX( IDC_BUTTON_CLEAR, OnClear )
		COMMAND_ID_HANDLER_EX( IDC_BUTTON_SHOWACTIVEREQUESTLOG, OnShowActiveRequestLog )
		CHAIN_MSG_MAP( CDialogResize<CLogViewWindow> )
	END_MSG_MAP()

	// void OnCommandIDHandlerEX(UINT uNotifyCode, int nID, CWindow wndCtl)

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnClear(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnShowActiveRequestLog(UINT uNotifyCode, int nID, CWindow wndCtl);

private:
	void	_AppendText(const CString& text, COLORREF textColor);

	// Data members
	CRichEditCtrl	m_editLog;
	CCriticalSection	m_csLog;
	struct EventLog {
		uint16_t	port;
		CString		text;
		COLORREF	textColor;

		EventLog(uint16_t p, const CString& t, COLORREF tc) : port(p), text(t), textColor(tc) { }
	};
	std::vector<std::unique_ptr<EventLog>>	m_vecActiveRequestLog;
	CCriticalSection	m_csActiveRequestLog;
};

