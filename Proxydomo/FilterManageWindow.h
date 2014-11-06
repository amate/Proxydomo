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
#include "Settings.h"

class CFilterDescriptor;

class CFilterManageWindow : 
	public CDialogImpl<CFilterManageWindow>,
	public CDialogResize<CFilterManageWindow>
{
public:
	enum { IDD = IDD_FILTERMANAGE };

	enum { 
		WM_CHECKSTATECHANGED = WM_APP + 1,

		kDragFolderExpandTimerId = 1,
		kDragFolderExpandTimerInterval = 400,
	};

	enum { kIconFolder = 0, kIconHeaderFilter = 1, kIconWebFilter = 2, };


	CFilterManageWindow();

	// overrides
	void DlgResize_UpdateLayout(int cxWidth, int cyHeight);


	BEGIN_DLGRESIZE_MAP( CFilterManageWindow )
		DLGRESIZE_CONTROL( IDC_TREE_FILTER, DLSZ_SIZE_X | DLSZ_SIZE_Y )
		DLGRESIZE_CONTROL( IDC_FILTERMANAGERTOOLBAR, DLSZ_SIZE_X | DLSZ_SIZE_Y)
	END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP_EX( CFilterManageWindow )
		MSG_WM_INITDIALOG( OnInitDialog )
		MSG_WM_DESTROY( OnDestroy )
		COMMAND_ID_HANDLER_EX( IDCANCEL, OnCancel )
		NOTIFY_HANDLER_EX( IDC_TREE_FILTER, NM_CLICK, OnTreeFilterClick )
		NOTIFY_HANDLER_EX( IDC_TREE_FILTER, NM_RCLICK, OnTreeFilterRClick)
		NOTIFY_HANDLER_EX( IDC_TREE_FILTER, TVN_BEGINLABELEDIT, OnTreeFilterBeginLabelEdit)
		NOTIFY_HANDLER_EX( IDC_TREE_FILTER, TVN_ENDLABELEDIT, OnTreeFilterEndLabelEdit )
		MESSAGE_HANDLER_EX( WM_CHECKSTATECHANGED, OnCheckStateChanged )
		NOTIFY_HANDLER_EX( IDC_TREE_FILTER, NM_DBLCLK, OnTreeFilterDblClk )		
		NOTIFY_HANDLER_EX( IDC_TREE_FILTER, TVN_BEGINDRAG, OnTreeFilterBeginDrag )
		MSG_WM_MOUSEMOVE( OnMouseMove )
		MSG_WM_TIMER( OnTimer )
		MSG_WM_LBUTTONUP( OnLButtonUp )
		MSG_WM_RBUTTONDOWN( OnRButtonDown )

		COMMAND_ID_HANDLER_EX( IDC_BUTTON_ADDFILTER, OnAddFilter )
		COMMAND_ID_HANDLER_EX( IDC_BUTTON_DELETEFILTER, OnDeleteFilter )
		COMMAND_ID_HANDLER_EX( IDC_BUTTON_CREATE_FOLDER, OnCreateFolder )
		COMMAND_ID_HANDLER_EX( IDC_BUTTON_IMPORTFROMPROXOMITRON, OnImportFromProxomitron )
		COMMAND_ID_HANDLER_EX( IDC_BUTTON_EXPORTTOPROXOMITRON , OnExportToProxomitron )
		CHAIN_MSG_MAP( CDialogResize<CFilterManageWindow> )
	END_MSG_MAP()

	// LRESULT OnMessageHandlerEX(UINT uMsg, WPARAM wParam, LPARAM lParam)
	// void OnCommandIDHandlerEX(UINT uNotifyCode, int nID, CWindow wndCtl)

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnDestroy();
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);

	LRESULT OnTreeFilterClick(LPNMHDR pnmh);
	LRESULT OnTreeFilterRClick(LPNMHDR pnmh);
	LRESULT OnTreeFilterBeginLabelEdit(LPNMHDR pnmh);
	LRESULT OnTreeFilterEndLabelEdit(LPNMHDR pnmh);
	LRESULT OnCheckStateChanged(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnTreeFilterDblClk(LPNMHDR pnmh);	

	LRESULT OnTreeFilterBeginDrag(LPNMHDR pnmh);
	void OnMouseMove(UINT nFlags, CPoint point);
	void OnTimer(UINT_PTR nIDEvent);
	void OnLButtonUp(UINT nFlags, CPoint point);
	void OnRButtonDown(UINT nFlags, CPoint point);

	void OnAddFilter(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnDeleteFilter(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnCreateFolder(UINT uNotifyCode, int nID, CWindow wndCtl);

	void OnImportFromProxomitron(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnExportToProxomitron(UINT uNotifyCode, int nID, CWindow wndCtl);

private:
	void _AddTreeItem(HTREEITEM htRoot, std::vector<std::unique_ptr<FilterItem>>& vecpFilter);
	void _AddFilterDescriptor(std::unique_ptr<CFilterDescriptor>&& filter);
	void _InsertFilterItem(std::unique_ptr<FilterItem>&& filterItem, 
							HTREEITEM htParentFolder, HTREEITEM htInsertAfter = TVI_LAST);
	std::vector<std::unique_ptr<FilterItem>>* _GetParentFilterFolder(HTREEITEM htItem);
	bool	_IsChildItem(HTREEITEM htParent, HTREEITEM htItem);

	// Data members
	CToolBarCtrl	m_toolBar;
	CTreeViewCtrl	m_treeFilter;

	HTREEITEM		m_htBeginDrag;
	//CImageList		m_dragImage;
	std::vector<std::unique_ptr<FilterItem>>*	m_pvecBeginDragParent;	// for delete
};