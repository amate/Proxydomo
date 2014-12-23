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

namespace {

std::vector<std::unique_ptr<CFilterDescriptor>> GetFilterDescriptorFromClipboard()
{
	std::vector<std::unique_ptr<CFilterDescriptor>>	vecFilterDescriptor;

	using std::string;

	std::wstring text = Misc::GetClipboardText();
	// Process text: lines will be terminated by \n (even from Mac text files)
	// and multiline values will have \r for inner newlines. The text will end
	// by a [] line so that we don't have to process anything after the loop.
	std::wstring str = text + L"\n\n";
	str = CUtil::replaceAll(str, L"\r\n", L"\n");
	str = CUtil::replaceAll(str, L"\r", L"\n");
	str = CUtil::replaceAll(str, L"\"\n\"", L"\r");
	str = CUtil::replaceAll(str, L"\"\n        \"", L"\r");
	str = CUtil::replaceAll(str, L"\"\n          \"", L"\r");

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
				vecFilterDescriptor.emplace_back(std::make_unique<CFilterDescriptor>(d));

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
			} else if (label == L"OUT") {
				if (value == L"TRUE") {
					d.filterType = CFilterDescriptor::kFilterHeadOut;
					isActive = true;
				}
			} else if (label == L"ACTIVE") {
				if (value == L"TRUE") {
					isActive = true;
				}
			} else if (label == L"KEY") {
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
			} else if (label == L"NAME") {
				d.title = CUtil::trim(value);
				d.filterType = CFilterDescriptor::kFilterText;
			} else if (label == L"VERSION") {
				d.version = UTF8fromUTF16(value);
			} else if (label == L"AUTHOR") {
				d.author = UTF8fromUTF16(value);
			} else if (label == L"COMMENT") {
				d.comment = UTF8fromUTF16(value);
			} else if (label == L"MULTI") {
				d.multipleMatches = (value[0] == 'T');
			} else if (label == L"LIMIT") {
				d.windowWidth = ::_wtoi(value.c_str());
			} else if (label == L"URL")
				d.urlPattern = value.c_str();
			else if (label == L"BOUNDS")
				d.boundsPattern = value.c_str();
			else if (label == L"MATCH") {
				d.matchPattern = value.c_str();
			} else if (label == L"REPLACE")
				d.replacePattern = value.c_str();
		}
		i = j + 1;
	}
	return vecFilterDescriptor;
}

void	OutputFilterClipboardFormat(std::wstringstream& out, CFilterDescriptor* filter)
{
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
}

}	// namespace

///////////////////////////////////////////////////////////////
// CFilterManageWindow


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
			int iconIndex = filter->filterType == CFilterDescriptor::kFilterText ? kIconWebFilter : kIconHeaderFilter;
			m_treeFilter.SetItemImage(htItem, iconIndex, iconIndex);
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

void CFilterManageWindow::DlgResize_UpdateLayout(int cxWidth, int cyHeight)
{
	__super::DlgResize_UpdateLayout(cxWidth, cyHeight);
	CRect rcItem;
	m_toolBar.GetItemRect(m_toolBar.GetButtonCount() - 1, &rcItem);

	CRect rcTree;
	rcTree.top = rcItem.bottom + 2;
	rcTree.right = cxWidth;
	rcTree.bottom = cyHeight;
	m_treeFilter.MoveWindow(&rcTree);
}

#ifndef TVS_EX_EXCLUSIONCHECKBOXES
#define TVS_EX_EXCLUSIONCHECKBOXES  0x0100
#endif
#ifndef TVS_EX_DOUBLEBUFFER
#define TVS_EX_DOUBLEBUFFER         0x0004
#endif

BOOL CFilterManageWindow::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	m_treeFilter = GetDlgItem(IDC_TREE_FILTER);
	// これがないと初回時のチェックが入らない…
	m_treeFilter.ModifyStyle(TVS_CHECKBOXES, 0);
	m_treeFilter.ModifyStyle(0, TVS_CHECKBOXES);

	m_treeFilter.SetExtendedStyle(TVS_EX_EXCLUSIONCHECKBOXES | TVS_EX_DOUBLEBUFFER, TVS_EX_EXCLUSIONCHECKBOXES | TVS_EX_DOUBLEBUFFER);

	m_toolBar.Create(m_hWnd, CRect(0, 0, 400, 20), _T("ToolBar"),
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TBSTYLE_LIST | TBSTYLE_FLAT | TBSTYLE_WRAPABLE | CCS_NODIVIDER, 0, IDC_FILTERMANAGERTOOLBAR);
	m_toolBar.SetButtonStructSize();

	CImageList toolBarImageList;
	toolBarImageList.Create(20, 20, ILC_COLOR24 | ILC_MASK, 10, 2);
	CBitmap	toolBarBitmap;
	toolBarBitmap.LoadBitmap(IDB_FILTERMANAGERTOOLBAR);
	toolBarImageList.Add(toolBarBitmap, RGB(255, 0, 255));
	m_toolBar.SetImageList(toolBarImageList);
	TBBUTTON	tbns[] = {
		{ 0, IDC_BUTTON_ADDFILTER, TBSTATE_ENABLED, TBSTYLE_BUTTON | BTNS_SHOWTEXT | TBSTYLE_AUTOSIZE, {}, 0, (INT_PTR)L"新規フィルターを追加" },
		{ 1, IDC_BUTTON_DELETEFILTER, TBSTATE_ENABLED, TBSTYLE_BUTTON | BTNS_SHOWTEXT | TBSTYLE_AUTOSIZE, {}, 0, (INT_PTR)L"フィルターを削除" },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP },
		{ 2, IDC_BUTTON_CREATE_FOLDER, TBSTATE_ENABLED, TBSTYLE_BUTTON | BTNS_SHOWTEXT | TBSTYLE_AUTOSIZE, {}, 0, (INT_PTR)L"フォルダを作成" },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP },
		{ 3, IDC_BUTTON_IMPORTFROMPROXOMITRON, TBSTATE_ENABLED, TBSTYLE_BUTTON | BTNS_SHOWTEXT | TBSTYLE_AUTOSIZE, {}, 0, (INT_PTR)L"クリップボードからインポート" },
		{ 4, IDC_BUTTON_EXPORTTOPROXOMITRON, TBSTATE_ENABLED, TBSTYLE_BUTTON | BTNS_SHOWTEXT | TBSTYLE_AUTOSIZE, {}, 0, (INT_PTR)L"クリップボードへエクスポート" },


	};
	m_toolBar.AddButtons(_countof(tbns), tbns);

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
	tvImageList.Create(16, 16, ILC_MASK | ILC_COLOR32, 3, 1);

	CIcon	icoFolder;
	icoFolder.LoadIcon(IDI_FOLDER);
	tvImageList.AddIcon(icoFolder);

	CIcon	icoHeaderFilter;
	icoHeaderFilter.LoadIcon(IDI_HEADERFILTER);
	tvImageList.AddIcon(icoHeaderFilter);

	CIcon	icoWebFilter;
	icoWebFilter.LoadIcon(IDI_WEBFILTER);
	tvImageList.AddIcon(icoWebFilter);

	m_treeFilter.SetImageList(tvImageList);

	HTREEITEM htRoot = m_treeFilter.InsertItem(_T("Root"), kIconFolder, kIconFolder, TVI_ROOT, TVI_FIRST);
	m_treeFilter.SetCheckState(htRoot, true);
	m_treeFilter.SetItemState(htRoot, INDEXTOSTATEIMAGEMASK(0), TVIS_STATEIMAGEMASK);

	_AddTreeItem(htRoot, CSettings::s_vecpFilters);

	m_treeFilter.Expand(htRoot);

	// 位置を復元
	using namespace boost::property_tree;
	std::string settingsPath = CT2A(Misc::GetExeDirectory() + _T("settings.ini"));
	ptree pt;
	try {
		read_ini(settingsPath, pt);
	}
	catch (...) {
	}
	CRect rcWindow;
	rcWindow.top = pt.get("FilterManageWindow.top", 0);
	rcWindow.left = pt.get("FilterManageWindow.left", 0);
	rcWindow.right = pt.get("FilterManageWindow.right", 0);
	rcWindow.bottom = pt.get("FilterManageWindow.bottom", 0);
	if (rcWindow != CRect()) {
		MoveWindow(&rcWindow);
	}
	GetClientRect(&rcWindow);
	DlgResize_UpdateLayout(rcWindow.Width(), rcWindow.Height());

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

	if (CSettings::s_threadSaveFilter.joinable())
		CSettings::s_threadSaveFilter.join();
}

void CFilterManageWindow::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
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
	auto pFitlerItem = (FilterItem*)m_treeFilter.GetItemData(ptvdi->item.hItem);
	if (pFitlerItem == nullptr || ptvdi->item.pszText == nullptr || ptvdi->item.pszText[0] == L'\0')
		return 0;

	pFitlerItem->name = ptvdi->item.pszText;
	if (pFitlerItem->pFilter) {
		pFitlerItem->pFilter->title = ptvdi->item.pszText;
	}
	m_treeFilter.SetItem(&ptvdi->item);
	CSettings::SaveFilter();
	return 0;
}

// チェックを反転させる
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

// 右クリックメニューを表示
LRESULT CFilterManageWindow::OnTreeFilterRClick(LPNMHDR pnmh)
{
	CPoint pt(::GetMessagePos());
	m_treeFilter.ScreenToClient(&pt);
	UINT flags = 0;
	HTREEITEM htHit = m_treeFilter.HitTest(pt, &flags);
	if (htHit == NULL)
		return 0;

	m_treeFilter.SelectItem(htHit);

	FilterItem* filterItem = (FilterItem*)m_treeFilter.GetItemData(htHit);
	bool bFolder = filterItem == nullptr || filterItem->pvecpChildFolder != nullptr;

	enum { 
		kFilterEdit = 1,
		kAddFilter,
		kCreateFolder,
		kFilterDelete,
		kImportFilter,
		kExportFilter,

	};
	CMenu menu;
	menu.CreatePopupMenu();
	if (bFolder == false) {
		menu.AppendMenu(MF_STRING, kFilterEdit, _T("フィルターを編集する(&E)"));
		menu.AppendMenu(MF_SEPARATOR);
	}
	menu.AppendMenu(MF_STRING, kAddFilter,		_T("新規フィルターを追加する(&A)"));
	if (htHit != m_treeFilter.GetRootItem()) {
		menu.AppendMenu(MF_STRING, kFilterDelete,
			bFolder ? _T("フォルダを削除する(&D)") : _T("フィルターを削除する(&D)"));
	}
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, kCreateFolder,	_T("フォルダを作成する(&C)"));
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, kImportFilter,	_T("クリップボードからインポートする(&I)"));
	menu.AppendMenu(MF_STRING, kExportFilter,	_T("クリップボードへエクスポートする(&E)"));

	HTREEITEM htSel = htHit;
	HTREEITEM htParentFolder = m_treeFilter.GetParentItem(htHit);

	CPoint ptCursor;
	::GetCursorPos(&ptCursor);
	int ret = menu.TrackPopupMenu(TPM_RETURNCMD, ptCursor.x, ptCursor.y, m_hWnd);
	switch (ret) {
	case kFilterEdit:
		OnTreeFilterDblClk(nullptr);
		break;

	case kAddFilter:
		if (bFolder) {
			OnAddFilter(0, 0, NULL);
		} else {
			std::unique_ptr<CFilterDescriptor>	filter(new CFilterDescriptor());
			filter->title = L"new filter";

			CFilterEditWindow filterEdit(filter.get());
			if (filterEdit.DoModal(m_hWnd) == IDCANCEL) {
				return 0;
			}
			auto filterItem = std::make_unique<FilterItem>(std::move(filter));
			_InsertFilterItem(std::move(filterItem), htParentFolder, htSel);
			CSettings::SaveFilter();
		}
		break;

	case kFilterDelete:
		OnDeleteFilter(0, 0, NULL);
		break;

	case kCreateFolder:
		if (bFolder) {
			OnCreateFolder(0, 0, NULL);
		} else {
			std::unique_ptr<FilterItem> pfolder(new FilterItem);
			pfolder->active = true;
			pfolder->name = L"新しいフォルダ";
			pfolder->pvecpChildFolder.reset(new std::vector<std::unique_ptr<FilterItem>>);

			_InsertFilterItem(std::move(pfolder), htParentFolder, htSel);
			CSettings::SaveFilter();
		}
		break;

	case kImportFilter:
		if (bFolder) {
			OnImportFromProxomitron(0, 0, NULL);
		} else {
			auto vecFilterDescriptor = GetFilterDescriptorFromClipboard();
			if (vecFilterDescriptor.size() > 0) {
				for (auto& filterDescriptor : vecFilterDescriptor) {
					auto filterItem = std::make_unique<FilterItem>(std::move(filterDescriptor));
					_InsertFilterItem(std::move(filterItem), htParentFolder, htSel);
					htSel = m_treeFilter.GetSelectedItem();
				}
				CSettings::SaveFilter();
			}
		}
		break;

	case kExportFilter:
		OnExportToProxomitron(0, IDC_BUTTON_EXPORTTOPROXOMITRON, NULL);
		break;
	}
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
	CSettings::SaveFilter();
	return 0;
}

/// アイテムがダブルクリックされた
/// フィルター編集ウィンドウを開く
LRESULT CFilterManageWindow::OnTreeFilterDblClk(LPNMHDR pnmh)
{
	HTREEITEM htHit = m_treeFilter.GetSelectedItem();
	if (htHit == NULL)
		return 0;

	FilterItem* filterItem = (FilterItem*)m_treeFilter.GetItemData(htHit);
	if (filterItem == nullptr || filterItem->pFilter == nullptr)
		return 0;

	// フィルター編集ダイアログを開く
	CFilterEditWindow filterEdit(filterItem->pFilter.get());
	if (filterEdit.DoModal(m_hWnd) == IDCANCEL) 
		return 0;

	filterItem->name = filterItem->pFilter->title.c_str();
	m_treeFilter.SetItemText(htHit, filterItem->pFilter->title.c_str());
	int iconIndex = filterItem->pFilter->filterType == CFilterDescriptor::kFilterText ? kIconWebFilter : kIconHeaderFilter;
	m_treeFilter.SetItemImage(htHit, iconIndex, iconIndex);

	CSettings::SaveFilter();	

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

#if 0
	// ドラッグイメージを作成
	enum { kIconWidth = 20 };
	CRect rcItem;
	m_treeFilter.GetItemRect(htDrag, &rcItem, TRUE);
	rcItem.left -= kIconWidth;
	m_treeFilter.ClientToScreen(&rcItem);
	CWindowDC dcDesktop(NULL);

	CDC dcMem = CreateCompatibleDC(dcDesktop);
	CBitmap bmpItem = CreateCompatibleBitmap(dcDesktop, rcItem.Width(), rcItem.Height());
	HBITMAP hbmpPrev = dcMem.SelectBitmap(bmpItem);
	dcMem.BitBlt(0, 0, rcItem.Width(), rcItem.Height(), dcDesktop, rcItem.left, rcItem.top, SRCCOPY | CAPTUREBLT);
	dcMem.SelectBitmap(hbmpPrev);

	m_dragImage.Create(rcItem.Width(), rcItem.Height(), ILC_COLORDDB/* important! */, 1, 0);
	m_dragImage.Add(bmpItem);


	m_dragImage.BeginDrag(0, -25, 0);
	CPoint pt;
	::GetCursorPos(&pt);
	m_dragImage.DragEnter(::GetDesktopWindow(), pt);
#endif
	m_treeFilter.SelectItem(htDrag);
	m_treeFilter.UpdateWindow();
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

	//CPoint pt;
	//::GetCursorPos(&pt);
	//m_dragImage.DragMove(pt);

	ClientToScreen(&point);
	CPoint ptScreen = point;
	m_treeFilter.ScreenToClient(&point);
	UINT flags = 0;
	HTREEITEM htHit = m_treeFilter.HitTest(point, &flags);
	if (htHit) {
		if (htHit == m_htBeginDrag || _IsChildItem(m_htBeginDrag, htHit)) {
			SetCursor(LoadCursor(0, IDC_NO));
			m_treeFilter.RemoveInsertMark();
			//m_treeFilter.SelectDropTarget(NULL);
			return ;
		}
		
		FilterItem* filterItem = (FilterItem*)m_treeFilter.GetItemData(htHit);
		bool bFolder = filterItem == nullptr || filterItem->pvecpChildFolder;
		
		CRect rcItem;
		m_treeFilter.GetItemRect(htHit, &rcItem, FALSE);
		CRect rcItemTop = rcItem;
		if (bFolder) {
			rcItemTop.bottom = rcItemTop.top + kDropItemSpan;
		} else {
			rcItemTop.bottom -= (rcItem.Height() / 2);
		}
		CRect rcItemBottom = rcItem;
		if (bFolder) {
			rcItemBottom.top = rcItemBottom.bottom - kDropItemSpan;
		} else {
			rcItemBottom.top += (rcItem.Height() / 2);
		}
		if (htHit == m_treeFilter.GetRootItem()) {
			rcItemTop.SetRectEmpty();
			rcItemBottom.SetRectEmpty();
		}

		if (rcItemTop.PtInRect(point)) {
			m_treeFilter.SetInsertMark(htHit, FALSE);

		} else if (rcItemBottom.PtInRect(point)) {
			m_treeFilter.SetInsertMark(htHit, TRUE);

		} else if (bFolder) {	// フォルダー
			SetCursor(LoadCursor(0, IDC_CROSS));
			//m_treeFilter.Expand(htHit);
			m_treeFilter.RemoveInsertMark();
			HTREEITEM htFolder = m_treeFilter.GetDropHilightItem();
			if (htFolder != htHit) {
				//m_treeFilter.SelectDropTarget(htHit);
				SetTimer(kDragFolderExpandTimerId, kDragFolderExpandTimerInterval);
			}
			return;
		}
		//m_treeFilter.SelectDropTarget(NULL);
		KillTimer(kDragFolderExpandTimerId);

	} else {
		m_treeFilter.RemoveInsertMark();
		//m_treeFilter.SelectDropTarget(NULL);
		KillTimer(kDragFolderExpandTimerId);
	}
	SetCursor(LoadCursor(0, IDC_ARROW));
}


void CFilterManageWindow::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == kDragFolderExpandTimerId) {
		KillTimer(kDragFolderExpandTimerId);

		CPoint pt;
		::GetCursorPos(&pt);
		m_treeFilter.ScreenToClient(&pt);
		UINT flags = 0;
		HTREEITEM htHit = m_treeFilter.HitTest(pt, &flags);
		if (htHit == NULL)
			return;
		m_treeFilter.Expand(htHit);
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
	KillTimer(kDragFolderExpandTimerId);
	::ReleaseCapture();
	//m_dragImage.DragLeave(::GetDesktopWindow());
	//m_dragImage.EndDrag();
	//m_dragImage.Destroy();

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
		bool bFolder = filterItem == nullptr || filterItem->pvecpChildFolder;

		auto funcInsertTreeItem = 
			[this, &dragFilterItem](HTREEITEM htParent, HTREEITEM htInsertAfter) -> HTREEITEM 
		{
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
				icon = dragFilterItem->pFilter->filterType == CFilterDescriptor::kFilterText ? kIconWebFilter : kIconHeaderFilter;
			}
			// 追加
			HTREEITEM htInsert = m_treeFilter.InsertItem(name, icon, icon, htParent, htInsertAfter);
			m_treeFilter.SetCheckState(htInsert, active);
			return htInsert;
		};

		auto funcDropFolder = [&]() {
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
			ATLASSERT(m_htBeginDrag == NULL);

			// 親フォルダを見つける
			std::vector<std::unique_ptr<FilterItem>>* pvecFilter = nullptr;
			if (filterItem == nullptr) {
				pvecFilter = &CSettings::s_vecpFilters;
			} else {
				pvecFilter = filterItem->pvecpChildFolder.get();
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

			CSettings::SaveFilter();
		};

		// 挿入ポイントを見つける
		int nInsertPos = 0;
		HTREEITEM htParent = m_treeFilter.GetParentItem(htHit);
		HTREEITEM htItem = m_treeFilter.GetChildItem(htParent);
		HTREEITEM htInsert = NULL;
		do {
			if (htItem == htHit) {
				CRect rcItem;
				m_treeFilter.GetItemRect(htItem, &rcItem, FALSE);
				CRect rcItemTop = rcItem;
				if (bFolder) {
					rcItemTop.bottom = rcItemTop.top + kDropItemSpan;
				} else {
					rcItemTop.bottom -= (rcItem.Height() / 2);
				}
				CRect rcItemBottom = rcItem;
				if (bFolder) {
					rcItemBottom.top = rcItemBottom.bottom - kDropItemSpan;
				} else {
					rcItemBottom.top += (rcItem.Height() / 2);
				}
				if (htHit == m_treeFilter.GetRootItem()) {
					rcItemTop.SetRectEmpty();
					rcItemBottom.SetRectEmpty();
				}

				if (rcItemTop.PtInRect(ptTree)) {
					htItem = m_treeFilter.GetPrevSiblingItem(htItem);
					if (htItem == NULL)
						htItem = TVI_FIRST;
					htInsert = funcInsertTreeItem(htParent, htItem);

				} else if (rcItemBottom.PtInRect(ptTree)) {
					++nInsertPos;
					htInsert = funcInsertTreeItem(htParent, htItem);

				} else if (bFolder) {
					funcDropFolder();
					return;	// フォルダにドロップされた
				}
				break;
			}
			++nInsertPos;
		} while (htItem = m_treeFilter.GetNextSiblingItem(htItem));
	
		ATLASSERT(htInsert);
		// 追加
		std::vector<std::unique_ptr<FilterItem>>* pvecFilter = _GetParentFilterFolder(htHit);
		pvecFilter->insert(pvecFilter->begin() + nInsertPos, std::unique_ptr<FilterItem>(std::move(dragFilterItem)));
			
		// Drag元から消す
		int i = 0;
		for (auto it = m_pvecBeginDragParent->begin(); it != m_pvecBeginDragParent->end(); ++it, ++i) {
			if (pvecFilter == m_pvecBeginDragParent && i == nInsertPos)
				continue;	// フォルダ内移動で移動先が先にヒットしないようにする

			if (it->get() == dragFilterItem) {
				it->release();	// 解放されないようにする
				m_pvecBeginDragParent->erase(it);

				m_treeFilter.DeleteItem(m_htBeginDrag);
				m_htBeginDrag = NULL;
				m_pvecBeginDragParent = nullptr;
				break;
			}
		}
		ATLASSERT(m_htBeginDrag == NULL);
		m_treeFilter.SetItemData(htInsert, (DWORD_PTR)dragFilterItem);

		// 子アイテムは登録し直し
		if (dragFilterItem->pvecpChildFolder) 
			_AddTreeItem(htInsert, *dragFilterItem->pvecpChildFolder);

		m_treeFilter.SelectItem(htInsert);

		CSettings::SaveFilter();
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
	KillTimer(kDragFolderExpandTimerId);
	::ReleaseCapture();
	//m_dragImage.DragLeave(::GetDesktopWindow());
	//m_dragImage.EndDrag();
	//m_dragImage.Destroy();

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
	_AddFilterDescriptor(std::move(filter));
	CSettings::SaveFilter();
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
	CSettings::SaveFilter();
}

/// フォルダを作成する
void CFilterManageWindow::OnCreateFolder(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	std::unique_ptr<FilterItem> pfolder(new FilterItem);
	pfolder->active = true;
	pfolder->name = L"新しいフォルダ";
	pfolder->pvecpChildFolder.reset(new std::vector<std::unique_ptr<FilterItem>>);

	HTREEITEM htSel = m_treeFilter.GetSelectedItem();
	if (htSel == NULL || htSel == m_treeFilter.GetRootItem()) {
		_InsertFilterItem(std::move(pfolder), m_treeFilter.GetRootItem());
	} else {
		FilterItem*	pFilterItem = (FilterItem*)m_treeFilter.GetItemData(htSel);
		if (pFilterItem->pvecpChildFolder) {
			_InsertFilterItem(std::move(pfolder), htSel);
		} else {
			_InsertFilterItem(std::move(pfolder), m_treeFilter.GetParentItem(htSel));
		}
	}
	CSettings::SaveFilter();
}

/// クリップボードからフィルターをインポート
void CFilterManageWindow::OnImportFromProxomitron(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	auto vecFilterDescriptor = GetFilterDescriptorFromClipboard();
	if (vecFilterDescriptor.size() > 0) {
		for (auto& filterDescriptor : vecFilterDescriptor)
			_AddFilterDescriptor(std::move(filterDescriptor));

		CSettings::SaveFilter();
	}
}

/// クリップボードへフィルターをエクスポート
void CFilterManageWindow::OnExportToProxomitron(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	HTREEITEM htSel = m_treeFilter.GetSelectedItem();
	if (htSel == NULL)
		htSel = m_treeFilter.GetRootItem();

	std::vector<std::unique_ptr<FilterItem>>* pvecFilter = nullptr;

	auto pFitlerItem = (FilterItem*)m_treeFilter.GetItemData(htSel);
	if (pFitlerItem == nullptr) {
		pvecFilter = &CSettings::s_vecpFilters;
	} else if (pFitlerItem->pvecpChildFolder) {
		pvecFilter = pFitlerItem->pvecpChildFolder.get();
	}
	if (pvecFilter) {
		if (nID) {
			int ret = MessageBox(_T("フォルダ内のフィルターをすべてクリップボードにコピーします。\nよろしいですか？ (※フォルダ構造はコピーされません)"), _T("確認"), MB_ICONQUESTION | MB_OKCANCEL);
			if (ret == IDCANCEL)
				return;
		}

		std::wstringstream	out;
		std::function<void (std::vector<std::unique_ptr<FilterItem>>*)> funcAddFilterOutput;
		funcAddFilterOutput = [&](std::vector<std::unique_ptr<FilterItem>>* pvecFilter) {
			for (auto& filterItem : *pvecFilter) {
				if (filterItem->pvecpChildFolder) {
					funcAddFilterOutput(filterItem->pvecpChildFolder.get());
				} else {
					OutputFilterClipboardFormat(out, filterItem->pFilter.get());
				}
			}
		};
		funcAddFilterOutput(pvecFilter);
		Misc::SetClipboardText(out.str().c_str());

	} else {
		CFilterDescriptor* filter = pFitlerItem->pFilter.get();
		std::wstringstream	out;
		OutputFilterClipboardFormat(out, filter);
		Misc::SetClipboardText(out.str().c_str());
	}
}

// フィルターを追加する
// 現在の選択アイテムがフォルダならそのフォルダの末尾に追加する
// それ以外なら選択アイテムの親のフォルダの末尾に追加する
void CFilterManageWindow::_AddFilterDescriptor(std::unique_ptr<CFilterDescriptor>&& filter)
{
	auto filterItem = std::make_unique<FilterItem>(std::move(filter));
	HTREEITEM htSel = m_treeFilter.GetSelectedItem();
	if (htSel == NULL || htSel == m_treeFilter.GetRootItem()) {
		_InsertFilterItem(std::move(filterItem), m_treeFilter.GetRootItem());
	} else {
		FilterItem*	pFilterItem = (FilterItem*)m_treeFilter.GetItemData(htSel);
		if (pFilterItem->pvecpChildFolder) {
			// 選択アイテムはフォルダ
			_InsertFilterItem(std::move(filterItem), htSel);
		} else {
			// 選択アイテムはフィルタなので親を見る
			HTREEITEM htParent = m_treeFilter.GetParentItem(htSel);
			pFilterItem = (FilterItem*)m_treeFilter.GetItemData(htParent);
			ATLASSERT(pFilterItem == nullptr || pFilterItem->pvecpChildFolder);
			_InsertFilterItem(std::move(filterItem), htParent);
		}
	}
}

void CFilterManageWindow::_InsertFilterItem(std::unique_ptr<FilterItem>&& filterItem,
	HTREEITEM htParentFolder, HTREEITEM htInsertAfter /*= TVI_LAST*/)
{
	ATLASSERT(filterItem->name.GetLength());

	// 挿入先のフォルダを見つける
	std::vector<std::unique_ptr<FilterItem>>* pvecpFilter = nullptr;
	FilterItem*	pParentFilterItem = (FilterItem*)m_treeFilter.GetItemData(htParentFolder);
	if (pParentFilterItem == nullptr) {
		// root フォルダ
		pvecpFilter = &CSettings::s_vecpFilters;
	} else {
		// 選択アイテムはフォルダ
		pvecpFilter = pParentFilterItem->pvecpChildFolder.get();
	}
	ATLASSERT(pvecpFilter);

	// ツリーに追加
	const int iconIndex = filterItem->pvecpChildFolder ? kIconFolder :
	filterItem->pFilter->filterType == CFilterDescriptor::kFilterText ? kIconWebFilter : kIconHeaderFilter;

	HTREEITEM htNew = m_treeFilter.InsertItem(filterItem->name, iconIndex, iconIndex, 
												htParentFolder, htInsertAfter);
	m_treeFilter.SetItemData(htNew, (DWORD_PTR)filterItem.get());
	m_treeFilter.SetCheckState(htNew, filterItem->active);

	// 内部データに追加
	if (htInsertAfter != TVI_LAST) {
		FilterItem* pSelectFilterItem = (FilterItem*)m_treeFilter.GetItemData(htInsertAfter);
		ATLASSERT(pSelectFilterItem);
		CCritSecLock	lock(CSettings::s_csFilters);
		bool bFound = false;
		for (auto it = pvecpFilter->begin(); it != pvecpFilter->end(); ++it) {
			if (it->get() == pSelectFilterItem) {
				pvecpFilter->insert(std::next(it), std::move(filterItem));
				bFound = true;
				break;
			}
		}
		ATLASSERT(bFound);
	} else {
		CCritSecLock	lock(CSettings::s_csFilters);
		pvecpFilter->push_back(std::move(filterItem));
	}
	m_treeFilter.SelectItem(htNew);

	// CSettings::SaveFilter(); // 手動でやって
}



