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
#include "resource.h"

class CFilterDescriptor;
class CFilterTestWindow;

class CFilterEditWindow : 
	public CDialogImpl<CFilterEditWindow>, 
	public CWinDataExchange<CFilterEditWindow>,
	public CDialogResize<CFilterEditWindow>
{
public:
	enum { IDD = IDD_FILTEREDIT };

	CFilterEditWindow(CFilterDescriptor* pfd);
	~CFilterEditWindow();

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
		DDX_INT(IDC_EDIT_WINDOWWIDTH		,	m_windowWidth)
		DDX_TEXT(IDC_EDIT_MATCHPATTERN		,	m_matchPattern)
		DDX_TEXT(IDC_EDIT_REPLACEPATTERN	,	m_replacePattern)

	END_DDX_MAP()

	BEGIN_DLGRESIZE_MAP( CFilterEditWindow )
		DLGRESIZE_CONTROL( IDC_EDIT_FILTERNAME, DLSZ_SIZE_X )
		DLGRESIZE_CONTROL( IDC_EDIT_AUTHERNAME, DLSZ_SIZE_X )
		DLGRESIZE_CONTROL( IDC_STATIC_VERSION, DLSZ_MOVE_X )
		DLGRESIZE_CONTROL( IDC_EDIT_VERSION, DLSZ_MOVE_X )
		DLGRESIZE_CONTROL( IDC_EDIT_FILTERDISCRIPTION, DLSZ_SIZE_X )
		DLGRESIZE_CONTROL( IDC_CHECKBOX_MULTIPLEMUTCH, DLSZ_MOVE_X )
		DLGRESIZE_CONTROL( IDC_EDIT_URLPATTERN, DLSZ_SIZE_X )
		DLGRESIZE_CONTROL( IDC_EDIT_BOUNDSPATTERN, DLSZ_SIZE_X )
		DLGRESIZE_CONTROL( IDC_STATIC_LIMIT, DLSZ_MOVE_X )
		DLGRESIZE_CONTROL( IDC_EDIT_WINDOWWIDTH, DLSZ_MOVE_X )
		DLGRESIZE_CONTROL( IDC_STATIC_BOUNDS, DLSZ_SIZE_X )
		DLGRESIZE_CONTROL( IDC_BUTTON_TEST, DLSZ_MOVE_X )
		DLGRESIZE_CONTROL( IDC_EDIT_MATCHPATTERN, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL( IDC_STATIC_REPLACE, DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL( IDC_EDIT_REPLACEPATTERN, DLSZ_MOVE_Y | DLSZ_SIZE_X )
		DLGRESIZE_CONTROL( IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL( IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y )

	END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP_EX( CFilterEditWindow )
		MSG_WM_INITDIALOG( OnInitDialog )
		MSG_WM_DESTROY( OnDestroy )
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
	void OnFilterSelChange(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnOK(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);

	void OnTest(UINT uNotifyCode, int nID, CWindow wndCtl);

private:
	void	_SaveToTempFilter();

	// Data members
	CFilterDescriptor*	m_pFilter;
	CFilterDescriptor*	m_pTempFilter;
	CFilterTestWindow*	m_pTestWindow;

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