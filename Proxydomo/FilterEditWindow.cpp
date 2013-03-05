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
#include "FilterDescriptor.h"
#include "FilterOwner.h"
#include "proximodo\filter.h"
#include "proximodo\expander.h"
#include "Settings.h"
#include "Misc.h"
#include "Matcher.h"
#include "CodeConvert.h"

using namespace CodeConvert;
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

	CFilterTestWindow(CFilterDescriptor* pfd, std::function<void ()> func) : 
		m_pFilter(pfd), m_funcSaveToTempFilter(func), m_bTestURLPattern(false) { }

	void	SetURLPatternMode(bool b)
	{
		m_bTestURLPattern = b;
		CString title = _T("フィルターテスト - ");
		title += m_bTestURLPattern ? _T("URLパターンテスト") : _T("マッチングパターンテスト");
		SetWindowText(title);
	}

	void OnFinalMessage(HWND /*hWnd*/) override { delete this; }

	BEGIN_DLGRESIZE_MAP( CFilterTestWindow )
		DLGRESIZE_CONTROL( IDC_EDIT_TESTTEXT, DLSZ_SIZE_X | DLSZ_SIZE_Y )
		DLGRESIZE_CONTROL( IDC_EDIT_RESULTTEXT, DLSZ_MOVE_Y | DLSZ_SIZE_X )

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
		m_editTest	= GetDlgItem(IDC_EDIT_TESTTEXT);
		m_editResult= GetDlgItem(IDC_EDIT_RESULTTEXT);

		m_editTest.SetWindowText(s_strLastTest);

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

		return 0;
	}

	void OnDestroy() { s_strLastTest = MiscGetWindowText(m_editTest); }

	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
	{
		std::string settingsPath = CT2A(Misc::GetExeDirectory() + _T("settings.ini"));
		ptree pt;
		try {
			read_ini(settingsPath, pt);
		} catch (...) {
		}

		CRect rcWindow;
		GetWindowRect(&rcWindow);
		
		pt.put("FilterTestWindow.top", rcWindow.top);
		pt.put("FilterTestWindow.left", rcWindow.left);
		pt.put("FilterTestWindow.right", rcWindow.right);
		pt.put("FilterTestWindow.bottom", rcWindow.bottom);

		write_ini(settingsPath, pt);

		ShowWindow(FALSE);
	}

	void OnTestAnalyze(UINT uNotifyCode, int nID, CWindow wndCtl)
	{
		m_funcSaveToTempFilter();

		m_pFilter->TestValidity();
        // ensure there is no parsing error in this filter
        if (!m_pFilter->errorMsg.empty()) {
			MessageBox(CA2T(m_pFilter->errorMsg.c_str()));
            return;
        }

		bool prof = (nID == IDC_BUTTON_ANALYZE);

        // create a fake text filter (would normally be a
        // resident CRequestManager + CTextBuffer + CTextFilter).
        // This instanciation is not profiled, because it the real
        // world this only happens when Proximodo starts up and
        // processes the first few pages.
        CFilterOwner owner;
		owner.SetInHeader("Host", "www.host.org");
        std::string url = "http://www.host.org/path/page.html?query=true#anchor";
        owner.url.parseUrl(url);
        owner.cnxNumber = 1;
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
			std::string text = CT2A(MiscGetWindowText(m_editTest));
			CUrl	url;
			url.parseUrl(text);
			const char* end = nullptr;
			if (matcher.match(url.getFromHost(), &filter)) {
				m_editResult.SetWindowText(_T("マッチしました！"));
			} else {
				m_editResult.SetWindowText(_T("マッチしませんでした..."));
			}
			return ;
		}

        std::wstringstream result;
        std::wstring text;
        int size = 0;
        int run = 0;
        int numMatch = 0;
		auto starttime = std::chrono::high_resolution_clock::now();
            
        do {
            text = MiscGetWindowText(m_editTest);
            size = text.size();
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
                while (index <= bufTail && !owner.killed && !filter.bypassed) {
                    if (index < bufTail && !okayChars[(unsigned char)(*index)]) {
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
                    if (run == 0) numMatch++;
                    result << std::wstring(done, (size_t)(index-done));
                    result << CExpander::expand(m_pFilter->replacePattern, filter);
                    filter.unlock();
                    done = end;
                    if (end == index) ++index; else index = end;
                }
                if (!owner.killed) result << std::wstring(done, (size_t)(bufTail-done));
                if (!found) result.str(/*CSettings::ref().getMessage*/(L"TEST_NO_MATCH"));

            // Test of a header filter. Much simpler...
            } else {

                const wchar_t *index, *stop, *end;
                index = text.c_str();
                stop  = index + text.size();
                if ((!text.empty() || m_pFilter->matchPattern.empty())
                    && matcher.match(index, stop, end, &matchData)) {

                    result << CExpander::expand(m_pFilter->replacePattern, filter);
                    if (run == 0) numMatch++;
                } else {
                    result << /*CSettings::ref().getMessage*/("TEST_NO_MATCH");
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

			CString text;
			text.Format(
				_T("分析結果...\r\n サンプルテキスト: %d バイト\r\n 一致回数: %d\r\n ")
				_T("平均時間: %d ミリ秒\r\n 処理速度: %lf KB/s"), 
				len, numMatch, (int)avarage, busytime);

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
	std::function<void ()>	m_funcSaveToTempFilter;
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
	m_headerName	= UTF16fromUTF8(m_pFilter->headerName).c_str();
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
	CComboBox cmb = GetDlgItem(IDC_COMBO_FILTERTYPE);
	cmb.AddString(_T("ウェブページ"));
	cmb.AddString(_T("送信ヘッダ"));
	cmb.AddString(_T("受信ヘッダ"));

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
    DlgResize_Init(true, true, WS_THICKFRAME | WS_MAXIMIZEBOX);

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

	write_ini(settingsPath, pt);
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
	_SaveToTempFilter();

	//m_pTempFilter->TestValidity();
	if (m_pTempFilter->CreateMatcher() == false) {
		MessageBox(CA2T(m_pTempFilter->errorMsg.c_str()), NULL, MB_ICONERROR);
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
	_SaveToTempFilter();

	if (m_pTestWindow->IsWindow() == FALSE) {
		m_pTestWindow->Create(m_hWnd);
	}
	m_pTestWindow->SetURLPatternMode(nID == IDC_BUTTON_TEST_URLPATTERN);
	m_pTestWindow->ShowWindow(TRUE);
}

void	CFilterEditWindow::_SaveToTempFilter()
{
	DoDataExchange(DDX_SAVE);

	m_pTempFilter->title		= m_title.GetBuffer();
	m_pTempFilter->author		= UTF8fromUTF16(m_auther.GetBuffer());
	m_pTempFilter->version		= UTF8fromUTF16(m_version.GetBuffer());
	m_pTempFilter->comment		= UTF8fromUTF16(m_description.GetBuffer());
	m_pTempFilter->filterType	= static_cast<CFilterDescriptor::FilterType>(m_filterType);
	m_pTempFilter->headerName	= UTF8fromUTF16(m_headerName.GetBuffer());
	m_pTempFilter->multipleMatches	= m_multipleMatch;
	m_pTempFilter->urlPattern	= (m_urlPattern.GetBuffer());
	m_pTempFilter->boundsPattern= (m_boundesMatch.GetBuffer());
	m_pTempFilter->windowWidth	= m_windowWidth;
	m_pTempFilter->matchPattern	= (m_matchPattern.GetBuffer());
	m_pTempFilter->replacePattern= (m_replacePattern.GetBuffer());
}





