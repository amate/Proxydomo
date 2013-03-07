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

enum { kIconFolder = 0, kIconFilter = 1, };

CFilterManageWindow::CFilterManageWindow() : m_htBeginDrag(NULL)
{
}

void CFilterManageWindow::_AddTreeItem(HTREEITEM htRoot, std::vector<std::unique_ptr<FilterItem>>& vecpFilter)
{
	for (auto& pFilterItem : vecpFilter) {
		if (pFilterItem->pFilter) {
			CFilterDescriptor* filter = pFilterItem->pFilter.get();
			HTREEITEM htItem = m_treeFilter.InsertItem(filter->title.c_str(), htRoot, TVI_LAST);
			m_treeFilter.SetItemData(htItem, (DWORD_PTR)pFilterItem.get());
			m_treeFilter.SetItemImage(htItem, kIconFilter, kIconFilter);
			m_treeFilter.SetCheckState(htItem, filter->Active);
		} else {
			HTREEITEM htItem = m_treeFilter.InsertItem(pFilterItem->name, htRoot, TVI_LAST);
			m_treeFilter.SetItemData(htItem, (DWORD_PTR)pFilterItem.get());
			m_treeFilter.SetItemImage(htItem, kIconFolder, kIconFolder);
			m_treeFilter.SetCheckState(htItem, pFilterItem->active);
			_AddTreeItem(htItem, *pFilterItem->pvecpChildFolder);
		}
	}
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

	// フォントを設定
	CLogFont	lf;
	lf.SetMenuFont();
	m_treeFilter.SetFont(lf.CreateFontIndirect());

	// アイコンを設定
	CImageList	tvImageList;
	tvImageList.Create(16, 16, ILC_MASK | ILC_COLOR32, 2, 1);
	CIcon	icoFolder;
	icoFolder.LoadIcon(IDI_FOLDER);
	tvImageList.AddIcon(icoFolder);
	CIcon	icoFilter;
	icoFilter.LoadIcon(IDI_FILTER);
	tvImageList.AddIcon(icoFilter);
	m_treeFilter.SetImageList(tvImageList);

	HTREEITEM htRoot = m_treeFilter.InsertItem(_T("Root"), kIconFolder, kIconFolder, TVI_ROOT, TVI_FIRST);
	m_treeFilter.SetCheckState(htRoot, true);

	_AddTreeItem(htRoot, CSettings::s_vecpFilters);

	m_treeFilter.Expand(htRoot);

	// 位置を復元
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

LRESULT CFilterManageWindow::OnTreeFilterBeginLabelEdit(LPNMHDR pnmh)
{
	auto ptvdi = (LPNMTVDISPINFO)pnmh;
	if (ptvdi->item.hItem == m_treeFilter.GetRootItem())	// Root は編集を禁止に
		return 1;	// Cancel
	return 0;
}

/// エディットラベルが編集された
LRESULT CFilterManageWindow::OnTreeFilterEndLabelEdit(LPNMHDR pnmh)
{
	auto ptvdi = (LPNMTVDISPINFO)pnmh;
	m_treeFilter.SetItem(&ptvdi->item);
	auto pFitlerItem = (FilterItem*)m_treeFilter.GetItemData(ptvdi->item.hItem);
	if (pFitlerItem == nullptr || ptvdi->item.pszText == nullptr)
		return 0;
	if (pFitlerItem->pFilter) {
		pFitlerItem->pFilter->title = ptvdi->item.pszText;
	} else {
		pFitlerItem->name = ptvdi->item.pszText;
	}
	return 0;
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
	HTREEITEM htHit = (HTREEITEM)wParam;
	FilterItem* filterItem = (FilterItem*)m_treeFilter.GetItemData(htHit);
	if (filterItem == nullptr)
		return 0;

	bool bActive = m_treeFilter.GetCheckState(htHit) != 0;
	if (filterItem->pFilter)
		filterItem->pFilter->Active = bActive;
	else
		filterItem->active = bActive;
	return 0;
}

/// アイテムがダブルクリックされた
/// フィルター編集ウィンドウを開く
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
	
	FilterItem* filterItem = (FilterItem*)m_treeFilter.GetItemData(htHit);
	if (filterItem == nullptr)
		return 0;

	if (filterItem->pFilter) {
		// フィルター編集ダイアログを開く
		CFilterEditWindow filterEdit(filterItem->pFilter.get());
		if (filterEdit.DoModal(m_hWnd) == IDCANCEL) 
			return 0;

		m_treeFilter.SetItemText(htHit, filterItem->pFilter->title.c_str());
	} else {

	}

	return 0;
}

/// htItem が所属する親フォルダのポインタを返す
std::vector<std::unique_ptr<FilterItem>>* CFilterManageWindow::_GetParentFilterFolder(HTREEITEM htItem)
{
	htItem = m_treeFilter.GetParentItem(htItem);
	std::vector<std::unique_ptr<FilterItem>>* pfolder = nullptr;
	FilterItem*	pFilterItem = (FilterItem*)m_treeFilter.GetItemData(htItem);
	if (pFilterItem == nullptr) {
		pfolder = &CSettings::s_vecpFilters;
	} else {
		pfolder = pFilterItem->pvecpChildFolder.get();
	}
	ATLASSERT( pfolder );
	return pfolder;
}

/// ドラッグを開始する
LRESULT CFilterManageWindow::OnTreeFilterBeginDrag(LPNMHDR pnmh)
{
	LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)pnmh;
	HTREEITEM htDrag = pnmtv->itemNew.hItem;
	if (htDrag == m_treeFilter.GetRootItem())
		return 0;

	SetCapture();
	m_htBeginDrag = htDrag;
	ATLASSERT( m_treeFilter.GetItemData(htDrag) != 0 );

	m_pvecBeginDragParent = _GetParentFilterFolder(htDrag);
	m_treeFilter.SelectItem(NULL);
	return TRUE;
}

bool	CFilterManageWindow::_IsChildItem(HTREEITEM htParent, HTREEITEM htItem)
{
	HTREEITEM htCur = m_treeFilter.GetChildItem(htParent);
	do {
		if (htCur == htItem)
			return true;
		if (m_treeFilter.ItemHasChildren(htCur)) {
			if (_IsChildItem(htCur, htItem))
				return true;
		}
	} while (htCur = m_treeFilter.GetNextSiblingItem(htCur));
	return false;
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
		if (htHit == m_htBeginDrag || _IsChildItem(m_htBeginDrag, htHit)) {
			SetCursor(LoadCursor(0, IDC_NO));
			m_treeFilter.RemoveInsertMark();
			m_treeFilter.SelectDropTarget(NULL);
			return ;
		}
		
		FilterItem* filterItem = (FilterItem*)m_treeFilter.GetItemData(htHit);
		// フォルダー
		if (filterItem == nullptr || filterItem->pvecpChildFolder) {
			m_treeFilter.Expand(htHit);
			m_treeFilter.RemoveInsertMark();
			m_treeFilter.SelectDropTarget(htHit);
		} else {
			CRect rcItem;
			m_treeFilter.GetItemRect(htHit, &rcItem, FALSE);
			rcItem.bottom	-= (rcItem.Height() / 2);
			if (rcItem.PtInRect(point)) {
				m_treeFilter.SetInsertMark(htHit, FALSE);
			} else {
				m_treeFilter.SetInsertMark(htHit, TRUE);
			}
			m_treeFilter.SelectDropTarget(NULL);
		}

	} else {
		m_treeFilter.RemoveInsertMark();
		m_treeFilter.SelectDropTarget(NULL);
	}
}

/// ドロップされた
void CFilterManageWindow::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (GetCapture() != m_hWnd)
		return ;

	SetCursor(LoadCursor(0, IDC_ARROW));
	m_treeFilter.RemoveInsertMark();
	m_treeFilter.SelectDropTarget(NULL);
	::ReleaseCapture();

	CPoint ptTree;
	::GetCursorPos(&ptTree);
	m_treeFilter.ScreenToClient(&ptTree);
	UINT flags = TVHT_ONITEM;	
	HTREEITEM htHit = m_treeFilter.HitTest(ptTree, &flags);
	if (htHit) {
		if (htHit == m_htBeginDrag || _IsChildItem(m_htBeginDrag, htHit))
			return ;

		CCritSecLock	lock(CSettings::s_csFilters);
		FilterItem* filterItem = (FilterItem*)m_treeFilter.GetItemData(htHit);
		FilterItem* dragFilterItem = (FilterItem*)m_treeFilter.GetItemData(m_htBeginDrag);
		auto funcInsertTreeItem = [this, &dragFilterItem](HTREEITEM htParent, HTREEITEM htInsertAfter) -> HTREEITEM {
			CString name;
			bool	active;
			int		icon;
			if (dragFilterItem->pvecpChildFolder) {
				name = dragFilterItem->name;
				active = dragFilterItem->active;
				icon = kIconFolder;
			} else {
				name = dragFilterItem->pFilter->title.c_str();
				active = dragFilterItem->pFilter->Active;
				icon = kIconFilter;
			}
			// 追加
			HTREEITEM htInsert = m_treeFilter.InsertItem(name, icon, icon, htParent, htInsertAfter);
			m_treeFilter.SetCheckState(htInsert, active);
			return htInsert;
		};
		// フォルダーにドロップされた
		if (filterItem == nullptr || filterItem->pvecpChildFolder) {
			std::vector<std::unique_ptr<FilterItem>>* pvecFilter;
			if (filterItem == nullptr)
				pvecFilter = &CSettings::s_vecpFilters;
			else
				pvecFilter = filterItem->pvecpChildFolder.get();

			// Drag元から消す
			for (auto it = m_pvecBeginDragParent->begin(); it != m_pvecBeginDragParent->end(); ++it) {
				if (it->get() == dragFilterItem) {
					it->release();
					m_pvecBeginDragParent->erase(it);

					m_treeFilter.DeleteItem(m_htBeginDrag);
					m_htBeginDrag = NULL;
					m_pvecBeginDragParent = nullptr;
					break;
				}
			}
			// 追加
			HTREEITEM htInsert = funcInsertTreeItem(htHit, TVI_LAST);
			m_treeFilter.SetItemData(htInsert, (DWORD_PTR)dragFilterItem);
			pvecFilter->push_back(std::unique_ptr<FilterItem>(std::move(dragFilterItem)));

			// 子アイテムは登録し直し
			if (dragFilterItem->pvecpChildFolder) 
				_AddTreeItem(htInsert, *dragFilterItem->pvecpChildFolder);

			m_treeFilter.Expand(htHit);
			m_treeFilter.SelectItem(htInsert);
		} else {
			// 挿入ポイントを見つける
			int nInsertPos = 0;
			HTREEITEM htParent = m_treeFilter.GetParentItem(htHit);
			HTREEITEM htItem = m_treeFilter.GetChildItem(htParent);
			HTREEITEM htInsert = NULL;
			do {
				CRect rcItem;
				m_treeFilter.GetItemRect(htItem, &rcItem, FALSE);
				if (rcItem.PtInRect(ptTree)) {
					rcItem.bottom	-= (rcItem.Height() / 2);
					if (rcItem.PtInRect(ptTree) == false) {
						++nInsertPos;
						htInsert = funcInsertTreeItem(htParent, htItem);
					} else {
						htItem = m_treeFilter.GetPrevSiblingItem(htItem);
						if (htItem == NULL)
							htItem = TVI_FIRST;
						htInsert = funcInsertTreeItem(htParent, htItem);
					}
					break;					
				}
				++nInsertPos;
			} while (htItem = m_treeFilter.GetNextSiblingItem(htItem));
	
			// 追加
			std::vector<std::unique_ptr<FilterItem>>* pvecFilter = _GetParentFilterFolder(htHit);
			pvecFilter->insert(pvecFilter->begin() + nInsertPos, std::unique_ptr<FilterItem>(std::move(dragFilterItem)));
			
			// Drag元から消す
			for (auto it = m_pvecBeginDragParent->begin(); it != m_pvecBeginDragParent->end(); ++it) {
				if (it->get() == dragFilterItem) {
					it->release();
					m_pvecBeginDragParent->erase(it);

					m_treeFilter.DeleteItem(m_htBeginDrag);
					m_htBeginDrag = NULL;
					m_pvecBeginDragParent = nullptr;
					break;
				}
			}
			m_treeFilter.SetItemData(htInsert, (DWORD_PTR)dragFilterItem);

			// 子アイテムは登録し直し
			if (dragFilterItem->pvecpChildFolder) 
				_AddTreeItem(htInsert, *dragFilterItem->pvecpChildFolder);

			m_treeFilter.SelectItem(htInsert);
		}
	}

}

/// ドラッグ中ならキャンセルする
void CFilterManageWindow::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (GetCapture() != m_hWnd)
		return ;

	SetCursor(LoadCursor(0, IDC_ARROW));
	m_treeFilter.RemoveInsertMark();
	m_treeFilter.SelectDropTarget(NULL);
	::ReleaseCapture();

	m_htBeginDrag = NULL;
	m_pvecBeginDragParent = nullptr;
}

/// 新しいフィルターを追加する
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

/// 選択されたフィルターを削除する
void CFilterManageWindow::OnDeleteFilter(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	HTREEITEM htSel = m_treeFilter.GetSelectedItem();
	if (htSel == NULL || htSel == m_treeFilter.GetRootItem())
		return ;

	// 万が一に備えてクリップボードに退避させておく
	OnExportToProxomitron(0, 0, NULL);

	CCritSecLock	lock(CSettings::s_csFilters);
	auto filterItem = (FilterItem*)m_treeFilter.GetItemData(htSel);
	if (filterItem->pvecpChildFolder) {
		int ret = MessageBox(_T("フォルダ内のフィルターはすべて削除されます。\nよろしいですか？"), _T("確認"), MB_ICONWARNING | MB_OKCANCEL);
		if (ret == IDCANCEL)
			return ;
	}
	std::function<void (std::vector<std::unique_ptr<FilterItem>>*)> funcSearchAndDestroy;
	funcSearchAndDestroy = [this, &funcSearchAndDestroy, &filterItem, htSel]
												(std::vector<std::unique_ptr<FilterItem>>* pvecpFilter) {
		for (auto it = pvecpFilter->begin(); it != pvecpFilter->end(); ++it) {
			if (it->get() == filterItem) {
				pvecpFilter->erase(it);
				m_treeFilter.DeleteItem(htSel);
				return ;
			}
			if (it->get()->pvecpChildFolder)
				funcSearchAndDestroy(it->get()->pvecpChildFolder.get());
		}
	};
	funcSearchAndDestroy(&CSettings::s_vecpFilters);
}

/// フィルターを保存する
void CFilterManageWindow::OnRefresh(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	CSettings::SaveFilter();
}

/// フォルダを作成する
void CFilterManageWindow::OnCreateFolder(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	HTREEITEM htSel = m_treeFilter.GetSelectedItem();
	if (htSel == NULL)
		htSel = m_treeFilter.GetRootItem();

	std::vector<std::unique_ptr<FilterItem>>* pvecpFilter = nullptr;
	do {
		FilterItem*	pFilterItem = (FilterItem*)m_treeFilter.GetItemData(htSel);
		if (pFilterItem == nullptr) {
			pvecpFilter = &CSettings::s_vecpFilters;
		} else if (pFilterItem->pvecpChildFolder == nullptr) {
			htSel = m_treeFilter.GetParentItem(htSel);
		} else {
			pvecpFilter = pFilterItem->pvecpChildFolder.get();
		}
	} while (pvecpFilter == nullptr);
	ATLASSERT( pvecpFilter );

	std::unique_ptr<FilterItem> pfolder(new FilterItem);
	pfolder->active	= true;
	pfolder->name	= L"新しいフォルダ";
	pfolder->pvecpChildFolder.reset(new std::vector<std::unique_ptr<FilterItem>>);

	HTREEITEM htNew = m_treeFilter.InsertItem(pfolder->name, kIconFolder, kIconFolder, htSel, TVI_LAST);
	m_treeFilter.SetItemData(htNew, (DWORD_PTR)pfolder.get());
	m_treeFilter.SetCheckState(htNew, pfolder->active);

	CCritSecLock	lock(CSettings::s_csFilters);
	pvecpFilter->push_back(std::move(pfolder));
	m_treeFilter.SelectItem(htNew);
}

/// クリップボードからフィルターをインポート
void CFilterManageWindow::OnImportFromProxomitron(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	using std::string;

	std::wstring text = Misc::GetClipboardText();
    // Process text: lines will be terminated by \n (even from Mac text files)
    // and multiline values will have \r for inner newlines. The text will end
    // by a [] line so that we don't have to process anything after the loop.
    std::wstring str = text + L"\n\n";
    str = CUtil::replaceAll(str, L"\r\n", L"\n");
    str = CUtil::replaceAll(str, L"\r",   L"\n");
	str = CUtil::replaceAll(str, L"\"\n\"", L"\r");
    str = CUtil::replaceAll(str, L"\"\n        \"",  L"\r");
    str = CUtil::replaceAll(str, L"\"\n          \"",  L"\r");

    CFilterDescriptor d;
    bool isActive = false;

    size_t i = 0; 
	size_t max = str.size();
	size_t count = 0;
    while (i < max) {
        size_t j = str.find(L"\n", i);
        std::wstring line = str.substr(i, j - i);
        size_t eq = line.find(L'=');
        if (line.empty()) {
            d.TestValidity();
            if (!d.title.empty()) {
				d.Active = isActive;
				// フィルターを追加
				_AddFilterDescriptor(new CFilterDescriptor(d));

                count++;
            }
            d.Clear();
            isActive = false;

        } else if (eq != wstring::npos) {
            std::wstring label = line.substr(0, eq);
            CUtil::trim(label);
            CUtil::upper(label);
            std::wstring value = line.substr(eq + 1);
            CUtil::trim(value);
            CUtil::trim(value, L"\"");
            value = CUtil::replaceAll(value, L"\r", L"\r\n");

            if (label == L"IN") {
                if (value == L"TRUE") {
					d.filterType = CFilterDescriptor::kFilterHeadIn;
                    isActive = true;
                }
            }
            else if (label == L"OUT") {
                if (value == L"TRUE") {
					d.filterType = CFilterDescriptor::kFilterHeadOut;
                    isActive = true;
                }
            }
            else if (label == L"ACTIVE") {
                if (value == L"TRUE") {
                    isActive = true;
                }
            }
            else if (label == L"KEY") {
                size_t colon = value.find(':');
                if (colon != string::npos) {
                    d.headerName = UTF8fromUTF16(value.substr(0, colon));
                    d.title = value.substr(colon + 1);
                    CUtil::trim(d.headerName);
                    CUtil::trim(d.title);
					if (d.filterType == CFilterDescriptor::kFilterText) {
                        CUtil::lower(value);
                        size_t i = value.find(L'(');
                        d.filterType = ((i != string::npos &&
                            value.find(L"out", i) != string::npos)
                            ? CFilterDescriptor::kFilterHeadOut : CFilterDescriptor::kFilterHeadIn);
                    }
                }
            }
            else if (label == L"NAME") {
                d.title = CUtil::trim(value);
                d.filterType = CFilterDescriptor::kFilterText;
            }
			else if (label == L"VERSION") {
				d.version = UTF8fromUTF16(value);
			}
			else if (label == L"AUTHOR") {
				d.author = UTF8fromUTF16(value);
			}
			else if (label == L"COMMENT") {
				d.comment = UTF8fromUTF16(value);
			}
            else if (label == L"MULTI") {
                d.multipleMatches = (value[0] == 'T');
            }
            else if (label == L"LIMIT") {
				d.windowWidth = ::_wtoi(value.c_str());
            }
            else if (label == L"URL"     ) 
				d.urlPattern = value.c_str();
            else if (label == L"BOUNDS"  ) 
				d.boundsPattern = value.c_str();
            else if (label == L"MATCH"   ) {
				d.matchPattern = value.c_str();
			}
            else if (label == L"REPLACE" ) 
				d.replacePattern = value.c_str();
        }
        i = j + 1;
    }
}

/// クリップボードへフィルターをエクスポート
void CFilterManageWindow::OnExportToProxomitron(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	HTREEITEM htSel = m_treeFilter.GetSelectedItem();
	if (htSel == NULL || htSel == m_treeFilter.GetRootItem())
		return ;

	auto pFitlerItem = (FilterItem*)m_treeFilter.GetItemData(htSel);
	if (pFitlerItem->pvecpChildFolder)
		return ;

	CFilterDescriptor* filter = pFitlerItem->pFilter.get();
	std::wstringstream	out;
	if (filter->filterType == CFilterDescriptor::kFilterText) {
		out << L"[Patterns]\r\n";
		out << L"Name = \"" << filter->title << L"\"\r\n";
	} else {
		out << L"[HTTP headers]\r\n";
		out << L"Key = \"" << UTF16fromUTF8(filter->headerName) << L": " << filter->title << L"\"\r\n";
		out << L"In = " << 
			(filter->filterType == CFilterDescriptor::kFilterHeadIn ? L"TRUE" : L"FALSE") << L"\r\n";
		out << L"Out = " << 
			(filter->filterType == CFilterDescriptor::kFilterHeadOut ? L"TRUE" : L"FALSE") << L"\r\n";
	}	
	out << L"Version = \"" << UTF16fromUTF8(filter->version) << "\"\r\n";
	out << L"Author = \"" << UTF16fromUTF8(filter->author) << L"\"\r\n";
	out << L"Comment = \"" << UTF16fromUTF8(filter->comment) << L"\"\r\n";

	out << L"Active = " << (filter->Active ? L"TRUE" : L"FALSE") << L"\r\n";
	out << L"Multi = " << (filter->multipleMatches ? L"TRUE" : L"FALSE") << L"\r\n";
	out << L"URL = \"" << filter->urlPattern << L"\"\r\n";
	out << L"Bounds = \"" << filter->boundsPattern << L"\"\r\n";
	out << L"Limit = " << filter->windowWidth << L"\r\n";

	auto funcAddMultiLine = [&](const std::wstring& name, const std::wstring& pattern) {
		std::wstring temp = CUtil::replaceAll(pattern, L"\r\n", L"\n") + _T("\n");		
		std::wstringstream	ssPattern(temp);
		std::wstring strLine;
		bool bFirst = true;
		while (std::getline(ssPattern, strLine).good()) {
			if (bFirst) {
				out << name << L" = \"" << strLine << L"\"\r\n";
				bFirst = false;
			} else {
				out << L"        \"" << strLine << L"\"\r\n";
			}
		}
	};
	funcAddMultiLine(L"Match", filter->matchPattern);
	funcAddMultiLine(L"Replace", filter->replacePattern);

	out << L"\r\n";

	Misc::SetClipboardText(out.str().c_str());
}


void CFilterManageWindow::_AddFilterDescriptor(CFilterDescriptor* filter)
{
	HTREEITEM htSel = m_treeFilter.GetSelectedItem();
	if (htSel == NULL)
		htSel = m_treeFilter.GetRootItem();

	std::vector<std::unique_ptr<FilterItem>>* pvecpFilter = nullptr;
	do {
		FilterItem*	pFilterItem = (FilterItem*)m_treeFilter.GetItemData(htSel);
		if (pFilterItem == nullptr) {
			pvecpFilter = &CSettings::s_vecpFilters;
		} else if (pFilterItem->pvecpChildFolder == nullptr) {
			htSel = m_treeFilter.GetParentItem(htSel);
		} else {
			pvecpFilter = pFilterItem->pvecpChildFolder.get();
		}
	} while (pvecpFilter == nullptr);
	ATLASSERT( pvecpFilter );

	std::unique_ptr<FilterItem> pfilterItem(new FilterItem);
	pfilterItem->pFilter.reset(std::move(filter));

	HTREEITEM htNew = m_treeFilter.InsertItem(pfilterItem->pFilter->title.c_str(), kIconFilter, kIconFilter, htSel, TVI_LAST);
	m_treeFilter.SetItemData(htNew, (DWORD_PTR)pfilterItem.get());
	m_treeFilter.SetCheckState(htNew, pfilterItem->pFilter->Active);

	CCritSecLock	lock(CSettings::s_csFilters);
	pvecpFilter->push_back(std::move(pfilterItem));
	m_treeFilter.SelectItem(htNew);
}





