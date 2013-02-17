/**
*	@file	LogViewWindow.cpp
*	@brief	ログ表示ウィンドウクラス
*/

#include "stdafx.h"
#include "LogViewWindow.h"
#include <fstream>
#include <boost\property_tree\ini_parser.hpp>
#include <boost\property_tree\ptree.hpp>
#include "Misc.h"

#define  LOG_COLOR_BACKGROUND  RGB(255, 255, 255)
#define  LOG_COLOR_DEFAULT     RGB(140, 140, 140)
#define  LOG_COLOR_FILTER      RGB(140, 140, 140)
#define  LOG_COLOR_REQUEST     RGB(240, 100,   0)
#define  LOG_COLOR_RESPONSE    RGB(  0, 150,   0)
#define  LOG_COLOR_PROXY       RGB(  0,   0,   0)


CLogViewWindow::CLogViewWindow()
{
}


CLogViewWindow::~CLogViewWindow()
{
}


// ILogTrace

void CLogViewWindow::ProxyEvent(LogProxyEvent Event, const IPv4Address& addr)
{
	CString msg;
	msg.Format(_T(">>> ポート %d : "), addr.GetPort());
	switch (Event) {
	case kLogProxyNewRequest:
		msg	+= _T("新しいリクエストを受信しました");
		break;

	case kLogProxyEndRequest:
		msg += _T("リクエストが完了しました");
		break;

	default:
		ATLASSERT( FALSE );
		return ;
	}
	msg += _T("\n");

	CString title;
	title.Format(_T("ログ - Active[ %d ]"), CLog::GetActiveRequestCount());
	SetWindowText(title);

	_AppendText(msg, LOG_COLOR_PROXY);
}

void CLogViewWindow::HttpEvent(LogHttpEvent Event, int RequestNumber, const std::string& text)
{
	CString msg;
	msg.Format(_T(">>> #%d : "), RequestNumber);
	switch (Event) {
	case kLogHttpRecvOut:
		msg += _T("ブラウザ ⇒ Proxy(this)");
		break;

	case kLogHttpSendOut:
		msg += _T("Proxy(this) ⇒ サイト");
		break;

	case kLogHttpRecvIn:
		msg += _T("サイト ⇒ Proxy(this)");
		break;

	case kLogHttpSendIn:
		msg += _T("Proxy(this) ⇒ ブラウザ");
		break;

	default:
		return ;
	}
	msg += _T("\n");
	msg += text.c_str();
	msg += _T("\n");

    // Colors depends on Outgoing or Incoming
	COLORREF color = LOG_COLOR_REQUEST;
	if (Event == kLogHttpRecvIn || Event == kLogHttpSendIn)
		color = LOG_COLOR_RESPONSE;
	
	_AppendText(msg, color);
}


BOOL CLogViewWindow::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	m_editLog = GetDlgItem(IDC_RICHEDIT_LOG);

	m_editLog.SetBackgroundColor(LOG_COLOR_BACKGROUND);

	CLog::RegisterLogTrace(this);

    // ダイアログリサイズ初期化
    DlgResize_Init(true, false, WS_THICKFRAME | WS_MAXIMIZEBOX | WS_CLIPCHILDREN);

	using namespace boost::property_tree;
	
	CString settingsPath = Misc::GetExeDirectory() + _T("settings.ini");
	std::ifstream fs(settingsPath);
	if (!fs)
		return 0;
	ptree pt;
	read_ini(fs, pt);
	CRect rcWindow;
	rcWindow.top	= pt.get("LogWindow.top", 0);
	rcWindow.left	= pt.get("LogWindow.left", 0);
	rcWindow.right	= pt.get("LogWindow.right", 0);
	rcWindow.bottom	= pt.get("LogWindow.bottom", 0);
	if (rcWindow != CRect()) {
		MoveWindow(&rcWindow);
	}

	return 0;
}

void CLogViewWindow::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	CLog::RemoveLogTrace();

	using namespace boost::property_tree;
	
	CString settingsPath = Misc::GetExeDirectory() + _T("settings.ini");
	std::ofstream fs(settingsPath);
	if (fs) {
		CRect rcWindow;
		GetWindowRect(&rcWindow);
		ptree pt;
		pt.add("LogWindow.top", rcWindow.top);
		pt.add("LogWindow.left", rcWindow.left);
		pt.add("LogWindow.right", rcWindow.right);
		pt.add("LogWindow.bottom", rcWindow.bottom);

		write_ini(fs, pt);
	}

	ShowWindow(FALSE);
}


void CLogViewWindow::OnClear(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	m_editLog.SetWindowText(_T(""));
}


void	CLogViewWindow::_AppendText(const CString& text, COLORREF textColor)
{
	CCritSecLock lock(m_csLog);

	CHARFORMAT cfmt = { sizeof(CHARFORMAT) };
	cfmt.dwMask = CFM_COLOR;
	cfmt.crTextColor	= textColor;
	
	m_editLog.SetSel(-1, -1);
	long start = 0;
	long end = 0;
	m_editLog.GetSel(start, end);

	m_editLog.AppendText(text);

	m_editLog.SetSel(end, -1);
	m_editLog.SetSelectionCharFormat(cfmt);
	m_editLog.SetSel(-1, -1);

	m_editLog.PostMessage(WM_VSCROLL, SB_BOTTOM);
}


