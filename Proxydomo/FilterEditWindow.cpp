/**
*	@file	FilterEditWindow.cpp
*	@brief	フィルター編集ウィンドウ
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
#include "FilterEditWindow.h"
#include <sstream>
#include <chrono>
#include <functional>
#include <boost\property_tree\ptree.hpp>
#include <boost\property_tree\ini_parser.hpp>
#include <atlmisc.h>
#include <atlsplit.h>
#include "FilterDescriptor.h"
#include "FilterOwner.h"
#include "proximodo\filter.h"
#include "proximodo\expander.h"
#include "Settings.h"
#include "Misc.h"
#include "Matcher.h"
#include "Log.h"
#include "CodeConvert.h"
using namespace CodeConvert;

#include "UITranslator.h"
using namespace UITranslator;

using namespace boost::property_tree;

CString MiscGetWindowText(HWND hWnd)
{
	int length = GetWindowTextLength(hWnd);
	if (length <= 0)
		return _T("");

	CString text;
	GetWindowText(hWnd, text.GetBuffer(length + 2), length + 1);
	text.ReleaseBuffer();
	return text;
}

//////////////////////////////////////////////////////////
// CFilterTestWindow

class CFilterTestWindow : public CDialogImpl<CFilterTestWindow>, public CDialogResize<CFilterTestWindow>
{
public:
	enum { IDD = IDD_FILTERTEST };

	enum { kMaxPrefCount = 1000 };

	CFilterTestWindow(CFilterDescriptor* pfd, std::function<bool ()> func) : 
		m_pFilter(pfd), m_funcSaveToTempFilter(func), m_bTestURLPattern(false) { }

	void	SetURLPatternMode(bool b)
	{
		m_bTestURLPattern = b;
		CString title = (GetTranslateMessage(IDD_FILTERTEST) + L" - ").c_str();
		title += m_bTestURLPattern ? GetTranslateMessage(ID_URLPATTERNTEST).c_str() : GetTranslateMessage(ID_MATCHPATTERNTEST).c_str();
		SetWindowText(title);
	}

	void OnFinalMessage(HWND /*hWnd*/) override { delete this; }

	BEGIN_DLGRESIZE_MAP( CFilterTestWindow )
		DLGRESIZE_CONTROL(IDC_STATIC_SPLITTER, DLSZ_SIZE_X | DLSZ_SIZE_Y)

	END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP_EX( CFilterTestWindow )
		MSG_WM_INITDIALOG( OnInitDialog )
		MSG_WM_DESTROY( OnDestroy )
		COMMAND_ID_HANDLER_EX( IDCANCEL, OnCancel )
		COMMAND_ID_HANDLER_EX( IDC_BUTTON_TEST, OnTestAnalyze )
		COMMAND_ID_HANDLER_EX( IDC_BUTTON_ANALYZE, OnTestAnalyze )
		COMMAND_ID_HANDLER_EX( IDC_BUTTON_CLEAR, OnClear )
		CHAIN_MSG_MAP( CDialogResize<CFilterTestWindow> )
	END_MSG_MAP()

	// void OnCommandIDHandlerEX(UINT uNotifyCode, int nID, CWindow wndCtl)

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
	{
		ChangeControlTextForTranslateMessage(m_hWnd, IDC_BUTTON_TEST);
		ChangeControlTextForTranslateMessage(m_hWnd, IDC_BUTTON_ANALYZE);
		ChangeControlTextForTranslateMessage(m_hWnd, IDC_BUTTON_CLEAR);

		HFONT hFont = GetDlgItem(IDC_STATIC_SPLITTER).GetFont();
		CRect rcSplitter;
		GetDlgItem(IDC_STATIC_SPLITTER).GetWindowRect(&rcSplitter);
		ScreenToClient(&rcSplitter);
		m_wndSplitter.Create(m_hWnd, rcSplitter, NULL, WS_CHILD | WS_VISIBLE);
		GetDlgItem(IDC_STATIC_SPLITTER).SetDlgCtrlID(0);
		m_wndSplitter.SetDlgCtrlID(IDC_STATIC_SPLITTER);
		m_wndSplitter.SetSplitterExtendedStyle(SPLIT_BOTTOMALIGNED);

		CLogFont lf;
		lf.SetMenuFont();
		m_editTest.Create(m_wndSplitter, NULL, NULL, 
			WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL | WS_HSCROLL | WS_BORDER);
		m_editTest.SetFont(hFont);
		m_editResult.Create(m_wndSplitter, NULL, NULL, 
			WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL | WS_HSCROLL | WS_BORDER);
		m_editResult.SetFont(hFont);
		m_wndSplitter.SetSplitterPanes(m_editTest, m_editResult);

		enum { kMaxEditLimitSize = 3 * 1024 * 1024 };
		m_editTest.LimitText(kMaxEditLimitSize);
		m_editResult.LimitText(kMaxEditLimitSize);

		m_editTest.SetWindowText(s_strLastTest);

		m_wndTest.SubclassWindow(m_editTest);
		m_wndResult.SubclassWindow(m_editResult);

		// ダイアログリサイズ初期化
		DlgResize_Init(true, true, WS_THICKFRAME | WS_CLIPCHILDREN | WS_MAXIMIZEBOX);

		std::string settingsPath = CT2A(Misc::GetExeDirectory() + _T("settings.ini"));
		ptree pt;
		try {
			read_ini(settingsPath, pt);
		} catch (...) {
		}
		CRect rcWindow;
		rcWindow.top	= pt.get("FilterTestWindow.top", 0);
		rcWindow.left	= pt.get("FilterTestWindow.left", 0);
		rcWindow.right	= pt.get("FilterTestWindow.right", 0);
		rcWindow.bottom	= pt.get("FilterTestWindow.bottom", 0);
		if (rcWindow != CRect())
			MoveWindow(&rcWindow);

		int splitterPos = pt.get("FilterTestWindow.SplitterPos", -1);
		m_wndSplitter.SetSplitterPos(splitterPos);

		return 0;
	}

	void OnDestroy()
	{
		s_strLastTest = MiscGetWindowText(m_editTest); 

		std::string settingsPath = CT2A(Misc::GetExeDirectory() + _T("settings.ini"));
		ptree pt;
		try {
			read_ini(settingsPath, pt);
		}
		catch (...) {
		}

		CRect rcWindow;
		GetWindowRect(&rcWindow);

		pt.put("FilterTestWindow.top", rcWindow.top);
		pt.put("FilterTestWindow.left", rcWindow.left);
		pt.put("FilterTestWindow.right", rcWindow.right);
		pt.put("FilterTestWindow.bottom", rcWindow.bottom);

		pt.put("FilterTestWindow.SplitterPos", m_wndSplitter.GetSplitterPos());

		write_ini(settingsPath, pt);
	}

	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
	{
		ShowWindow(FALSE);
	}

	void OnTestAnalyze(UINT uNotifyCode, int nID, CWindow wndCtl)
	{
		if (m_funcSaveToTempFilter() == false)
			return;

		m_pFilter->TestValidity();
        // ensure there is no parsing error in this filter
        if (m_pFilter->errorMsg.length()) {
			MessageBox(m_pFilter->errorMsg.c_str());
            return;
        }

		bool prof = (nID == IDC_BUTTON_ANALYZE);

        // create a fake text filter (would normally be a
        // resident CRequestManager + CTextBuffer + CTextFilter).
        // This instanciation is not profiled, because it the real
        // world this only happens when Proximodo starts up and
        // processes the first few pages.
        CFilterOwner owner;
		owner.SetInHeader(L"Host", L"www.host.org");
        std::wstring url = L"http://www.host.org/path/page.html?query=true#anchor";
        owner.url.parseUrl(url);
        //owner.cnxNumber = 1;
		owner.fileType	= "htm";
        CFilter filter(owner);
		Proxydomo::MatchData matchData(&filter);

        bool okayChars[256];
		Proxydomo::CMatcher& matcher = m_bTestURLPattern ? 
			*m_pFilter->spURLMatcher : *m_pFilter->spTextMatcher;
        matcher.mayMatch(okayChars);
        Proxydomo::CMatcher* boundsMatcher = NULL;
		if (m_pFilter->filterType == CFilterDescriptor::kFilterText && !m_pFilter->boundsPattern.empty()) {
			boundsMatcher = m_pFilter->spBoundsMatcher.get();
            bool tab[256];
            boundsMatcher->mayMatch(tab);
            for (int i = 0; i < 256; i++) 
				okayChars[i] = okayChars[i] && tab[i];
        }
		if (m_bTestURLPattern == false) {
			// special processing for <start> and <end> filters
			if (m_pFilter->matchPattern == L"<start>" || m_pFilter->matchPattern == L"<end>") {
				std::wstring str = CExpander::expand(m_pFilter->replacePattern, filter);
				filter.unlock();
				if (m_pFilter->matchPattern == L"<start>") {
					str += (MiscGetWindowText(m_editTest));
				} else {
					std::wstring temp = str;
					str = (MiscGetWindowText(m_editTest));
					str += temp;
				}
				m_editResult.SetWindowText((str.c_str()));
				return;
			}
		} else {
			std::wstring text = (LPCWSTR)MiscGetWindowText(m_editTest);
			auto brpos = text.find_first_of(L"\r\n");
			if (brpos != std::wstring::npos) 
				text = text.substr(0, brpos);
			
			CUrl	url;
			url.parseUrl(text);
			const char* end = nullptr;
			std::wstring urlFromHost = url.getFromHost();
			CString result = GetTranslateMessage(ID_TESTTEXT, urlFromHost.c_str()).c_str();
			result += L"\r\n\r\n";
			if (matcher.match(urlFromHost, &filter)) {
				m_editResult.SetWindowText(result + GetTranslateMessage(ID_MATCHSUCCEEDED).c_str());
			} else {
				m_editResult.SetWindowText(result + GetTranslateMessage(ID_MATCHFAILED).c_str());
			}
			return ;
		}

        std::wstringstream result;
		const std::wstring text = MiscGetWindowText(m_editTest);
		const size_t size = text.size();
        int run = 0;
        int numMatch = 0;
		auto starttime = std::chrono::high_resolution_clock::now();
            
        do {
            result.str(L"");
            owner.killed = false;
            filter.bypassed = false;

            // Test of a text filter.
            // We don't take URL pattern into account.
			if (m_pFilter->filterType == CFilterDescriptor::kFilterText) {

                bool found = false;
                const wchar_t* bufHead = text.c_str();
                const wchar_t* bufTail = bufHead + size;
                const wchar_t* index   = bufHead;
                const wchar_t* done    = bufHead;
                while (index < bufTail && !owner.killed && !filter.bypassed) {
                    if (okayChars[(unsigned char)(*index)] == false) {
                        ++index;
                        continue;
                    }
                    bool matched;
                    const wchar_t* end = nullptr;
                    const wchar_t* stop = index + m_pFilter->windowWidth;
                    if (stop > bufTail) 
						stop = bufTail;

                    filter.clearMemory();

                    if (boundsMatcher) {
                        matched = boundsMatcher->match(index, stop, end, &matchData);
                        filter.unlock();
                        if (!matched) {
                            ++index;
                            continue;
                        }
                        stop = end;
                    }
                    matched = matcher.match(index, stop, end, &matchData);
                    filter.unlock();
                    if (!matched || (boundsMatcher && end != stop)) {
                        ++index;
                        continue;
                    }
                    found = true;

					if (prof == false) {
						for (auto& matchListLog : matchData.matchListLog) {
							CLog::FilterEvent(kLogFilterListMatch, owner.requestNumber, matchListLog.first, std::to_string(matchListLog.second));
						}
					}

                    if (run == 0) numMatch++;
                    result << std::wstring(done, (size_t)(index - done));
                    result << CExpander::expand(m_pFilter->replacePattern, filter);
                    filter.unlock();
                    done = end;
                    if (end == index) ++index; else index = end;
                }
                if (!owner.killed) result << std::wstring(done, (size_t)(bufTail - done));
				if (!found) result.str(GetTranslateMessage(ID_TEST_NO_MATCH).c_str());

            // Test of a header filter. Much simpler...
            } else {

                const wchar_t *index, *stop, *end;
                index = text.c_str();
                stop  = index + text.size();
                if (matcher.match(index, stop, end, &matchData)) {
                    result << CExpander::expand(m_pFilter->replacePattern, filter);
                    if (run == 0) numMatch++;
                } else {
					result << GetTranslateMessage(ID_TEST_NO_MATCH);
                }
                filter.unlock();
            }
        // If profiling, repeat this loop 1000 times or 10 seconds,
        // whichever comes first.
		} while (prof && run++ < kMaxPrefCount &&
				 ((std::chrono::high_resolution_clock::now() - starttime) < std::chrono::seconds(5)) );

        if (prof) {
			int len = m_editTest.GetWindowTextLength();
			double totaltime = (double)std::chrono::duration_cast<std::chrono::milliseconds>
									(std::chrono::high_resolution_clock::now() - starttime).count();
			double avarage = totaltime / run;
			double busytime = 0.0;
			if (totaltime > 0)
				busytime = (((len * run) / 1000.0)/* byte -> Kbyte */ / (totaltime / 1000.0)/* milisec -> sec */);

			CString text = GetTranslateMessage(ID_ANALYZERESULT, len, numMatch, (int)avarage, busytime).c_str();
			m_editResult.SetWindowText(text);
        } else {
			m_editResult.SetWindowText((result.str().c_str()));
        }
	}

	void OnClear(UINT uNotifyCode, int nID, CWindow wndCtl)
	{
		m_editTest.SetWindowText(_T(""));
	}

private:
	// Data members
	CFilterDescriptor*	m_pFilter;
	std::function<bool ()>	m_funcSaveToTempFilter;

	CEditSelAllHelper	m_wndTest;
	CEditSelAllHelper	m_wndResult;

	CHorSplitterWindow	m_wndSplitter;
	CEdit	m_editTest;
	CEdit	m_editResult;
	static CString s_strLastTest;
	bool	m_bTestURLPattern;
};

CString CFilterTestWindow::s_strLastTest;


//////////////////////////////////////////////////////
// CFilterEditWindow

CFilterEditWindow::CFilterEditWindow(CFilterDescriptor* pfd) : 
	m_pFilter(pfd), 
	m_pTempFilter(new CFilterDescriptor(*pfd)),
	m_pTestWindow(new CFilterTestWindow(m_pTempFilter, std::bind(&CFilterEditWindow::_SaveToTempFilter, this)))
{
	ATLASSERT( m_pFilter );

	m_title			= m_pFilter->title.c_str();
	m_auther		= UTF16fromUTF8(m_pFilter->author).c_str();
	m_version		= UTF16fromUTF8(m_pFilter->version).c_str();
	m_description	= UTF16fromUTF8(m_pFilter->comment).c_str();
	m_filterType	= m_pFilter->filterType;
	m_headerName	= m_pFilter->headerName.c_str();
	m_multipleMatch	= m_pFilter->multipleMatches;
	m_urlPattern	= (m_pFilter->urlPattern).c_str();
	m_boundesMatch	= (m_pFilter->boundsPattern).c_str();
	m_windowWidth	= m_pFilter->windowWidth;
	m_matchPattern	= (m_pFilter->matchPattern).c_str();
	m_replacePattern= (m_pFilter->replacePattern).c_str();
}

CFilterEditWindow::~CFilterEditWindow()
{
	delete m_pTempFilter;

	if (m_pTestWindow->IsWindow())
		m_pTestWindow->DestroyWindow();
}

BOOL CFilterEditWindow::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	SetWindowText(GetTranslateMessage(IDD_FILTEREDIT).c_str());

	CComboBox cmb = GetDlgItem(IDC_COMBO_FILTERTYPE);
	cmb.AddString(GetTranslateMessage(ID_TRANS_WEBPAGE).c_str());
	cmb.AddString(GetTranslateMessage(ID_TRANS_OUTHEADER).c_str());
	cmb.AddString(GetTranslateMessage(ID_TRANS_INHEADER).c_str());

	ChangeControlTextForTranslateMessage(m_hWnd, IDC_STATIC_FILTERTITLE);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_STATIC_AUTHOR);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_EDIT_VERSION);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_STATIC_DESCRIPTION);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_STATIC_FILTERTYPE);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_STATIC_HEADERNAME);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_STATIC_BOUNDS);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_STATIC_URLPATTERN);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_CHECKBOX_MULTIPLEMUTCH);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_STATIC_BOUNDMATCH);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_STATIC_LIMIT);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_STATIC_MATCHPATTERN);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_STATIC_REPLACE);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_BUTTON_TEST);
	ChangeControlTextForTranslateMessage(m_hWnd, IDC_BUTTON_TEST_URLPATTERN);
	ChangeControlTextForTranslateMessage(m_hWnd, IDOK);
	ChangeControlTextForTranslateMessage(m_hWnd, IDCANCEL);

	CComboBox	cmbHeaderName = GetDlgItem(IDC_COMBO_HEADERNAME);
	if (m_filterType == CFilterDescriptor::kFilterText)
		cmbHeaderName.EnableWindow(FALSE);

	LPCTSTR headerCollection[] = {
		_T("Accept-Language"), _T("Content-Encoding"), _T("Content-Type"), _T("Cookie"), _T("Location"), _T("Pragma"), _T("Referer"), _T("URL"), _T("User-Agent")
	};
	for (auto headerName : headerCollection)
		cmbHeaderName.AddString(headerName);

	DoDataExchange(DDX_LOAD);

    // ダイアログリサイズ初期化
    DlgResize_Init(true, true, WS_CLIPCHILDREN | WS_THICKFRAME | WS_MAXIMIZEBOX);

	std::string settingsPath = CT2A(Misc::GetExeDirectory() + _T("settings.ini"));
	ptree pt;
	try {
		read_ini(settingsPath, pt);
	} catch (...) {
	}

	CRect rcWindow;
	rcWindow.top	= pt.get("FilterEditWindow.top", 0);
	rcWindow.left	= pt.get("FilterEditWindow.left", 0);
	rcWindow.right	= pt.get("FilterEditWindow.right", 0);
	rcWindow.bottom	= pt.get("FilterEditWindow.bottom", 0);
	if (rcWindow != CRect())
		MoveWindow(&rcWindow);

	if (auto value = pt.get_optional<int>("FilterEditWindow.SplitterPos"))
		_MoveSplitterPos(value.get());

	return 0;
}

void CFilterEditWindow::OnDestroy()
{
	std::string settingsPath = CT2A(Misc::GetExeDirectory() + _T("settings.ini"));
	ptree pt;
	try {
		read_ini(settingsPath, pt);
	} catch (...) {
	}

	CRect rcWindow;
	GetWindowRect(&rcWindow);
		
	pt.put("FilterEditWindow.top", rcWindow.top);
	pt.put("FilterEditWindow.left", rcWindow.left);
	pt.put("FilterEditWindow.right", rcWindow.right);
	pt.put("FilterEditWindow.bottom", rcWindow.bottom);
	
	pt.put("FilterEditWindow.SplitterPos", _GetSplitterPos());

	write_ini(settingsPath, pt);
}


void CFilterEditWindow::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRect rcMatching = _GetControlRect(IDC_EDIT_MATCHPATTERN);
	CRect rcReplace = _GetControlRect(IDC_EDIT_REPLACEPATTERN);

	CRect rcDragBound;
	rcDragBound.top = rcMatching.bottom;
	rcDragBound.left = rcMatching.left;
	rcDragBound.right = rcMatching.right;
	rcDragBound.bottom = rcReplace.top;

	if (rcDragBound.PtInRect(point)) {
		if (::DragDetect(m_hWnd, point)) {
			SetCapture();
			SetCursor(LoadCursor(0, IDC_SIZENS));
		}
	}
}

int		CFilterEditWindow::_GetSplitterPos()
{
	CRect rcMatching = _GetControlRect(IDC_EDIT_MATCHPATTERN);
	return rcMatching.bottom;
}

void	CFilterEditWindow::_MoveSplitterPos(int cyPos)
{
	CRect rcMatching = _GetControlRect(IDC_EDIT_MATCHPATTERN);
	CRect rcReplace = _GetControlRect(IDC_EDIT_REPLACEPATTERN);

	CRect rcDragBound;
	rcDragBound.top = rcMatching.bottom;
	rcDragBound.left = rcMatching.left;
	rcDragBound.right = rcMatching.right;
	rcDragBound.bottom = rcReplace.top;

	rcDragBound.MoveToY(cyPos);

	CRect rcText = _GetControlRect(IDC_STATIC_REPLACE);
	rcText.MoveToY(rcDragBound.top + kcyStaticReplaceTextMargin);

	rcMatching.bottom = rcDragBound.top;
	rcReplace.top = rcDragBound.bottom;
	if (rcMatching.Height() < kMinEditHeight || rcReplace.Height() < kMinEditHeight)
		return;

	GetDlgItem(IDC_STATIC_REPLACE).MoveWindow(&rcText);
	GetDlgItem(IDC_EDIT_MATCHPATTERN).MoveWindow(&rcMatching);
	GetDlgItem(IDC_EDIT_REPLACEPATTERN).MoveWindow(&rcReplace);
}

void CFilterEditWindow::OnMouseMove(UINT nFlags, CPoint point)
{
	if (GetCapture() != m_hWnd)
		return;

	int moveY = point.y - (kMatchReplaceSpace / 2);
	_MoveSplitterPos(moveY);
}

void CFilterEditWindow::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (GetCapture() != m_hWnd)
		return;

	OnMouseMove(nFlags, point);
	ReleaseCapture();
	SetCursor(LoadCursor(0, IDC_ARROW));

	POINT ptBack = m_ptMinTrackSize;
	DlgResize_Init(true, true, WS_CLIPCHILDREN | WS_THICKFRAME | WS_MAXIMIZEBOX);
	m_ptMinTrackSize = ptBack;
}

CRect	CFilterEditWindow::_GetControlRect(int nID)
{
	CRect rcArea;
	GetDlgItem(nID).GetWindowRect(&rcArea);
	ScreenToClient(&rcArea);
	return rcArea;
}

void CFilterEditWindow::DlgResize_UpdateLayout(int cxWidth, int cyHeight)
{
	__super::DlgResize_UpdateLayout(cxWidth, cyHeight);

	CRect rcArea = _GetControlRect(IDC_STATIC_EDITAREA);
	
	CRect rcReplace = rcArea;
	rcReplace.top = rcArea.bottom - _GetControlRect(IDC_EDIT_REPLACEPATTERN).Height();

	CRect rcText = _GetControlRect(IDC_STATIC_REPLACE);
	rcText.MoveToY(rcReplace.top - kMatchReplaceSpace + kcyStaticReplaceTextMargin);

	CRect rcMatch = rcArea;
	rcMatch.bottom = rcReplace.top - kMatchReplaceSpace;

	if (rcMatch.Height() < kMinEditHeight) {
		rcMatch.bottom = rcMatch.top + kMinEditHeight;

		rcText.MoveToY(rcMatch.bottom + kcyStaticReplaceTextMargin);

		rcReplace.top = rcMatch.bottom + kMatchReplaceSpace;
	}

	GetDlgItem(IDC_EDIT_MATCHPATTERN).MoveWindow(&rcMatch);
	GetDlgItem(IDC_STATIC_REPLACE).MoveWindow(&rcText);
	GetDlgItem(IDC_EDIT_REPLACEPATTERN).MoveWindow(&rcReplace);
}


void CFilterEditWindow::OnFilterSelChange(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	CComboBox cmb = GetDlgItem(IDC_COMBO_FILTERTYPE);
	if (cmb.GetCurSel() == 0) {
		GetDlgItem(IDC_COMBO_HEADERNAME).EnableWindow(FALSE);
	} else {
		GetDlgItem(IDC_COMBO_HEADERNAME).EnableWindow(TRUE);
	}
}

void CFilterEditWindow::OnOK(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	if (_SaveToTempFilter() == false)
		return;

	//m_pTempFilter->TestValidity();
	if (m_pTempFilter->CreateMatcher() == false) {
		MessageBox(m_pTempFilter->errorMsg.c_str(), NULL, MB_ICONERROR);
		return ;
	}

	// 念のため
	CCritSecLock	lock(CSettings::s_csFilters);
	*m_pFilter = *m_pTempFilter;

	EndDialog(nID);
}


void CFilterEditWindow::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	EndDialog(nID);
}

void CFilterEditWindow::OnTest(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	if (_SaveToTempFilter() == false)
		return;

	if (m_pTestWindow->IsWindow() == FALSE) {
		m_pTestWindow->Create(GetParent());
	}
	m_pTestWindow->SetURLPatternMode(nID == IDC_BUTTON_TEST_URLPATTERN);
	m_pTestWindow->ShowWindow(TRUE);
}


void CFilterEditWindow::OnDataExchangeError(UINT nCtrlID, BOOL bSave)
{
	if (nCtrlID == IDC_EDIT_WINDOWWIDTH) {
		_XData data;
		data.intData.nMin = 1;
		data.intData.nMax = INT_MAX;
		OnDataValidateError(nCtrlID, bSave, data);

	} else {
		CString strMsg;
		strMsg.Format(_T("コントロール（ID:%u）とのデータ交換に失敗。"), nCtrlID);
		MessageBox(strMsg, _T("DataExchangeError"), MB_ICONWARNING);

		::SetFocus(GetDlgItem(nCtrlID));
	}
}

void CFilterEditWindow::OnDataValidateError(UINT nCtrlID, BOOL bSave, _XData& data)
{
	CString strMsg;
	strMsg.Format(_T("%d から %d までの値を入力してください。"),
		data.intData.nMin, data.intData.nMax);
	MessageBox(strMsg, _T("DataValidateError"), MB_ICONEXCLAMATION);

	::SetFocus(GetDlgItem(nCtrlID));
}



bool	CFilterEditWindow::_SaveToTempFilter()
{
	if (DoDataExchange(DDX_SAVE) == FALSE)
		return false;

	m_pTempFilter->title		= m_title.GetBuffer();
	m_pTempFilter->author		= UTF8fromUTF16(m_auther.GetBuffer());
	m_pTempFilter->version		= UTF8fromUTF16(m_version.GetBuffer());
	m_pTempFilter->comment		= UTF8fromUTF16(m_description.GetBuffer());
	m_pTempFilter->filterType	= static_cast<CFilterDescriptor::FilterType>(m_filterType);
	if (m_pTempFilter->filterType != CFilterDescriptor::kFilterText) {
		m_pTempFilter->headerName = m_headerName.GetBuffer();
	} else {
		m_pTempFilter->headerName.clear();
	}
	m_pTempFilter->multipleMatches	= m_multipleMatch;
	m_pTempFilter->urlPattern	= (m_urlPattern.GetBuffer());
	m_pTempFilter->boundsPattern= (m_boundesMatch.GetBuffer());
	m_pTempFilter->windowWidth	= m_windowWidth;
	m_pTempFilter->matchPattern	= (m_matchPattern.GetBuffer());
	m_pTempFilter->replacePattern= (m_replacePattern.GetBuffer());
	return  true;
}





