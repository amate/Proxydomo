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
#include <Shlwapi.h>
#include "Misc.h"
#include "Settings.h"
#include "Logger.h"
#include "proximodo\util.h"
#include "CodeConvert.h"
using namespace CodeConvert;
#include "UITranslator.h"
using namespace UITranslator;

COLORREF  LOG_COLOR_BACKGROUND	= RGB(255, 255, 255);
COLORREF  LOG_COLOR_DEFAULT		= RGB(140, 140, 140);
COLORREF  LOG_COLOR_FILTER		= RGB(140, 140, 140);
COLORREF  LOG_COLOR_REQUEST		= RGB(240, 100,   0);
COLORREF  LOG_COLOR_RESPONSE	= RGB(0	 , 150,   0);
COLORREF  LOG_COLOR_PROXY		= RGB(0  ,   0,   0);


CLogViewWindow::CLogViewWindow() :
	m_bStopLog(false),
	m_bRecentURLs(false),
	m_bBrowserToProxy(false),
	m_bProxyToWeb(true),
	m_bProxyFromWeb(false),
	m_bBrowserFromProxy(true),
	
	m_bProxyEvent(true),
	m_bFilterEvent(true),
	m_bViewPostData(false),
	m_bWebFilterDebug(false)
{
}


CLogViewWindow::~CLogViewWindow()
{
}


void	CLogViewWindow::ShowWindow()
{
	if (IsWindowVisible() == FALSE) {
		if (m_bStopLog == false) {
			CLog::RegisterLogTrace(this);
		}
		_RefreshTitle();
	}
	__super::ShowWindow(TRUE);
}


// ILogTrace

void CLogViewWindow::ProxyEvent(LogProxyEvent Event, const IPv4Address& addr)
{
	CString msg = GetTranslateMessage(ID_PROXYEVENTHEADER, addr.GetPortNumber()).c_str();
	switch (Event) {
	case kLogProxyNewRequest:
		msg += GetTranslateMessage(ID_PROXYNEWREQUEST).c_str();
		break;

	case kLogProxyEndRequest:
		msg += GetTranslateMessage(ID_PROXYENDREQUEST).c_str();
		break;

	default:
		ATLASSERT( FALSE );
		return ;
	}

	SetWindowText(GetTranslateMessage(IDD_LOGVIEW, CLog::GetActiveRequestCount()).c_str());

	if (m_bProxyEvent == false)
		return;

	_AppendText(msg, LOG_COLOR_PROXY);
}

void CLogViewWindow::HttpEvent(LogHttpEvent Event, const IPv4Address& addr, int RequestNumber, const std::string& text)
{
	CString msg = GetTranslateMessage(ID_HTTPEVENTHEADER, addr.GetPortNumber(), RequestNumber).c_str();
	switch (Event) {
	case kLogHttpNewRequest:
		{
			CString url = text.c_str();
			CString temp;
			temp.Format(_T("#%d %s"), RequestNumber, (LPCWSTR)url);
			int nSel = m_cmbRequest.AddString(temp);
			m_cmbRequest.SetItemData(nSel, RequestNumber);

			CCritSecLock	lock(m_csRequestLog);
			auto it = std::find_if(m_vecRquestLog.begin(), m_vecRquestLog.end(),
				[RequestNumber](const std::unique_ptr<RequestLog>& requestLog) { return requestLog->RequestNumber == RequestNumber; });
			if (it == m_vecRquestLog.end()) {
				m_vecRquestLog.emplace_back(new RequestLog(RequestNumber));
			}
			return;
		}
		break;
		
	case kLogHttpRecvOut:	if (!m_bBrowserToProxy)	return ;
		msg += GetTranslateMessage(ID_HTTPRECVOUT).c_str();
		break;

	case kLogHttpSendOut:	if (!m_bProxyToWeb)	return ;
		msg += GetTranslateMessage(ID_HTTPSENDOUT).c_str();
		break;

	case kLogHttpRecvIn:	if (!m_bProxyFromWeb)	return ;
		msg += GetTranslateMessage(ID_HTTPRECVIN).c_str();
		break;

	case kLogHttpSendIn:	if (!m_bBrowserFromProxy)	return ;
		msg += GetTranslateMessage(ID_HTTPSENDIN).c_str();
		break;

	case kLogHttpPostOut:	if (!m_bViewPostData)	return ;
		msg += L"PostData";
		break;

	default:
		return ;
	}
	msg += _T("\n");
	msg += UTF16fromUTF8(text).c_str();
	msg += _T("\n");

    // Colors depends on Outgoing or Incoming
	COLORREF color = LOG_COLOR_REQUEST;
	if (Event == kLogHttpRecvIn || Event == kLogHttpSendIn) {
		color = LOG_COLOR_RESPONSE;
	} else if (Event == kLogHttpPostOut) {
		color = LOG_COLOR_DEFAULT;

		std::string unescText = CUtil::UESC(text);
		std::string charaCode = DetectCharaCode(unescText);
		if (charaCode.length()) {
			UErrorCode err = UErrorCode::U_ZERO_ERROR;
			auto pConverter = ucnv_open(charaCode.c_str(), &err);
			if (pConverter) {
				std::wstring utf16PostData = UTF16fromConverter(unescText, pConverter);
				msg.AppendFormat(_T(">> Decode Data [%s]\n%s\n"), UTF16fromUTF8(charaCode).c_str(), utf16PostData.c_str());
				ucnv_close(pConverter);
			}
		}
		msg += _T("\n");
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
		msg += GetTranslateMessage(ID_FILTERHEADERMATCH).c_str();
		break;

	case kLogFilterHeaderReplace:
		return ;
		msg += GetTranslateMessage(ID_FILTERHEADERREPLACE).c_str();
		break;

	case kLogFilterTextMatch:
		msg += GetTranslateMessage(ID_FILTERTEXTMATCH).c_str();
		break;

	case kLogFilterTextReplace:
		return ;
		msg += GetTranslateMessage(ID_FILTERTEXTRAPLCE).c_str();
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
		msg += GetTranslateMessage(ID_FILTERJUMPTO, UTF16fromUTF8(text)).c_str();
		funcPartLog();
		_AppendText(msg, LOG_COLOR_FILTER);
		return ;
		break;

	case kLogFilterRdir:
		msg += GetTranslateMessage(ID_FILTERRDIR, UTF16fromUTF8(text)).c_str();
		funcPartLog();
		_AppendText(msg, LOG_COLOR_FILTER);
		return ;
		break;

	case kLogFilterListReload:
		msg = GetTranslateMessage(ID_FILTERLISTRELOAD, UTF16fromUTF8(title), RequestNumber).c_str();
		_AppendText(msg, LOG_COLOR_FILTER);
		return ;
		break;

	case kLogFilterListBadLine:
		msg = GetTranslateMessage(ID_FILTERLISTBADLINE, UTF16fromUTF8(title), RequestNumber).c_str();
		_AppendText(msg, LOG_COLOR_FILTER);
		return;
		break;

	case kLogFilterListMatch:
		msg = GetTranslateMessage(ID_FILTERLISTMATCH, RequestNumber, UTF16fromUTF8(title), UTF16fromUTF8(text)).c_str();
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

	//temp = it->contentLength.c_str();
	if (it->contentLength.length() > 0) {
		WCHAR tempSize[64] = L"";
		::StrFormatByteSizeW(std::stoll(it->contentLength), tempSize, 64);
		lvi.iSubItem = 3;
		lvi.pszText = tempSize;
		m_listRequest.SetItem(&lvi);
	}

	temp = it->url.c_str();
	lvi.iSubItem	= 4;
	lvi.pszText		= temp.GetBuffer();
	m_listRequest.SetItem(&lvi);

	if (it->kill) {
		m_listRequest.SetItemData(0, 1);
	}
}

void CLogViewWindow::AddNewRequest(long requestNumber)
{
	_AddNewRequest(&CLog::s_deqRecentURLs.front());
			
	while (m_listRequest.GetItemCount() > CLog::kMaxRecentURLCount)
		m_listRequest.DeleteItem(m_listRequest.GetItemCount() - 1);
}


BOOL CLogViewWindow::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	SetWindowText(GetTranslateMessage(IDD_LOGVIEW, 0).c_str());

	ChangeControlTextForTranslateMessage(m_hWnd, IDC_CHECKBOX_STOPLOG);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_CHECKBOX_RECENTURLS);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_BUTTON_CLEAR);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_BUTTON_VIEWCONNECTIONMONITOR);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_GROUP_HTTPEVENT);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_CHECKBOX_BROWSERTOPROXY);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_CHECKBOX_PROXYTOWEB);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_CHECKBOX_PROXYFROMWEB);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_CHECKBOX_BROWSERFROMPROXY);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_CHECKBOX_PROXYEVENT);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_CHECKBOX_FILTEREVENT);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_CHECKBOX_VIEWPOSTDATA);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_CHECKBOX_WEBFILTERDEBUG);

	m_editLog = GetDlgItem(IDC_RICHEDIT_LOG);
	m_editLog.SetTargetDevice(NULL, 0);

	m_editPartLog = GetDlgItem(IDC_RICHEDIT_PARTLOG);
	m_editPartLog.SetTargetDevice(NULL, 0);
	m_editPartLog.ShowWindow(FALSE);

	m_cmbRequest = GetDlgItem(IDC_COMBO_REQUEST);
	m_cmbRequest.AddString(GetTranslateMessage(ID_ALLLOG).c_str());
	m_cmbRequest.SetCurSel(0);

	if (auto font = UITranslator::getFont()) {
		CLogFont lf;
		font.GetLogFont(&lf);
		lf.SetHeight(8);
		CFont smallfont = lf.CreateFontIndirect();
		m_editLog.SetFont(smallfont);
		m_editPartLog.SetFont(smallfont);
		m_cmbRequest.SetFont(font);
	}

	m_listRequest.SubclassWindow(GetDlgItem(IDC_LIST_RECENTURLS));
	m_listRequest.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER);
	m_listRequest.AddColumn(_T("Con"), 0, 
							-1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
	m_listRequest.SetColumnWidth(0, 50);
	m_listRequest.SetColumnSortType(0, LVCOLSORT_LONG);
	m_listRequest.AddColumn(_T("Code"), 1);
	m_listRequest.SetColumnSortType(1, LVCOLSORT_LONG);
	m_listRequest.AddColumn(_T("Content-Type"), 2);
	m_listRequest.AddColumn(_T("Length"), 3, 
							-1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
	m_listRequest.SetColumnSortType(3, LVCOLSORT_LONG);
	m_listRequest.SetColumnWidth(3, 80);
	m_listRequest.AddColumn(_T("URL"), 4);
	m_listRequest.SetColumnWidth(4, 400);

	m_connectionMonitor.Create(GetParent());

    // ダイアログリサイズ初期化
    DlgResize_Init(true, true, WS_THICKFRAME | WS_MAXIMIZEBOX);

	using namespace boost::property_tree;
	
	CString settingsPath = Misc::GetExeDirectory() + _T("settings.ini");
	std::ifstream fs(settingsPath);
	if (fs) {
		ptree pt;
		try {
			read_ini(fs, pt);
		}
		catch (...) {
			ERROR_LOG << L"CLogViewWindow::OnInitDialog : settings.iniの読み込みに失敗";
			pt.clear();
		}
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
		if (auto value = pt.get_optional<bool>("LogWindow.ViewPostData"))
			m_bViewPostData = value.get();

		auto funcConvertRGB = [](const std::string& colorString, COLORREF& color) {
			if (colorString.length() != 6)
				return;
				
			std::string r = colorString.substr(0, 2);
			std::string g = colorString.substr(2, 2);
			std::string b = colorString.substr(4, 2);
			color = RGB(std::stoi(r, nullptr, 16), std::stoi(g, nullptr, 16), std::stoi(b, nullptr, 16));
		};
		if (auto value = pt.get_optional<std::string>("LogWindow.COLOR_BACKGROUND"))
			funcConvertRGB(value.get(), LOG_COLOR_BACKGROUND);
		if (auto value = pt.get_optional<std::string>("LogWindow.COLOR_DEFAULT"))
			funcConvertRGB(value.get(), LOG_COLOR_DEFAULT);
		if (auto value = pt.get_optional<std::string>("LogWindow.COLOR_FILTER"))
			funcConvertRGB(value.get(), LOG_COLOR_FILTER);
		if (auto value = pt.get_optional<std::string>("LogWindow.COLOR_REQUEST"))
			funcConvertRGB(value.get(), LOG_COLOR_REQUEST);
		if (auto value = pt.get_optional<std::string>("LogWindow.COLOR_RESPONSE"))
			funcConvertRGB(value.get(), LOG_COLOR_RESPONSE);
		if (auto value = pt.get_optional<std::string>("LogWindow.COLOR_PROXY"))
			funcConvertRGB(value.get(), LOG_COLOR_PROXY);
	}
	m_editLog.SetBackgroundColor(LOG_COLOR_BACKGROUND);
	m_editPartLog.SetBackgroundColor(LOG_COLOR_BACKGROUND);

	DoDataExchange(DDX_LOAD);

	return 0;
}

void CLogViewWindow::OnDestroy()
{
	CLog::RemoveLogTrace(this);

	if (m_connectionMonitor.IsWindow())
		m_connectionMonitor.DestroyWindow();

	using namespace boost::property_tree;
	
	std::string settingsPath = CT2A(Misc::GetExeDirectory() + _T("settings.ini"));
	ptree pt;
	try {
		read_ini(settingsPath, pt);
	} catch (...) {
		ERROR_LOG << L"CLogViewWindow::OnDestroy : settings.iniの読み込みに失敗";
		pt.clear();
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
	pt.put("LogWindow.ViewPostData"		, m_bViewPostData);

	write_ini(settingsPath, pt);

	PostQuitMessage(0);
}

// ShowWindowと対になる関数
void CLogViewWindow::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	if (m_bStopLog == false) {
		CLog::RemoveLogTrace(this);
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
		m_cmbRequest.AddString(GetTranslateMessage(ID_ALLLOG).c_str());
		m_cmbRequest.SetCurSel(0);

		m_editPartLog.ShowWindow(FALSE);
		m_editLog.ShowWindow(TRUE);
	}
}

void CLogViewWindow::OnViewConnectionMonitor(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	m_connectionMonitor.ShowWindow();
}

void CLogViewWindow::OnComboRequestSelChange(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	if (m_bRecentURLs)
		return;

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
		m_editPartLog.SetRedraw(FALSE);
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
		m_editPartLog.SetRedraw(TRUE);
		m_editPartLog.Invalidate();
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

LRESULT CLogViewWindow::OnRecentURLListRDblClick(LPNMHDR pnmh)
{
	auto lpnmitem = (LPNMITEMACTIVATE)pnmh;
	CString con;
	m_listRequest.GetItemText(lpnmitem->iItem, 0, con);
	ATLASSERT(con.GetLength());

	m_bRecentURLs = false;
	DoDataExchange(DDX_LOAD, IDC_CHECKBOX_RECENTURLS);

	GetDlgItem(IDC_LIST_RECENTURLS).ShowWindow(FALSE);

	m_editLog.ShowWindow(TRUE);
	m_editPartLog.ShowWindow(FALSE);

	int RequestNumber = std::stoi((LPCWSTR)con);
	CCritSecLock	lock(m_csRequestLog);
	int nIndex = 0;
	for (auto& reqLog : m_vecRquestLog) {
		++nIndex;
		if (reqLog->RequestNumber == RequestNumber) {
			m_cmbRequest.SetCurSel(nIndex);

			m_editPartLog.SetWindowText(_T(""));
			if (m_editPartLog.IsWindowVisible() == FALSE) {
				m_editPartLog.ShowWindow(TRUE);
				m_editLog.ShowWindow(FALSE);
			}

			for (auto& log : reqLog->vecLog)
				_AppendRequestLogText(log->text, log->textColor);
			m_editPartLog.PostMessage(WM_VSCROLL, SB_TOP);
			return 0;
		}
	}
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

DWORD CLogViewWindow::OnPrePaint(int nID, LPNMCUSTOMDRAW lpnmcd)
{
	if (lpnmcd->hdr.idFrom == IDC_LIST_RECENTURLS)
		return CDRF_NOTIFYITEMDRAW;
	else
		return CDRF_DODEFAULT;
}

DWORD CLogViewWindow::OnItemPrePaint(int nID, LPNMCUSTOMDRAW lpnmcd)
{
	if (lpnmcd->hdr.idFrom == IDC_LIST_RECENTURLS) {
		LPNMLVCUSTOMDRAW lpnmlv = (LPNMLVCUSTOMDRAW)lpnmcd;
		if (lpnmlv->nmcd.lItemlParam == 1) {
			int nSel = m_listRequest.GetSelectedIndex();
			if ((lpnmlv->nmcd.uItemState & (CDIS_SELECTED | CDIS_FOCUS)) == (CDIS_SELECTED | CDIS_FOCUS) 
				&& lpnmlv->nmcd.dwItemSpec == nSel) 
			{
				lpnmlv->clrText = RGB(255, 255, 255);
				lpnmlv->clrTextBk = RGB(0xEF, 0x85, 0x8C);	// #EF858C
			} else {
				lpnmlv->clrText = RGB(255, 255, 255);
				lpnmlv->clrTextBk = RGB(0xDF, 0x33, 0x4E);
			}
			lpnmlv->nmcd.uItemState &= ~CDIS_SELECTED;
			return CDRF_NEWFONT;
		}
	}
	return CDRF_DODEFAULT;
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
	SetWindowText(GetTranslateMessage(IDD_LOGVIEW, CLog::GetActiveRequestCount()).c_str());
}





