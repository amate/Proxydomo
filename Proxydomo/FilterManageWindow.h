/**
*	@file	FilterManageWindow.h
*	@brief	フィルター管理ウィンドウ
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
#include <atlmisc.h>
#include "resource.h"

class CFilterDescriptor;

class CFilterManageWindow : 
	public CDialogImpl<CFilterManageWindow>,
	public CDialogResize<CFilterManageWindow>
{
public:
	enum { IDD = IDD_FILTERMANAGE };

	enum { 
		WM_CHECKSTATECHANGED = WM_APP + 1,
		WM_DRAGDROPFINISH = WM_APP + 2,
	};

	CFilterManageWindow();


	BEGIN_DLGRESIZE_MAP( CFilterManageWindow )
		DLGRESIZE_CONTROL( IDC_TREE_FILTER, DLSZ_SIZE_X | DLSZ_SIZE_Y )
		DLGRESIZE_CONTROL( IDC_BUTTON_IMPORTFROMPROXOMITRON, DLSZ_MOVE_X | DLSZ_MOVE_Y )
	END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP_EX( CFilterManageWindow )
		MSG_WM_INITDIALOG( OnInitDialog )
		MSG_WM_DESTROY( OnDestroy )
		COMMAND_ID_HANDLER_EX( IDCANCEL, OnCancel )
		NOTIFY_HANDLER_EX( IDC_TREE_FILTER, NM_CLICK, OnTreeFilterClick )
		MESSAGE_HANDLER_EX( WM_CHECKSTATECHANGED, OnCheckStateChanged )
		NOTIFY_HANDLER_EX( IDC_TREE_FILTER, NM_DBLCLK, OnTreeFilterDblClk )		
		NOTIFY_HANDLER_EX( IDC_TREE_FILTER, TVN_BEGINDRAG, OnTreeFilterBeginDrag )
		MSG_WM_MOUSEMOVE( OnMouseMove )
		MSG_WM_LBUTTONUP( OnLButtonUp )
		MESSAGE_HANDLER_EX( WM_DRAGDROPFINISH, OnDragDropFinish )

		COMMAND_ID_HANDLER_EX( IDC_BUTTON_ADDFILTER, OnAddFilter )
		COMMAND_ID_HANDLER_EX( IDC_BUTTON_DELETEFILTER, OnDeleteFilter )
		COMMAND_ID_HANDLER_EX( IDC_BUTTON_REFRESH, OnRefresh )
		COMMAND_ID_HANDLER_EX( IDC_BUTTON_IMPORTFROMPROXOMITRON, OnImportFromProxomitron )
		CHAIN_MSG_MAP( CDialogResize<CFilterManageWindow> )
	END_MSG_MAP()

	// LRESULT OnMessageHandlerEX(UINT uMsg, WPARAM wParam, LPARAM lParam)
	// void OnCommandIDHandlerEX(UINT uNotifyCode, int nID, CWindow wndCtl)

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnDestroy();
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);

	LRESULT OnTreeFilterClick(LPNMHDR pnmh);
	LRESULT OnCheckStateChanged(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnTreeFilterDblClk(LPNMHDR pnmh);	

	LRESULT OnTreeFilterBeginDrag(LPNMHDR pnmh);
	void OnMouseMove(UINT nFlags, CPoint point);
	void OnLButtonUp(UINT nFlags, CPoint point);
	LRESULT OnDragDropFinish(UINT uMsg, WPARAM wParam, LPARAM lParam);

	void OnAddFilter(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnDeleteFilter(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnRefresh(UINT uNotifyCode, int nID, CWindow wndCtl);

	void OnImportFromProxomitron(UINT uNotifyCode, int nID, CWindow wndCtl);

private:
	void _AddFilterDescriptor(CFilterDescriptor* filter);

	// Data members
	CTreeViewCtrl	m_treeFilter;
	HTREEITEM		m_htBeginDrag;
};