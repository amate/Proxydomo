/**
*	@file	FilterManageWindow.cpp
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
#include "stdafx.h"
#include "FilterManageWindow.h"
#include <memory>
#include <string>
#include <atlmisc.h>
#include <boost\property_tree\ptree.hpp>
#include <boost\property_tree\ini_parser.hpp>
#include "FilterDescriptor.h"
#include "FilterEditWindow.h"
#include "Settings.h"
#include "proximodo\util.h"
#include "Misc.h"
#include "CodeConvert.h"

using namespace CodeConvert;

///////////////////////////////////////////////////////////////
// CFilterManageWindow

CFilterManageWindow::CFilterManageWindow() : m_htBeginDrag(NULL)
{
}


BOOL CFilterManageWindow::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	m_treeFilter	= GetDlgItem(IDC_TREE_FILTER);
	// これがないと初回時のチェックが入らない…
	m_treeFilter.ModifyStyle( TVS_CHECKBOXES, 0 );
	m_treeFilter.ModifyStyle( 0, TVS_CHECKBOXES );

	//CImageList imagelist;
	//imagelist.Create(16, 16, ILC_COLOR32 | ILC_MASK, 2, 1);
	//m_treeFilter.SetImageList(imagelist);

    // ダイアログリサイズ初期化
    DlgResize_Init(true, true, WS_THICKFRAME | WS_MAXIMIZEBOX | WS_CLIPCHILDREN);

	CLogFont	lf;
	lf.SetMenuFont();
	m_treeFilter.SetFont(lf.CreateFontIndirect());
	HTREEITEM htRoot = m_treeFilter.InsertItem(_T("Root"), TVI_ROOT, TVI_FIRST);

	for (auto& filter : CSettings::s_vecpFilters) {
		HTREEITEM htItem = m_treeFilter.InsertItem(filter->title.c_str(), htRoot, TVI_LAST);
		m_treeFilter.SetItemData(htItem, (DWORD_PTR)filter.get());
	}
	PostMessage(WM_CHECKSTATECHANGED, NULL);

	m_treeFilter.Expand(htRoot);

	using namespace boost::property_tree;
	std::string settingsPath = CT2A(Misc::GetExeDirectory() + _T("settings.ini"));
	ptree pt;
	try {
		read_ini(settingsPath, pt);
	} catch (...) {
	}
	CRect rcWindow;
	rcWindow.top	= pt.get("FilterManageWindow.top", 0);
	rcWindow.left	= pt.get("FilterManageWindow.left", 0);
	rcWindow.right	= pt.get("FilterManageWindow.right", 0);
	rcWindow.bottom	= pt.get("FilterManageWindow.bottom", 0);
	if (rcWindow != CRect())
		MoveWindow(&rcWindow);

	return 0;
}

void CFilterManageWindow::OnDestroy()
{

	using namespace boost::property_tree;	
	std::string settingsPath = CT2A(Misc::GetExeDirectory() + _T("settings.ini"));
	ptree pt;
	try {
		read_ini(settingsPath, pt);
	} catch (...) {
	}

	CRect rcWindow;
	GetWindowRect(&rcWindow);
		
	pt.put("FilterManageWindow.top", rcWindow.top);
	pt.put("FilterManageWindow.left", rcWindow.left);
	pt.put("FilterManageWindow.right", rcWindow.right);
	pt.put("FilterManageWindow.bottom", rcWindow.bottom);

	write_ini(settingsPath, pt);
}

void CFilterManageWindow::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	OnRefresh(0, 0, NULL);

	ShowWindow(FALSE);
}

LRESULT CFilterManageWindow::OnTreeFilterClick(LPNMHDR pnmh)
{
	CPoint pt(::GetMessagePos());
	m_treeFilter.ScreenToClient(&pt);
	UINT flags = 0;
	HTREEITEM htHit = m_treeFilter.HitTest(pt, &flags);
	if (htHit == NULL || (flags & TVHT_ONITEMSTATEICON) == false)
		return 0;

	PostMessage(WM_CHECKSTATECHANGED, (WPARAM)htHit);
	m_treeFilter.SelectItem(htHit);
	return 0;
}

LRESULT CFilterManageWindow::OnCheckStateChanged(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (wParam == 0) {
		HTREEITEM htChild = m_treeFilter.GetChildItem(m_treeFilter.GetRootItem());
		while (htChild) {
			CFilterDescriptor* filter = (CFilterDescriptor*)m_treeFilter.GetItemData(htChild);
			if (filter)
				m_treeFilter.SetCheckState(htChild, filter->Active);
			htChild = m_treeFilter.GetNextSiblingItem(htChild);			
		}
	} else {
		HTREEITEM htHit = (HTREEITEM)wParam;
		CFilterDescriptor* filter = (CFilterDescriptor*)m_treeFilter.GetItemData(htHit);
		if (filter == nullptr)
			return 0;

		filter->Active = m_treeFilter.GetCheckState(htHit) != 0;
	}
	return 0;
}


LRESULT CFilterManageWindow::OnTreeFilterDblClk(LPNMHDR pnmh)
{
	CPoint pt(::GetMessagePos());
	m_treeFilter.ScreenToClient(&pt);
	UINT flags = 0;
	HTREEITEM htHit = m_treeFilter.HitTest(pt, &flags);
	if (htHit == NULL || (flags & TVHT_ONITEM) == false)
		return 0;

	HTREEITEM htRoot = m_treeFilter.GetRootItem();
	if (htRoot == htHit)
		return 0;
	
	CFilterDescriptor* filter = (CFilterDescriptor*)m_treeFilter.GetItemData(htHit);
	if (filter == nullptr)
		return 0;

	// フィルター編集ダイアログを開く
	CFilterEditWindow filterEdit(filter);
	if (filterEdit.DoModal(m_hWnd) == IDCANCEL) 
		return 0;

	m_treeFilter.SetItemText(htHit, filter->title.c_str());

	return 0;
}


LRESULT CFilterManageWindow::OnTreeFilterBeginDrag(LPNMHDR pnmh)
{
	LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)pnmh;
	HTREEITEM htDrag = pnmtv->itemNew.hItem;
	if (htDrag == m_treeFilter.GetRootItem())
		return 0;

	SetCapture();
	m_htBeginDrag = htDrag;
	return TRUE;
}

void CFilterManageWindow::OnMouseMove(UINT nFlags, CPoint point)
{
	if (GetCapture() != m_hWnd)
		return ;

	SetCursor(LoadCursor(0, IDC_ARROW));

	ClientToScreen(&point);
	CPoint ptScreen = point;
	m_treeFilter.ScreenToClient(&point);
	UINT flags = 0;
	HTREEITEM htHit = m_treeFilter.HitTest(point, &flags);
	if (htHit) {
		if (htHit == m_treeFilter.GetRootItem()) {
			m_treeFilter.RemoveInsertMark();
			SetCursor(LoadCursor(0, IDC_NO));
		} else {
			CRect rcItem;
			m_treeFilter.GetItemRect(htHit, &rcItem, FALSE);
			rcItem.bottom	-= (rcItem.Height() / 2);
			if (rcItem.PtInRect(point)) {
				m_treeFilter.SetInsertMark(htHit, FALSE);
			} else {
				m_treeFilter.SetInsertMark(htHit, TRUE);
			}
		}
	} else {
		m_treeFilter.RemoveInsertMark();
	}
}

void CFilterManageWindow::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (GetCapture() != m_hWnd)
		return ;

	SetCursor(LoadCursor(0, IDC_ARROW));
	m_treeFilter.RemoveInsertMark();
	::ReleaseCapture();
	::ShowCursor(TRUE);

	CPoint ptTree;
	::GetCursorPos(&ptTree);
	m_treeFilter.ScreenToClient(&ptTree);
	UINT flags = TVHT_ONITEM;	
	HTREEITEM htHit = m_treeFilter.HitTest(ptTree, &flags);
	if (htHit) {
		if (htHit == m_treeFilter.GetRootItem()) {
			m_htBeginDrag = NULL;
			return ;
		} else {
			CCritSecLock	lock(CSettings::s_csFilters);
			CFilterDescriptor* filterDrag = (CFilterDescriptor*)m_treeFilter.GetItemData(m_htBeginDrag);
			ATLASSERT( filterDrag );
			CFilterDescriptor* filterDrop = (CFilterDescriptor*)m_treeFilter.GetItemData(htHit);
			ATLASSERT( filterDrop );
			auto it = CSettings::s_vecpFilters.begin();
			for (; it != CSettings::s_vecpFilters.end(); ++it) {
				if (it->get() == filterDrop)
					break;
			}

			CRect rcItem;
			m_treeFilter.GetItemRect(htHit, &rcItem, FALSE);
			rcItem.bottom	-= (rcItem.Height() / 2);
			if (rcItem.PtInRect(ptTree) == false) {
				++it;
			}
			
			auto itInsert = CSettings::s_vecpFilters.insert(it, std::unique_ptr<CFilterDescriptor>(std::move(filterDrag)));
			for (auto it = CSettings::s_vecpFilters.begin(); it != CSettings::s_vecpFilters.end(); ++it) {
				if (it->get() == filterDrag && it != itInsert) {
					it->release();
					CSettings::s_vecpFilters.erase(it);
					break;
				}
			}
			// 全登録し直し
			m_treeFilter.DeleteAllItems();
			HTREEITEM htRoot = m_treeFilter.InsertItem(_T("Root"), TVI_ROOT, TVI_FIRST);
			for (auto& filter : CSettings::s_vecpFilters) {
				HTREEITEM htItem = m_treeFilter.InsertItem(
						filter->title.c_str(), htRoot, TVI_LAST);
				m_treeFilter.SetCheckState(htItem, filter->Active);
				m_treeFilter.SetItemData(htItem, (DWORD_PTR)filter.get());
			}
			m_treeFilter.Expand(htRoot);
		}
	}
	PostMessage(WM_DRAGDROPFINISH, (WPARAM)htHit);

}

LRESULT CFilterManageWindow::OnDragDropFinish(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//m_treeFilter.SelectDropTarget(NULL);
	//m_treeFilter.SelectItem((HTREEITEM)wParam);
	return 0;
}


void CFilterManageWindow::OnAddFilter(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	std::unique_ptr<CFilterDescriptor>	filter(new CFilterDescriptor());
	filter->title		=  L"new filter";

	CFilterEditWindow filterEdit(filter.get());
	if (filterEdit.DoModal(m_hWnd) == IDCANCEL) {
		return ;
	}
	_AddFilterDescriptor(filter.release());
}

void CFilterManageWindow::OnDeleteFilter(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	HTREEITEM htSel = m_treeFilter.GetSelectedItem();
	if (htSel == NULL || htSel == m_treeFilter.GetRootItem())
		return ;

	CCritSecLock	lock(CSettings::s_csFilters);
	auto filter = (CFilterDescriptor*)m_treeFilter.GetItemData(htSel);
	for (auto it = CSettings::s_vecpFilters.begin(); it != CSettings::s_vecpFilters.end(); ++it) {
		if (it->get() == filter) {
			CSettings::s_vecpFilters.erase(it);
			break;
		}
	}
	m_treeFilter.DeleteItem(htSel);
}

void CFilterManageWindow::OnRefresh(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	CSettings::SaveFilter();
}


void CFilterManageWindow::OnImportFromProxomitron(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	using std::string;

	std::string text = CT2A(Misc::GetClipboardText());
    // Process text: lines will be terminated by \n (even from Mac text files)
    // and multiline values will have \r for inner newlines. The text will end
    // by a [] line so that we don't have to process anything after the loop.
    string str = text + "\n\n";
    str = CUtil::replaceAll(str, "\r\n", "\n");
    str = CUtil::replaceAll(str, "\r",   "\n");
    str = CUtil::replaceAll(str, "\"\n        \"",  "\r");
    str = CUtil::replaceAll(str, "\"\n          \"",  "\r");
    int priority = 999;

    CFilterDescriptor d;
    bool isActive = false;

    size_t i = 0; 
	size_t max = str.size();
	size_t count = 0;
    while (i < max) {
        size_t j = str.find("\n", i);
        string line = str.substr(i, j - i);
        size_t eq = line.find('=');
        if (line.empty()) {
            d.TestValidity();
            if (!d.title.empty()) {
				if (isActive) {
					_AddFilterDescriptor(new CFilterDescriptor(d));
				}
                count++;
            }
            d.Clear();
            isActive = false;

        } else if (eq != string::npos) {
            string label = line.substr(0, eq);
            CUtil::trim(label);
            CUtil::upper(label);
            string value = line.substr(eq + 1);
            CUtil::trim(value);
            CUtil::trim(value, "\"");
            value = CUtil::replaceAll(value, "\r", "\r\n");

            if (label == "IN") {
                if (value == "TRUE") {
					d.filterType = CFilterDescriptor::kFilterHeadIn;
                    isActive = true;
                }
            }
            else if (label == "OUT") {
                if (value == "TRUE") {
					d.filterType = CFilterDescriptor::kFilterHeadOut;
                    isActive = true;
                }
            }
            else if (label == "ACTIVE") {
                if (value == "TRUE") {
                    isActive = true;
                }
            }
            else if (label == "KEY") {
                size_t colon = value.find(':');
                if (colon != string::npos) {
                    d.headerName = value.substr(0, colon);
                    d.title = UTF16fromUTF8(value.substr(colon + 1));
                    CUtil::trim(d.headerName);
                    CUtil::trim(d.title);
					if (d.filterType == CFilterDescriptor::kFilterText) {
                        CUtil::lower(value);
                        size_t i = value.find('(');
                        d.filterType = ((i != string::npos &&
                            value.find("out", i) != string::npos)
                            ? CFilterDescriptor::kFilterHeadOut : CFilterDescriptor::kFilterHeadIn);
                    }
                }
            }
            else if (label == "NAME") {
                d.title = UTF16fromUTF8(CUtil::trim(value));
                d.filterType = CFilterDescriptor::kFilterText;
            }
            else if (label == "MULTI") {
                d.multipleMatches = (value[0] == 'T');
            }
            else if (label == "LIMIT") {
				d.windowWidth = ::atoi(value.c_str());
            }
            else if (label == "URL"     ) d.urlPattern = CA2W(value.c_str());
            else if (label == "BOUNDS"  ) d.boundsPattern = CA2W(value.c_str());
            else if (label == "MATCH"   ) {
				//value = CUtil::replaceAll(value, "\r", "\r\n");
				d.matchPattern = CA2W(value.c_str());
			}
            else if (label == "REPLACE" ) d.replacePattern = CA2W(value.c_str());
        }
        i = j + 1;
    }
}


void CFilterManageWindow::_AddFilterDescriptor(CFilterDescriptor* filter)
{
	HTREEITEM htAdd = m_treeFilter.InsertItem(
		filter->title.c_str(), m_treeFilter.GetRootItem(), TVI_LAST);
	m_treeFilter.SetCheckState(htAdd, filter->Active);
	m_treeFilter.SetItemData(htAdd, (DWORD_PTR)filter);

	CCritSecLock	lock(CSettings::s_csFilters);
	CSettings::s_vecpFilters.push_back(std::unique_ptr<CFilterDescriptor>(std::move(filter)));
}





