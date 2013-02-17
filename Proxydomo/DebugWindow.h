/**
 *	@file	DebugWindow.h
 *	@brief	とりあえず的な適当デバッグ用ダイアログを生成するクラス
 */
#pragma once

#define DEBUGPUT TRACE

#ifdef _DEBUG
 #ifndef TRACE
  #define TRACE		CDebugUtility::Write
 #endif
 #ifndef TRACEIN
  #define TRACEIN	CDebugUtility::WriteIn
 #endif
#ifndef TIMERSTART
	#define	TIMERSTART	CDebugUtility::TimerStart
#endif
#ifndef TIMERSTOP
	#define	TIMERSTOP	CDebugUtility::TimerStop
#endif

#else // NODEBUG
 #ifndef TRACE
  #define TRACE		__noop
 #endif 
 #ifndef TRACEIN
  #define TRACEIN	__noop
 #endif
#ifndef TIMERSTART
	#define	TIMERSTART	__noop
#endif
#ifndef TIMERSTOP
	#define	TIMERSTOP	__noop
#endif
#endif

/////////////////////////////////////////////////
// CDebugUtility

class CDebugUtility
{
public:
	CDebugUtility();
	~CDebugUtility();

	static void Write(LPCSTR pstrFormat, ...);
	static void Write(LPCWSTR pstrFormat, ...);
	static void WriteIn(LPCSTR pstrFormat, ...);
	static void WriteIn(LPCWSTR pstrFormat, ...);
	static void	TimerStart();
	static void	TimerStop(LPCTSTR pstrFormat, ...);

private:
	class Impl;
	static Impl*	pImpl;
};


#if 0
#include <atlbase.h>
#include <atlapp.h>
#include <atlmisc.h>
#include <atlctrls.h>
#include <atlcrack.h>
//#include "resource.h"
#ifndef IDD_DEBUGWINDOW
#define IDD_DEBUGWINDOW                 308
#endif

const bool	g_cnt_b_use_debug_window	=	false;

// DEBUGPUT(_T("test%s\n"), _T("test"));
#ifdef _DEBUG
 #ifndef DEBUGPUT
  #define DEBUGPUT	CDebugWindow::OutPutString
 #endif
 #ifndef DEBUGMENU
  #define DEBUGMENU	CDebugWindow::OutPutMenu
 #endif
#else
 #ifndef DEBUGPUT
  #define DEBUGPUT	__noop
 #endif 
 #ifndef DEBUGMENU
  #define DEBUGMENU	__noop
 #endif
#endif


/**
	とりあえず的な適当デバッグ用ダイアログを生成するクラス

	リリースビルド時にはコンパイルすらされないようにすべきではあるが、ﾏﾝﾄﾞｸｾ('A`)
	使い方はCreate()とDestroy()の呼び出しの間にOutPutStringを呼び出すだけ
 */

class CDebugWindow 
	: public CDialogImpl<CDebugWindow>
	, public CMessageFilter
{
	typedef CDialogImpl<CDebugWindow>  baseclass;

	CEdit					m_wndEdit;
	static CDebugWindow*	s_pThis;

public:
	enum { IDD = IDD_DEBUGWINDOW };

	void		Create(HWND hWndParent);
	void		Destroy();
	CEdit		GetEditCtrl() { return m_wndEdit; }

	static void OutPutString(LPCTSTR pstrFormat, ...);
	static void OutPutMenu(CMenuHandle menu);


    virtual BOOL PreTranslateMessage(MSG* pMsg) { 
		return CWindow::IsDialogMessage(pMsg); 
	}

	// Message map
	BEGIN_MSG_MAP( CDebugWindow )
		MSG_WM_INITDIALOG	( OnInitDialog	)
		MSG_WM_DESTROY		( OnDestroy		)
		MSG_WM_SIZE			( OnSize		)
		COMMAND_ID_HANDLER_EX( IDCANCEL, OnCancel )
	END_MSG_MAP()

	BOOL		OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void		OnDestroy();
	LRESULT 	OnSize(UINT, CSize size);

	void		OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);
};

#endif
