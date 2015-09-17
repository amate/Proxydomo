/**
 *	@file	DebugWindow.cpp
 *	@brief	とりあえず的な適当デバッグ用ダイアログを生成するクラス
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
#include "DebugWindow.h"
#include <ctime>
#include <fstream>
#include <codecvt>
#include <boost\timer.hpp>
#include "Misc.h"

#pragma comment(lib, "winmm.lib")

#ifdef _DEBUG
static CDebugUtility	g_debugutil;
__declspec(thread) DWORD	g_timer;
#endif

/////////////////////////////////////////////////
// CDebugUtility::Impl

class CDebugUtility::Impl
{
public:
	Impl();
	~Impl();

	void	Write(LPCSTR strFormat, va_list argList);
	void	Write(LPCWSTR strFormat, va_list argList);
	void	WriteIn(LPCSTR strFormat, va_list argList);
	void	WriteIn(LPCWSTR strFormat, va_list argList);

	void	_WriteConsole(LPCTSTR str);

	HANDLE	m_hOut;
};


//------------------------------------
CDebugUtility::Impl::Impl()  : m_hOut(NULL)
{
	// ログファイル作成
	FILE* fp = nullptr;
	_wfopen_s(&fp, Misc::GetExeDirectory() + _T("log.txt"), L"w");
	fclose(fp);

	::AllocConsole();
	m_hOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
}

//------------------------------------
CDebugUtility::Impl::~Impl()
{
	::FreeConsole();
}

//------------------------------------
void	CDebugUtility::Impl::Write(LPCSTR strFormat, va_list argList)
{
	CStringA str;
	str.FormatV(strFormat, argList);
	_WriteConsole(CStringW(str));
}

//------------------------------------
void	CDebugUtility::Impl::Write(LPCWSTR strFormat, va_list argList)
{
	CStringW str;
	str.FormatV(strFormat, argList);
	_WriteConsole(str);
}


//------------------------------------
void	CDebugUtility::Impl::WriteIn(LPCSTR strFormat, va_list argList)
{
	CStringA str;
	str.FormatV(strFormat, argList);
	str += "\r\n";
	_WriteConsole(CStringW(str));
}

//------------------------------------
void	CDebugUtility::Impl::WriteIn(LPCWSTR strFormat, va_list argList)
{
	CStringW str;
	str.FormatV(strFormat, argList);
	str += L"\r\n";
	_WriteConsole(str);
}

//------------------------------------
void	CDebugUtility::Impl::_WriteConsole(LPCTSTR str)
{
	CString strFilePath = Misc::GetExeDirectory() + _T("log.txt");
	std::wfstream	filestream(strFilePath, std::ios::app | std::ios::binary);
	if (filestream) {
		filestream.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t>));
		time_t time = std::time(nullptr);
		tm date = {};
		localtime_s(&date, &time);
		WCHAR strtime[128] = L"";
		std::wcsftime(strtime, _countof(strtime), L"%c | ", &date);
		filestream << strtime << str;
	}
	DWORD dwWrite;
	::WriteConsole(m_hOut, str, lstrlen(str), &dwWrite, NULL);
}




/////////////////////////////////////////////////
// CDebugUtility

CDebugUtility::Impl* CDebugUtility::pImpl = NULL;

//------------------------------------
CDebugUtility::CDebugUtility()
{ 
	pImpl = new Impl;
}

//------------------------------------
CDebugUtility::~CDebugUtility()
{
	delete pImpl;
}

//------------------------------------
void CDebugUtility::Write(LPCSTR pstrFormat, ...)
{
	va_list args;
	va_start(args, pstrFormat);	
	pImpl->Write(pstrFormat, args);
	va_end(args);
}


//------------------------------------
void CDebugUtility::Write(LPCWSTR pstrFormat, ...)
{
	va_list args;
	va_start(args, pstrFormat);	
	pImpl->Write(pstrFormat, args);
	va_end(args);
}

//------------------------------------
void CDebugUtility::WriteIn(LPCSTR pstrFormat, ...)
{
	va_list args;
	va_start(args, pstrFormat);	
	pImpl->WriteIn(pstrFormat, args);
	va_end(args);
}

//------------------------------------
void CDebugUtility::WriteIn(LPCWSTR pstrFormat, ...)
{
	va_list args;
	va_start(args, pstrFormat);	
	pImpl->WriteIn(pstrFormat, args);
	va_end(args);
}

void	CDebugUtility::TimerStart()
{
#ifdef _DEBUG
	g_timer = ::timeGetTime();
#endif
}

void	CDebugUtility::TimerStop(LPCTSTR pstrFormat, ...)
{	
#ifdef _DEBUG
	va_list args;
	va_start(args, pstrFormat);	
	CString str;
	str.FormatV(pstrFormat, args);
	va_end(args);
	CString strTime;
	strTime.Format(_T(" : %.4lf\n"), (static_cast<double>(::timeGetTime()) - static_cast<double>(g_timer)) / 1000.0);

	str += strTime;
	pImpl->_WriteConsole(str);
#endif
}


#if 0
#include "../resource.h"



#ifndef NDEBUG

CDebugWindow *		CDebugWindow::s_pThis = NULL;




void CDebugWindow::Create(HWND hWndParent)
{
	if (g_cnt_b_use_debug_window) {
		CWindow hWnd = baseclass::Create(hWndParent);
		hWnd.ModifyStyleEx(WS_EX_APPWINDOW, WS_EX_TOOLWINDOW);
		s_pThis = this;

		::DeleteFile(_GetFilePath(_T("log.txt")));
	}
}



void CDebugWindow::Destroy()
{
	if (g_cnt_b_use_debug_window) {
		if ( IsWindow() ) {
			baseclass::DestroyWindow();
		}
		s_pThis = NULL;
	}
}



void CDebugWindow::OutPutString(LPCTSTR pstrFormat, ...)
{
	if (!s_pThis)
		return;

	if ( !s_pThis->m_wndEdit.IsWindow() )
		return;

	if (!g_cnt_b_use_debug_window)
		return;

	CString strText;
	{
		va_list args;
		va_start(args, pstrFormat);
		
		strText.FormatV(pstrFormat, args);

		va_end(args);
	}

	FILE *fp = _tfopen(_GetFilePath(_T("log.txt")), _T("a"));
	if (fp) {
		_ftprintf(fp, _T("%s"), strText);
		fclose(fp);
	}
	strText.Replace(_T("\n"), _T("\r\n"));
//	if (bReturn) strText += _T("\r\n");

	s_pThis->m_wndEdit.AppendText(strText);
}

void CDebugWindow::OutPutMenu(CMenuHandle menu)
{
	OutPutString(_T("=======================\n"));
	for (int i = 0; i < menu.GetMenuItemCount(); ++i) {
		CString strText;
		menu.GetMenuString(i, strText, MF_BYPOSITION);
		UINT uCmdID = menu.GetMenuItemID(i);
		if (uCmdID == 0) { strText = _T("――――――"); }
		if (uCmdID == -1) {	// サブメニュー？
			OutPutString(_T("           : > %s\n"), strText);
		} else {
			OutPutString(_T("%05d : %s\n"), uCmdID, strText);
		}
	}
	OutPutString(_T("=======================\n\n"));

}


BOOL	CDebugWindow::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	SetMsgHandled(FALSE);

	//CRect rc(0, 170, 300, 600);
	CRect rc(-300, 0, 0, 500);
	MoveWindow(rc);

	m_wndEdit = GetDlgItem(IDC_EDIT1);
	//m_wndEdit.Create(m_hWnd, rc, NULL, ES_MULTILINE | WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL);

	// メッセージループにメッセージフィルタとアイドルハンドラを追加
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	pLoop->AddMessageFilter(this);
//	pLoop->AddIdleHandler(this);

	SetWindowPos(HWND_TOPMOST, -1, -1, -1, -1, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	OnSize(0, CSize(300, 600));

	return TRUE;
}

void	CDebugWindow::OnDestroy()
{
	// メッセージループからメッセージフィルタとアイドルハンドラを削除
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	pLoop->RemoveMessageFilter(this);
	//pLoop->RemoveIdleHandler(this);
}


LRESULT CDebugWindow::OnSize(UINT, CSize size)
{
	SetMsgHandled(FALSE);

	if (m_wndEdit.IsWindow()) {
		CRect rc;
		GetClientRect(&rc);
		::InflateRect(&rc, -4, -4);
		m_wndEdit.MoveWindow(rc);
	}
	return 0;
}


void	CDebugWindow::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	DestroyWindow();
}


#endif
#endif