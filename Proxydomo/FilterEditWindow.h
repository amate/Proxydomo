/**
*	@file	FilterEditWindow.h
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
#pragma once

#include <atlcrack.h>
#include <atlctrls.h>
#include <atlframe.h>
#include <atlddx.h>
#include <atlmisc.h>
#include "resource.h"


class CEditSelAllHelper : public CWindowImpl<CEditSelAllHelper, CEdit>
{
public:
	BEGIN_MSG_MAP_EX(CFilterEditWindow)
		MSG_WM_KEYDOWN(OnKeyDown)
	END_MSG_MAP()

	void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
		if (nChar == 'A' && GetKeyState(VK_CONTROL) < 0) {
			SetSelAll(TRUE);
		}
		SetMsgHandled(FALSE);
	}
};

class CFilterDescriptor;
class CFilterTestWindow;

class CFilterEditWindow : 
	public CDialogImpl<CFilterEditWindow>, 
	public CWinDataExchange<CFilterEditWindow>,
	public CDialogResize<CFilterEditWindow>
{
public:
	enum { IDD = IDD_FILTEREDIT };

	enum {
		kMinEditHeight = 24,
		kMatchReplaceSpace = 20,
		kcyStaticReplaceTextMargin = 4,
	};

	CFilterEditWindow(CFilterDescriptor* pfd);
	~CFilterEditWindow();

	// overrides
	void OnDataExchangeError(UINT nCtrlID, BOOL bSave);
	void OnDataValidateError(UINT nCtrlID, BOOL bSave, _XData& data);

	BEGIN_DDX_MAP( CFilterEditWindow )
		DDX_TEXT(IDC_EDIT_FILTERNAME		,	m_title)
		DDX_TEXT(IDC_EDIT_AUTHERNAME		,	m_auther)
		DDX_TEXT(IDC_EDIT_VERSION			,	m_version)
		DDX_TEXT(IDC_EDIT_FILTERDISCRIPTION	,	m_description)
		DDX_COMBO_INDEX(IDC_COMBO_FILTERTYPE,	m_filterType)
		DDX_TEXT(IDC_COMBO_HEADERNAME		,	m_headerName)
		DDX_CHECK(IDC_CHECKBOX_MULTIPLEMUTCH,	m_multipleMatch)
		DDX_TEXT(IDC_EDIT_URLPATTERN		,	m_urlPattern)
		DDX_TEXT(IDC_EDIT_BOUNDSPATTERN		,	m_boundesMatch)
		DDX_INT_RANGE(IDC_EDIT_WINDOWWIDTH, m_windowWidth, 1, INT_MAX)
		DDX_TEXT(IDC_EDIT_MATCHPATTERN		,	m_matchPattern)
		DDX_TEXT(IDC_EDIT_REPLACEPATTERN	,	m_replacePattern)

		DDX_CONTROL(IDC_EDIT_FILTERDISCRIPTION, m_wndDisciption)
		DDX_CONTROL(IDC_EDIT_MATCHPATTERN	, m_wndMatchPattern)
		DDX_CONTROL(IDC_EDIT_REPLACEPATTERN, m_wndReplaceText)
	END_DDX_MAP()

	BEGIN_DLGRESIZE_MAP( CFilterEditWindow )
		DLGRESIZE_CONTROL( IDC_EDIT_FILTERNAME, DLSZ_SIZE_X )
		DLGRESIZE_CONTROL( IDC_EDIT_AUTHERNAME, DLSZ_SIZE_X )
		DLGRESIZE_CONTROL( IDC_STATIC_VERSION, DLSZ_MOVE_X )
		DLGRESIZE_CONTROL( IDC_EDIT_VERSION, DLSZ_MOVE_X )
		DLGRESIZE_CONTROL( IDC_EDIT_FILTERDISCRIPTION, DLSZ_SIZE_X )
		DLGRESIZE_CONTROL( IDC_COMBO_HEADERNAME, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL( IDC_CHECKBOX_MULTIPLEMUTCH, DLSZ_MOVE_X )
		DLGRESIZE_CONTROL( IDC_EDIT_URLPATTERN, DLSZ_SIZE_X )
		DLGRESIZE_CONTROL( IDC_EDIT_BOUNDSPATTERN, DLSZ_SIZE_X )
		DLGRESIZE_CONTROL( IDC_STATIC_LIMIT, DLSZ_MOVE_X )
		DLGRESIZE_CONTROL( IDC_EDIT_WINDOWWIDTH, DLSZ_MOVE_X )
		DLGRESIZE_CONTROL( IDC_STATIC_BOUNDS, DLSZ_SIZE_X )
		DLGRESIZE_CONTROL( IDC_STATIC_EDITAREA, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL( IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL( IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y )

	END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP_EX( CFilterEditWindow )
		MSG_WM_INITDIALOG( OnInitDialog )
		MSG_WM_DESTROY( OnDestroy )
		MSG_WM_SETCURSOR( OnSetCursor )
		MSG_WM_LBUTTONDOWN( OnLButtonDown )
		MSG_WM_MOUSEMOVE( OnMouseMove )
		MSG_WM_LBUTTONUP( OnLButtonUp )
		COMMAND_HANDLER_EX( IDC_COMBO_FILTERTYPE, CBN_SELCHANGE, OnFilterSelChange )
		COMMAND_ID_HANDLER_EX( IDOK, OnOK )
		COMMAND_ID_HANDLER_EX( IDCANCEL, OnCancel )
		COMMAND_ID_HANDLER_EX( IDC_BUTTON_TEST, OnTest )
		COMMAND_ID_HANDLER_EX( IDC_BUTTON_TEST_URLPATTERN, OnTest )
		CHAIN_MSG_MAP( CDialogResize<CFilterEditWindow> )
	END_MSG_MAP()


	// void OnCommandIDHandlerEX(UINT uNotifyCode, int nID, CWindow wndCtl)

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnDestroy();

	BOOL OnSetCursor(CWindow wnd, UINT nHitTest, UINT message);
	void OnLButtonDown(UINT nFlags, CPoint point);
	void OnMouseMove(UINT nFlags, CPoint point);
	void OnLButtonUp(UINT nFlags, CPoint point);
	void DlgResize_UpdateLayout(int cxWidth, int cyHeight);

	void OnFilterSelChange(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnOK(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);

	void OnTest(UINT uNotifyCode, int nID, CWindow wndCtl);

private:
	bool	_SaveToTempFilter();

	int		_GetSplitterPos();
	void	_MoveSplitterPos(int cyPos);
	CRect	_GetControlRect(int nID);

	// Data members
	CFilterDescriptor*	m_pFilter;
	CFilterDescriptor*	m_pTempFilter;
	CFilterTestWindow*	m_pTestWindow;

	CEditSelAllHelper	m_wndDisciption;
	CEditSelAllHelper	m_wndMatchPattern;
	CEditSelAllHelper	m_wndReplaceText;

	CString	m_title;
	CString	m_auther;
	CString	m_version;
	CString	m_description;
	int		m_filterType;
	CString	m_headerName;
	bool	m_multipleMatch;
	CString	m_urlPattern;
	CString	m_boundesMatch;
	int		m_windowWidth;
	CString	m_matchPattern;
	CString	m_replacePattern;
};