/**
 *	@file	Misc.h
 *	@biref	わりと汎用的な雑多なルーチン群
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
#ifndef MISC_H
#define MISC_H

#pragma once

#include <stdarg.h>
#include <io.h>
#include <vector>
#include <list>
#include <algorithm>

#include <atlstr.h>
#include <atlapp.h>
#define _WTL_NO_CSTRING
#include <atlmisc.h>

template <class _Function>
bool ForEachFile(const CString &strDirectoryPath, _Function __f)
{
	CString 		strPathFind = strDirectoryPath;

	::PathAddBackslash(strPathFind.GetBuffer(MAX_PATH));
	strPathFind.ReleaseBuffer();

	CString 		strPath = strPathFind;
	strPathFind += _T("*.*");

	WIN32_FIND_DATA wfd;
	HANDLE	h = ::FindFirstFile(strPathFind, &wfd);
	if (h == INVALID_HANDLE_VALUE)
		return false;

	// Now scan the directory
	do {
		// it is a file
		if ((wfd.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) == 0) {
			__f(strPath + wfd.cFileName);
		}
	} while (::FindNextFile(h, &wfd));

	::FindClose(h);

	return true;
}

template <class _Function>
bool ForEachFileFolder(const CString &strDirectoryPath, _Function __f)
{
	CString 		strPathFind = strDirectoryPath;

	::PathAddBackslash(strPathFind.GetBuffer(MAX_PATH));
	strPathFind.ReleaseBuffer();

	CString 		strPath = strPathFind;
	strPathFind += _T("*.*");

	WIN32_FIND_DATA wfd;
	HANDLE	h = ::FindFirstFile(strPathFind, &wfd);
	if (h == INVALID_HANDLE_VALUE)
		return false;
	
	std::list<std::pair<std::wstring, bool>> vecFileFolder;
	// Now scan the directory
	do {
		if (::lstrcmp(wfd.cFileName, _T(".")) == 0 || ::lstrcmp(wfd.cFileName, _T("..")) == 0)
			continue;

		if ((wfd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) == 0) {
			vecFileFolder.emplace_back(wfd.cFileName, (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
		}
	} while (::FindNextFile(h, &wfd));

	::FindClose(h);

	vecFileFolder.sort([](const std::pair<std::wstring, bool>& first, const std::pair<std::wstring, bool>& second) -> bool {
		return ::StrCmpLogicalW(first.first.c_str(), second.first.c_str()) < 0;
	});
	for (auto& pair : vecFileFolder)
		__f(strPath + pair.first.c_str(), pair.second);

	return true;
}


namespace Misc {

/// クリップボードにあるテキストを取得する
CString GetClipboardText(bool bUseOLE = false);
bool	SetClipboardText(const CString& str);

// ==========================================================================

//+++ ファイルパス名より、ファイル名を取得
const CString	GetFileBaseName(const CString& strFileName);

//+++ ファイルパス名より、ディレクトリ名を取得. 最後の'\\'は含まない.
const CString	GetDirName(const CString& strFileName);

///+++ ファイル名の拡張子の取得. ※ 結果の文字列には'.'は含まれない.
const CString	GetFileExt(const CString& strFileName);

///+++ フォルダ＆拡張子無しのファイル名の取得. ※ 結果の文字列には'.'は含まれない.
const CString	GetFileBaseNoExt(const CString& strFileName);

///+++ 拡張子無しのファイル名の取得. ※ 結果の文字列には'.'は含まれない.
const CString	GetFileNameNoExt(const CString& strFileName);

//+++ ttp://にhを足したり、両端の空白を削除したりする(SearchBar.hの関数から分離改造したもの)
void	StrToNormalUrl(CString& strUrl);

/// 被らないファイルパスにして返す
int	GetUniqueFilePath(CString& filepath, int nStart = 1);

//	strFileを .bak をつけたファイルにコピー. 古い .bak があればそれは削除.
void	CopyToBackupFile(const CString& strFileName);


// ==========================================================================

///+++ undonut.exeのフルパス名を返す.  (MtlGetModuleFileNameと一緒だった...)
const CString 	GetExeFileName();

///+++ exe(dll)のあるフォルダを返す. 最後に'\\'が付く
const CString 	GetExeDirectory();

///+++ exe(dll)のあるフォルダを返す. 最後に'\\'が付かない
const CString 	GetExeDirName();

///+++ 手抜きなフルパス化. ディレクトリの指定がなければ、undonutフォルダ下となる.
const CString GetFullPath_ForExe(const CString& strFileName);

//------------------------------------------------------
CRect	GetMonitorWorkArea(HWND hWnd);


}	// namespace Misc


#endif
