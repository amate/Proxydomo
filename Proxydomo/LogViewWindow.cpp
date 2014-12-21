/**
*	@file	LogViewWindow.cpp
*	@brief	ログ表示ウィンドウクラス
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
#include "LogViewWindow.h"
#include <fstream>
#include <algorithm>
#include <boost\property_tree\ini_parser.hpp>
#include <boost\property_tree\ptree.hpp>
#include "Misc.h"
#include "Settings.h"
#include "CodeConvert.h"

using namespace CodeConvert;

#define  LOG_COLOR_BACKGROUND  RGB(255, 255, 255)
#define  LOG_COLOR_DEFAULT     RGB(140, 140, 140)
#define  LOG_COLOR_FILTER      RGB(140, 140, 140)
#define  LOG_COLOR_REQUEST     RGB(240, 100,   0)
#define  LOG_COLOR_RESPONSE    RGB(  0, 150,   0)
#define  LOG_COLOR_PROXY       RGB(  0,   0,   0)


CLogViewWindow::CLogViewWindow() :
	m_bStopLog(false),
	m_bRecentURLs(false),
	m_bBrowserToProxy(false),
	m_bProxyToWeb(true),
	m_bProxyFromWeb(false),
	m_bBrowserFromProxy(true),
	
	m_bProxyEvent(true),
	m_bFilterEvent(true),
	m_bWebFilterDebug(false)
{
}


CLogViewWindow::~CLogViewWindow()
{
}


void	CLogViewWindow::ShowWindow()
{
	if (IsWindowVisible() == FALSE) {
		CLog::RegisterLogTrace(this);
		_RefreshTitle();
	}
	__super::ShowWindow(TRUE);
}


// ILogTrace

void CLogViewWindow::ProxyEvent(LogProxyEvent Event, const IPv4Address& addr)
{
	CString msg;
	msg.Format(_T(">>> ポート %d : "), addr.GetPortNumber());
	switch (Event) {
	case kLogProxyNewRequest:
		msg	+= _T("新しいリクエストを受信しました\n");
		{
			CCritSecLock	lock(m_csActiveRequestLog);
			m_vecActiveRequestLog.emplace_back(new EventLog(addr.GetPortNumber(), msg, LOG_COLOR_PROXY));
		}
		break;

	case kLogProxyEndRequest:
		msg += _T("リクエストが完了しました\n");
		{
			CCritSecLock	lock(m_csActiveRequestLog);
			uint16_t port = addr.GetPortNumber();
			auto result = std::remove_if(m_vecActiveRequestLog.begin(), m_vecActiveRequestLog.end(), 
				[port](const std::unique_ptr<EventLog>& log) { return log->port == port; }
			);
			m_vecActiveRequestLog.erase(result, m_vecActiveRequestLog.end());
		}
		break;

	default:
		ATLASSERT( FALSE );
		return ;
	}

	CString title;
	title.Format(_T("ログ - Active[ %d ]"), CLog::GetActiveRequestCount());
	SetWindowText(title);

	if (m_bProxyEvent == false)
		return;

	_AppendText(msg, LOG_COLOR_PROXY);
}

void CLogViewWindow::HttpEvent(LogHttpEvent Event, const IPv4Address& addr, int RequestNumber, const std::string& text)
{
	CString msg;
	msg.Format(_T(">>> ポート %d #%d : "), addr.GetPortNumber(), RequestNumber);
	switch (Event) {
	case kLogHttpRecvOut:	if (!m_bBrowserToProxy)	return ;
		msg += _T("ブラウザ → Proxy(this)");
		break;

	case kLogHttpSendOut:	if (!m_bProxyToWeb)	return ;
		msg += _T("Proxy(this) → サイト");
		break;

	case kLogHttpRecvIn:	if (!m_bProxyFromWeb)	return ;
		msg += _T("Proxy(this) ← サイト");
		break;

	case kLogHttpSendIn:	if (!m_bBrowserFromProxy)	return ;
		msg += _T("ブラウザ ← Proxy(this)");
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
	
	{
		CCritSecLock	lock(m_csActiveRequestLog);
		m_vecActiveRequestLog.emplace_back(new EventLog(addr.GetPortNumber(), msg, color));
	}
	{
		CCritSecLock	lock(m_csRequestLog);
		bool bFound = false;
		for (auto& reqLog : m_vecRquestLog) {
			if (reqLog->RequestNumber == RequestNumber) {
				reqLog->vecLog.emplace_back(new TextLog(msg, color));
				lock.Unlock();
				int nCurSel = m_cmbRequest.GetCurSel();
				if (nCurSel > 0 && m_cmbRequest.GetItemData(nCurSel) == RequestNumber) {
					_AppendRequestLogText(msg, color);
				}
				bFound = true;
				break;
			}
		}
		if (bFound == false) {
			m_vecRquestLog.emplace_back(new RequestLog(RequestNumber));
			m_vecRquestLog.back()->vecLog.emplace_back(new TextLog(msg, color));
			lock.Unlock();
			CString temp;
			temp.Format(_T("#%d"), RequestNumber);
			int nSel = m_cmbRequest.AddString(temp);
			m_cmbRequest.SetItemData(nSel, RequestNumber);
		}
	}
	_AppendText(msg, color);
}

void CLogViewWindow::FilterEvent(LogFilterEvent Event, int RequestNumber, const std::string& title, const std::string& text)
{
	if (m_bFilterEvent == false)
		return ;

	CString msg;
	COLORREF color = LOG_COLOR_FILTER;

	auto funcPartLog = [&]() {
		CCritSecLock	lock(m_csRequestLog);
		bool bFound = false;
		for (auto& reqLog : m_vecRquestLog) {
			if (reqLog->RequestNumber == RequestNumber) {
				reqLog->vecLog.emplace_back(new TextLog(msg, color));
				lock.Unlock();
				int nCurSel = m_cmbRequest.GetCurSel();
				if (nCurSel > 0 && m_cmbRequest.GetItemData(nCurSel) == RequestNumber) {
					_AppendRequestLogText(msg, color);
				}
				bFound = true;
				break;
			}
		}
		if (bFound == false) {
			m_vecRquestLog.emplace_back(new RequestLog(RequestNumber));
			m_vecRquestLog.back()->vecLog.emplace_back(new TextLog(msg, color));
			lock.Unlock();
			CString temp;
			temp.Format(_T("#%d"), RequestNumber);
			int nSel = m_cmbRequest.AddString(temp);
			m_cmbRequest.SetItemData(nSel, RequestNumber);
		}
	};
	
	msg.Format(_T("#%d : "), RequestNumber);
	switch (Event) {
	case kLogFilterHeaderMatch:
		msg	+= _T("HeaderMatch");
		break;

	case kLogFilterHeaderReplace:
		return ;
		msg += _T("HeaderReplace");
		break;

	case kLogFilterTextMatch:
		msg += _T("TextMatch");
		break;

	case kLogFilterTextReplace:
		return ;
		msg += _T("TextReplace");
		break;

	case kLogFilterLogCommand:
	{
		std::string log = text;
		if (log.length() > 0) {
			if (text[0] == '!') {
				ShowWindow();
				log.erase(log.begin());
			}
		}
		if (log.length() > 0) {
			switch (log[0]) {
			case 'R': color = RGB(238, 0, 0); log.erase(log.begin()); break;
			case 'W': color = RGB(0, 0, 0); log.erase(log.begin()); break;
			case 'w': color = RGB(170, 170, 170); log.erase(log.begin()); break;
			case 'B': color = RGB(0, 0, 221); log.erase(log.begin()); break;
			case 'G': color = RGB(0, 170, 0); log.erase(log.begin()); break;
			case 'Y': color = RGB(216, 216, 0); log.erase(log.begin()); break;
			case 'V': color = RGB(136, 0, 136); log.erase(log.begin()); break;
			case 'C': color = RGB(0, 255, 255); log.erase(log.begin()); break;
			}
		}
		msg += UTF16fromUTF8(log).c_str();
		msg += _T("\n");
		funcPartLog();
		_AppendText(msg, color);
		return;
	}
		break;

	case kLogFilterJump:
		msg += _T("JumpTo: ");
		msg += text.c_str();
		msg += _T("\n");
		funcPartLog();
		_AppendText(msg, LOG_COLOR_FILTER);
		return ;
		break;

	case kLogFilterRdir:
		msg += _T("RedirectTo: ");
		msg += text.c_str();
		msg += _T("\n");
		funcPartLog();
		_AppendText(msg, LOG_COLOR_FILTER);
		return ;
		break;

	case kLogFilterListReload:
		msg = _T("リストが再読み込みされました: ");
		msg += UTF16fromUTF8(title).c_str();
		msg += _T("\n");
		_AppendText(msg, LOG_COLOR_FILTER);
		return ;
		break;

	case kLogFilterListBadLine:
		msg.Format(_T("リストのマッチングルールに異常があります: [%s] %d 行目\n"), UTF16fromUTF8(title).c_str(), RequestNumber);
		_AppendText(msg, LOG_COLOR_FILTER);
		return;
		break;

	case kLogFilterListMatch:
		msg.Format(_T("#%d : ListMatch [%s] %s 行目\n"), RequestNumber, UTF16fromUTF8(title).c_str(), UTF16fromUTF8(text).c_str());
		funcPartLog();
		_AppendText(msg, LOG_COLOR_FILTER);		
		return;
		break;

	default:
		ATLASSERT( FALSE );
		return ;
	}
	msg.AppendFormat(_T(" [%s] "), UTF16fromUTF8(title).c_str());
	//if (Event == kLogFilterHeaderMatch)
	//	msg += text.c_str();
	msg += _T("\n");

	funcPartLog();
	
	_AppendText(msg, color);
}

void	CLogViewWindow::_AddNewRequest(CLog::RecentURLData* it)
{
	m_listRequest.InsertItem(0, std::to_wstring(it->requestNumber).c_str());
	CString temp = it->responseCode.c_str();
	LVITEM lvi = { 0 };
	lvi.mask	= LVIF_TEXT;
	lvi.iSubItem	= 1;
	lvi.pszText		= temp.GetBuffer();
	m_listRequest.SetItem(&lvi);

	temp = it->contentType.c_str();
	lvi.iSubItem	= 2;
	lvi.pszText		= temp.GetBuffer();
	m_listRequest.SetItem(&lvi);

	temp = it->contentLength.c_str();
	lvi.iSubItem	= 3;
	lvi.pszText		= temp.GetBuffer();
	m_listRequest.SetItem(&lvi);

	temp = it->url.c_str();
	lvi.iSubItem	= 4;
	lvi.pszText		= temp.GetBuffer();
	m_listRequest.SetItem(&lvi);
}

void CLogViewWindow::AddNewRequest(long requestNumber)
{
	_AddNewRequest(&CLog::s_deqRecentURLs.front());
			
	while (m_listRequest.GetItemCount() > CLog::kMaxRecentURLCount)
		m_listRequest.DeleteItem(m_listRequest.GetItemCount() - 1);
}


BOOL CLogViewWindow::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	m_editLog = GetDlgItem(IDC_RICHEDIT_LOG);
	m_editLog.SetBackgroundColor(LOG_COLOR_BACKGROUND);
	m_editLog.SetTargetDevice(NULL, 0);

	m_editPartLog = GetDlgItem(IDC_RICHEDIT_PARTLOG);
	m_editPartLog.SetBackgroundColor(LOG_COLOR_BACKGROUND);
	m_editPartLog.SetTargetDevice(NULL, 0);
	m_editPartLog.ShowWindow(FALSE);

	m_cmbRequest = GetDlgItem(IDC_COMBO_REQUEST);
	m_cmbRequest.AddString(_T("すべてのログ"));
	m_cmbRequest.SetCurSel(0);

	m_listRequest.SubclassWindow(GetDlgItem(IDC_LIST_RECENTURLS));
	m_listRequest.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
	m_listRequest.AddColumn(_T("Con"), 0, 
							-1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
	m_listRequest.SetColumnSortType(0, LVCOLSORT_LONG);
	m_listRequest.AddColumn(_T("Code"), 1);
	m_listRequest.SetColumnSortType(1, LVCOLSORT_LONG);
	m_listRequest.AddColumn(_T("Content-Type"), 2);
	m_listRequest.AddColumn(_T("Length"), 3, 
							-1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
	m_listRequest.SetColumnSortType(3, LVCOLSORT_LONG);
	m_listRequest.AddColumn(_T("URL"), 4);
	m_listRequest.SetColumnWidth(4, 400);

    // ダイアログリサイズ初期化
    DlgResize_Init(true, true, WS_THICKFRAME | WS_MAXIMIZEBOX);

	using namespace boost::property_tree;
	
	CString settingsPath = Misc::GetExeDirectory() + _T("settings.ini");
	std::ifstream fs(settingsPath);
	if (fs) {
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
		if (pt.get("LogWindow.ShowWindow", false))
			ShowWindow();

		if (auto value = pt.get_optional<bool>("LogWindow.BrowserToProxy"))
			m_bBrowserToProxy	= value.get();
		if (auto value = pt.get_optional<bool>("LogWindow.ProxyToWeb"))
			m_bProxyToWeb	= value.get();
		if (auto value = pt.get_optional<bool>("LogWindow.ProxyFromWeb"))
			m_bProxyFromWeb	= value.get();
		if (auto value = pt.get_optional<bool>("LogWindow.BrowserFromProxy"))
			m_bBrowserFromProxy	= value.get();
		if (auto value = pt.get_optional<bool>("LogWindow.ProxyEvent"))
			m_bProxyEvent	= value.get();
		if (auto value = pt.get_optional<bool>("LogWindow.FilterEvent"))
			m_bFilterEvent	= value.get();
	}

	DoDataExchange(DDX_LOAD);

	return 0;
}

void CLogViewWindow::OnDestroy()
{
	CLog::RemoveLogTrace(this);

	using namespace boost::property_tree;
	
	std::string settingsPath = CT2A(Misc::GetExeDirectory() + _T("settings.ini"));
	ptree pt;
	try {
		read_ini(settingsPath, pt);
	} catch (...) {
	}

	CRect rcWindow;
	GetWindowRect(&rcWindow);
		
	pt.put("LogWindow.top", rcWindow.top);
	pt.put("LogWindow.left", rcWindow.left);
	pt.put("LogWindow.right", rcWindow.right);
	pt.put("LogWindow.bottom", rcWindow.bottom);
	bool bVisible = IsWindowVisible() != 0;
	pt.put("LogWindow.ShowWindow", bVisible);

	pt.put("LogWindow.BrowserToProxy"	, m_bBrowserToProxy);
	pt.put("LogWindow.ProxyToWeb"		, m_bProxyToWeb);
	pt.put("LogWindow.ProxyFromWeb"		, m_bProxyFromWeb);
	pt.put("LogWindow.BrowserFromProxy"	, m_bBrowserFromProxy);
	pt.put("LogWindow.ProxyEvent"		, m_bProxyEvent);
	pt.put("LogWindow.FilterEvent"		, m_bFilterEvent);

	write_ini(settingsPath, pt);
}

void CLogViewWindow::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	CLog::RemoveLogTrace(this);

	{
		CCritSecLock	lock(m_csActiveRequestLog);
		m_vecActiveRequestLog.clear();
	}

	CSettings::s_WebFilterDebug = false;

	__super::ShowWindow(FALSE);
}

void CLogViewWindow::OnClear(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	if (m_bRecentURLs) {
		{
			CCritSecLock	lock(CLog::s_csdeqRecentURLs);
			CLog::s_deqRecentURLs.clear();
		}
		m_listRequest.DeleteAllItems();

	} else {
		m_editLog.SetWindowText(_T(""));
		_RefreshTitle();
		m_editLog.Invalidate();

		m_editPartLog.SetWindowText(_T(""));

		m_vecRquestLog.clear();

		m_cmbRequest.ResetContent();
		m_cmbRequest.AddString(_T("すべてのログ"));
		m_cmbRequest.SetCurSel(0);

		m_editPartLog.ShowWindow(FALSE);
		m_editLog.ShowWindow(TRUE);
	}
}

void CLogViewWindow::OnComboRequestSelChange(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	int nCurSel = m_cmbRequest.GetCurSel();
	if (nCurSel == 0) {
		m_editPartLog.SetWindowText(_T(""));
		m_editPartLog.ShowWindow(FALSE);
		m_editLog.ShowWindow(TRUE);
	} else {
		m_editPartLog.SetWindowText(_T(""));
		if (m_editPartLog.IsWindowVisible() == FALSE) {
			m_editPartLog.ShowWindow(TRUE);
			m_editLog.ShowWindow(FALSE);
		}
		int RequestNumber = (int)m_cmbRequest.GetItemData(nCurSel);
		CCritSecLock	lock(m_csRequestLog);
		for (auto& reqLog : m_vecRquestLog) {
			if (reqLog->RequestNumber == RequestNumber) {
				for (auto& log : reqLog->vecLog)
					_AppendRequestLogText(log->text, log->textColor);
				m_editPartLog.PostMessage(WM_VSCROLL, SB_TOP);
				break;
			}
		}
	}


}

void CLogViewWindow::OnShowActiveRequestLog(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	bool bPrev = m_bRecentURLs;
	m_bRecentURLs = false;
	OnClear(0, 0, NULL);
	m_bRecentURLs = bPrev;

	_RefreshTitle();

	CCritSecLock	lock(m_csActiveRequestLog);
	for (auto& log : m_vecActiveRequestLog) {
		_AppendText(log->text, log->textColor);
	}

}

/// URLをクリップボードにコピー
LRESULT CLogViewWindow::OnRecentURLListRClick(LPNMHDR pnmh)
{
	auto lpnmitem = (LPNMITEMACTIVATE) pnmh;
	CString url;
	m_listRequest.GetItemText(lpnmitem->iItem, 4, url);
	Misc::SetClipboardText(url);

	return 0;
}

/// URLを開く
LRESULT CLogViewWindow::OnRecentURLListDblClick(LPNMHDR pnmh)
{
	auto lpnmitem = (LPNMITEMACTIVATE) pnmh;
	CString url;
	m_listRequest.GetItemText(lpnmitem->iItem, 4, url);
	::ShellExecute(NULL, NULL, url, NULL, NULL, SW_NORMAL);
	return 0;
}


void CLogViewWindow::OnCheckBoxChanged(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	DoDataExchange(DDX_SAVE, nID);
	if (nID == IDC_CHECKBOX_WEBFILTERDEBUG) {
		CSettings::s_WebFilterDebug	= m_bWebFilterDebug;

	} else if (nID == IDC_CHECKBOX_STOPLOG) {
		if (m_bStopLog)
			CLog::RemoveLogTrace(this);
		else
			CLog::RegisterLogTrace(this);

	} else if (nID == IDC_CHECKBOX_RECENTURLS) {
		GetDlgItem(IDC_LIST_RECENTURLS).ShowWindow(m_bRecentURLs);
		if (!m_bRecentURLs) {
			m_editLog.ShowWindow(!m_bRecentURLs);
			m_cmbRequest.SetCurSel(0);
		} else {
			m_editLog.ShowWindow(!m_bRecentURLs);
			m_editPartLog.ShowWindow(!m_bRecentURLs);
		}
		if (m_bRecentURLs) {
			m_listRequest.DeleteAllItems();
			CCritSecLock	lock(CLog::s_csdeqRecentURLs);
			for (auto it = CLog::s_deqRecentURLs.rbegin(); it != CLog::s_deqRecentURLs.rend(); ++it) {
				_AddNewRequest(&(*it));
			}
		}
	}
}

void	CLogViewWindow::_AppendText(const CString& text, COLORREF textColor)
{
	if (m_bRecentURLs)
		return ;

	CCritSecLock lock(m_csLog);
	m_logList.emplace_back(text, textColor);
	PostMessage(WM_APPENDTEXT);
}

LRESULT CLogViewWindow::OnAppendText(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CCritSecLock lock(m_csLog);
	std::list<TextLog> logList;
	logList.swap(m_logList);
	lock.Unlock();

	for (auto& log : logList){
		const CString& text = log.text;
		COLORREF textColor = log.textColor;

		CHARFORMAT cfmt = { sizeof(CHARFORMAT) };
		cfmt.dwMask = CFM_COLOR;
		cfmt.crTextColor = textColor;

		m_editLog.HideSelection(TRUE);

		m_editLog.SetSel(-1, -1);
		long start = 0;
		long end = 0;
		m_editLog.GetSel(start, end);

		m_editLog.AppendText(text);

		m_editLog.SetSel(end, -1);
		m_editLog.SetSelectionCharFormat(cfmt);
		m_editLog.SetSel(-1, -1);

		m_editLog.HideSelection(FALSE);

		m_editLog.PostMessage(WM_VSCROLL, SB_BOTTOM);
	}
	return 0;
}

void	CLogViewWindow::_AppendRequestLogText(const CString& text, COLORREF textColor)
{
	CHARFORMAT cfmt = { sizeof(CHARFORMAT) };
	cfmt.dwMask = CFM_COLOR;
	cfmt.crTextColor = textColor;

	m_editPartLog.HideSelection(TRUE);

	m_editPartLog.SetSel(-1, -1);
	long start = 0;
	long end = 0;
	m_editPartLog.GetSel(start, end);

	m_editPartLog.AppendText(text);

	m_editPartLog.SetSel(end, -1);
	m_editPartLog.SetSelectionCharFormat(cfmt);
	m_editPartLog.SetSel(-1, -1);

	m_editPartLog.HideSelection(FALSE);

	m_editPartLog.PostMessage(WM_VSCROLL, SB_BOTTOM);
}


void	CLogViewWindow::_RefreshTitle()
{
	CString title;
	title.Format(_T("ログ - Active[ %d ]"), CLog::GetActiveRequestCount());
	SetWindowText(title);
}





