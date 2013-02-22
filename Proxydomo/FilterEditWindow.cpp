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
#include "proximodo\matcher.h"
#include "proximodo\expander.h"
#include "Settings.h"
#include "Misc.h"

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

class CFilterTestWindow : public CDialogImpl<CFilterTestWindow>
{
public:
	enum { IDD = IDD_FILTERTEST };

	enum { kMaxPrefCount = 1000 };

	CFilterTestWindow(CFilterDescriptor* pfd, std::function<void ()> func) : 
		m_pFilter(pfd), m_funcSaveToTempFilter(func) { }

	BEGIN_MSG_MAP_EX( CFilterTestWindow )
		MSG_WM_INITDIALOG( OnInitDialog )
		MSG_WM_DESTROY( OnDestroy )
		COMMAND_ID_HANDLER_EX( IDCANCEL, OnCancel )
		COMMAND_ID_HANDLER_EX( IDC_BUTTON_TEST, OnTestAnalyze )
		COMMAND_ID_HANDLER_EX( IDC_BUTTON_ANALYZE, OnTestAnalyze )
	END_MSG_MAP()

	// void OnCommandIDHandlerEX(UINT uNotifyCode, int nID, CWindow wndCtl)

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
	{
		m_editTest	= GetDlgItem(IDC_EDIT_TESTTEXT);
		m_editResult= GetDlgItem(IDC_EDIT_RESULTTEXT);

		m_editTest.SetWindowText(s_strLastTest);

		return 0;
	}

	void OnDestroy() { s_strLastTest = MiscGetWindowText(m_editTest); }

	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
	{
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
        bool okayChars[256];
        CMatcher matcher(m_pFilter->matchPattern, filter);
        matcher.mayMatch(okayChars);
        CMatcher* boundsMatcher = NULL;
		if (m_pFilter->filterType == CFilterDescriptor::kFilterText && !m_pFilter->boundsPattern.empty()) {
            boundsMatcher = new CMatcher(m_pFilter->boundsPattern, filter);
            bool tab[256];
            boundsMatcher->mayMatch(tab);
            for (int i = 0; i < 256; i++) 
				okayChars[i] = okayChars[i] && tab[i];
        }

        // special processing for <start> and <end> filters
        if (m_pFilter->matchPattern == "<start>" || m_pFilter->matchPattern == "<end>") {
            std::string str = CExpander::expand(m_pFilter->replacePattern, filter);
            filter.unlock();
            if (m_pFilter->matchPattern == "<start>") {
				str += CT2A(MiscGetWindowText(m_editTest));
            } else {
				std::string temp = str;
                str = CT2A(MiscGetWindowText(m_editTest));
				str += temp;
            }
			m_editResult.SetWindowText(CA2T(str.c_str()));
            return;
        }

        std::stringstream result;
        std::string text;
        int size = 0;
        int run = 0;
        int numMatch = 0;
		auto starttime = std::chrono::high_resolution_clock::now();
            
        do {
            text = CT2A(MiscGetWindowText(m_editTest));
            size = text.size();
            result.str("");
            owner.killed = false;
            filter.bypassed = false;

            // Test of a text filter.
            // We don't take URL pattern into account.
			if (m_pFilter->filterType == CFilterDescriptor::kFilterText) {

                bool found = false;
                const char* bufHead = text.c_str();
                const char* bufTail = bufHead + size;
                const char* index   = bufHead;
                const char* done    = bufHead;
                while (index<=bufTail && !owner.killed && !filter.bypassed) {
                    if (index<bufTail && !okayChars[(unsigned char)(*index)]) {
                        ++index;
                        continue;
                    }
                    bool matched;
                    const char *end, *reached;
                    const char *stop = index + m_pFilter->windowWidth;
                    if (stop > bufTail) stop = bufTail;
                    filter.clearMemory();
                    if (boundsMatcher) {
                        matched = boundsMatcher->match(index, stop, end, reached);
                        filter.unlock();
                        if (!matched) {
                            ++index;
                            continue;
                        }
                        stop = end;
                    }
                    matched = matcher.match(index, stop, end, reached);
                    filter.unlock();
                    if (!matched || (boundsMatcher && end != stop)) {
                        ++index;
                        continue;
                    }
                    found = true;
                    if (run == 0) numMatch++;
                    result << string(done, (size_t)(index-done));
                    result << CExpander::expand(m_pFilter->replacePattern, filter);
                    filter.unlock();
                    done = end;
                    if (end == index) ++index; else index = end;
                }
                if (!owner.killed) result << string(done, (size_t)(bufTail-done));
                if (!found) result.str(/*CSettings::ref().getMessage*/("TEST_NO_MATCH"));

            // Test of a header filter. Much simpler...
            } else {

                const char *index, *stop, *end, *reached;
                index = text.c_str();
                stop  = index + text.size();
                if ((!text.empty() || m_pFilter->matchPattern.empty())
                    && matcher.match(index, stop, end, reached)) {

                    result << CExpander::expand(m_pFilter->replacePattern, filter);
                    if (run == 0) numMatch++;
                } else {
                    result << /*CSettings::ref().getMessage*/("TEST_NO_MATCH");
                }
                filter.unlock();
            }
        // If profiling, repeat this loop 1000 times or 10 seconds,
        // whichever comes first.
		} while (prof && run++ < kMaxPrefCount && (std::chrono::high_resolution_clock::now() - starttime) < std::chrono::seconds(10));
        if (boundsMatcher) delete boundsMatcher;

        if (prof) {
			int len = m_editTest.GetWindowTextLength();
			double time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - starttime).count();
			double avarage = (1000.0 * time / run);
			double busytime = time ? (1000.0 * len * run / (1024.0 * time)) : 0.0;			

			CString text;
			text.Format(_T("分析結果...\r\n サンプルテキスト: %d バイト\r\n 一致回数: %d\r\n 平均時間: %lf ミリ秒\r\n 処理速度: %lf KB/s"), len, numMatch, avarage, busytime);

			m_editResult.SetWindowText(text);
        } else {
			m_editResult.SetWindowText(CA2T(result.str().c_str()));
        }

	}

private:
	// Data members
	CFilterDescriptor*	m_pFilter;
	std::function<void ()>	m_funcSaveToTempFilter;
	CEdit	m_editTest;
	CEdit	m_editResult;
	static CString s_strLastTest;
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
	m_auther		= m_pFilter->author.c_str();
	m_version		= m_pFilter->version.c_str();
	m_description	= m_pFilter->comment.c_str();
	m_filterType	= m_pFilter->filterType;
	m_headerName	= m_pFilter->headerName.c_str();
	m_multipleMatch	= m_pFilter->multipleMatches;
	m_urlPattern	= m_pFilter->urlPattern.c_str();
	m_boundesMatch	= m_pFilter->boundsPattern.c_str();
	m_windowWidth	= m_pFilter->windowWidth;
	m_matchPattern	= m_pFilter->matchPattern.c_str();
	m_replacePattern= m_pFilter->replacePattern.c_str();
}

CFilterEditWindow::~CFilterEditWindow()
{
	delete m_pTempFilter;

	if (m_pTestWindow->IsWindow())
		m_pTestWindow->DestroyWindow();
	delete m_pTestWindow;
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

	using namespace boost::property_tree;
	std::string settingsPath = CT2A(Misc::GetExeDirectory() + _T("settings.ini"));
	ptree pt;
	read_ini(settingsPath, pt);
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
	using namespace boost::property_tree;	
	std::string settingsPath = CT2A(Misc::GetExeDirectory() + _T("settings.ini"));
	ptree pt;
	read_ini(settingsPath, pt);

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

	m_pTempFilter->TestValidity();
	if (m_pTempFilter->errorMsg.size() > 0) {
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
	m_pTestWindow->ShowWindow(TRUE);
}

void	CFilterEditWindow::_SaveToTempFilter()
{
	DoDataExchange(DDX_SAVE);

	m_pTempFilter->title		= CT2A(m_title);
	m_pTempFilter->author		= CT2A(m_auther);
	m_pTempFilter->version		= CT2A(m_version);
	m_pTempFilter->comment		= CT2A(m_description);
	m_pTempFilter->filterType	= static_cast<CFilterDescriptor::FilterType>(m_filterType);
	m_pTempFilter->headerName	= CT2A(m_headerName);
	m_pTempFilter->multipleMatches	= m_multipleMatch;
	m_pTempFilter->urlPattern	= CT2A(m_urlPattern);
	m_pTempFilter->boundsPattern= CT2A(m_boundesMatch);
	m_pTempFilter->windowWidth	= m_windowWidth;
	m_pTempFilter->matchPattern	= CT2A(m_matchPattern);
	m_pTempFilter->replacePattern= CT2A(m_replacePattern);
}





