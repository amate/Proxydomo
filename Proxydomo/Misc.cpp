/**
 *	@file	Misc.cpp
 *	@biref	わりと汎用的な雑多なルーチン群
 *	@note
 *
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

#include "stdafx.h"
#include "Misc.h"
#include <stdarg.h>
#include <io.h>
#include <winerror.h>


namespace Misc {

	
//////////////////////////////////////////////////////////////////////////
CString GetClipboardText(bool bUseOLE /*= false*/)
{
	CString strText;
	if (bUseOLE == false) {
		if ( ::IsClipboardFormatAvailable(CF_UNICODETEXT) && ::OpenClipboard(NULL) ) {
			HGLOBAL hText = ::GetClipboardData(CF_UNICODETEXT);
			if (hText) {
				strText = reinterpret_cast<LPTSTR>( ::GlobalLock(hText) );		//+++ UNICODE修正
				::GlobalUnlock(hText);
			}
			::CloseClipboard();
		}
	} else {
		CComPtr<IDataObject> spDataObject;
		HRESULT hr = ::OleGetClipboard(&spDataObject);

		if ( FAILED(hr) )
			return strText;
		FORMATETC			 formatetc = { CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		STGMEDIUM			 stgmedium;
		hr = spDataObject->GetData(&formatetc, &stgmedium);

		if ( SUCCEEDED(hr) ) {
			if (stgmedium.hGlobal != NULL) {
				HGLOBAL hText = stgmedium.hGlobal;
				strText = reinterpret_cast<LPTSTR>( ::GlobalLock(hText) );		//+++ UNICODE 修正
				::GlobalUnlock(hText);
			}
			::ReleaseStgMedium(&stgmedium);
		}
	}

	return strText;
}


bool SetClipboardText(const CString& str)
{
	if ( str.IsEmpty() )
		return false;

	int 	nByte = (str.GetLength() + 1) * sizeof(TCHAR);
	HGLOBAL hText = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, nByte);
	if (hText == NULL)
		return false;

	BYTE* pText = (BYTE*)::GlobalLock(hText);
	if (pText == NULL)
		return false;

	::memcpy(pText, (LPCTSTR) str, nByte);

	::GlobalUnlock(hText);

	::OpenClipboard(NULL);
	::EmptyClipboard();
	::SetClipboardData(CF_UNICODETEXT, hText);
	::CloseClipboard();
	return true;
}


// ==========================================================================

//+++ ファイルパス名より、ファイル名を取得
TCHAR *GetFileBaseName(const TCHAR* adr)
{
	const TCHAR *p;
	for (p = adr; *p != _T('\0'); ++p) {
		if (*p == _T(':') || *p == _T('/') || *p == _T('\\')) {
			adr = p + 1;
		}
	  #ifndef UNICODE
		if (_ismbblead((*(unsigned char *)p)) && *(p+1) )
			++p;
	  #endif
	}
	return (TCHAR*)adr;
}


//+++ ファイルパス名より、ファイル名を取得
const CString	GetFileBaseName(const CString& strFileName)
{
  #if 1
	return GetFileBaseName(LPCTSTR(strFileName));
  #else
	int 	n	= strFileName.ReverseFind( _T('\\') );
	int 	m	= strFileName.ReverseFind( _T('/') );
	if (n < 0 && m < 0) {
		return strFileName;
	}
	n = std::max(n, m);
	return	strFileName.Mid(n + 1);
  #endif
}


//+++ ファイルパス名より、ディレクトリ名を取得. 最後の'\\'は含まない.
const CString	GetDirName(const CString& strFileName)
{
	int 	n	= strFileName.ReverseFind( _T('\\') );
	if (n < 0)
		return _T(".");	//strFileName;
	return	strFileName.Left(n);
}



///+++ ファイル名の拡張子の取得. ※ 結果の文字列には'.'は含まれない.
TCHAR *GetFileExt(const TCHAR *name)
{
	name			= GetFileBaseName(name);
	const TCHAR*  p = _tcsrchr(name, _T('.'));
	if (p) {
		return (TCHAR*)(p+1);
	}
	return (TCHAR*)name + _tcslen(name);
}



///+++ ファイル名の拡張子の取得. ※ 結果の文字列には'.'は含まれない.
const CString	GetFileExt(const CString& strFileName)
{
  #if 1
	return GetFileExt(LPCTSTR(strFileName));
  #else
	const CString strBaseName = GetFileBaseName(strFileName);
	int 	n	= strBaseName.ReverseFind( _T('.') );
	if (n < 0)
		return CString();
	return strBaseName.Mid(n + 1);
  #endif
}


///+++ フォルダ＆拡張子無しのファイル名の取得. ※ 結果の文字列には'.'は含まれない.
const CString	GetFileBaseNoExt(const CString& strFileName)
{
	const CString strBaseName = GetFileBaseName(strFileName);
	int 	n	= strBaseName.ReverseFind( _T('.') );
	if (n < 0)
		return strBaseName;
	return strBaseName.Left(n);
}



///+++ 拡張子無しのファイル名の取得. ※ 結果の文字列には'.'は含まれない.
const CString	GetFileNameNoExt(const CString& strFileName)
{
  #if 1
	const TCHAR*	name	= LPCTSTR(strFileName);
	const TCHAR*	baseName= GetFileBaseName(name);
	const TCHAR*	p 		= _tcsrchr(baseName, _T('.'));
	if (p) {
		if (p == baseName)
			return CString();
		return CString(name, static_cast<int>(p - name));
	}
	return strFileName;
  #else
	const CString strBaseName = GetFileBaseName(strFileName);
	int 	n	= strBaseName.ReverseFind( _T('.') );
	if (n < 0)
		return strFileName;

	int 	nd	= strFileName.ReverseFind( _T('\\') );
	if (nd >= 0)
		return strFileName.Left(nd + 1) + strBaseName.Left(n);
	return strBaseName.Left(n);
  #endif
}

/// 被らないファイルパスにして返す
int	GetUniqueFilePath(CString& filepath, int nStart /*= 1*/)
{
	if (::PathFileExists(filepath) == FALSE)
		return 0;

	CString basepath;
	CString ext;
	int nExt = filepath.ReverseFind(_T('.'));
	if (nExt != -1) {
		basepath = filepath.Left(nExt);
		ext		= filepath.Mid(nExt);
	} else {
		basepath = filepath;
	}
	for (;;) {
		filepath.Format(_T("%s_[%d]%s"), basepath, nStart, ext);
		if (::PathFileExists(filepath) == FALSE)
			return nStart;
		++nStart;
	}
}


// ==========================================================================

///+++ undonut.exeのフルパス名を返す.  (MtlGetModuleFileNameと一緒だった...)
const CString 	GetExeFileName()
{
	TCHAR	buf[MAX_PATH];
	buf[0] = 0;
	::GetModuleFileName(_Module.GetModuleInstance(), buf, MAX_PATH);
	return CString(buf);
}


///+++ exe(dll)のあるフォルダを返す. 最後に'\\'が付く
const CString GetExeDirectory()
{
	CString str = GetExeFileName();
	int 	n	= str.ReverseFind( _T('\\') );
	return	str.Left(n+1);
}


///+++ exe(dll)のあるフォルダを返す. 最後に'\\'が付かない
const CString GetExeDirName()
{
	CString str = GetExeFileName();
	int 	n	= str.ReverseFind( _T('\\') );
	return	str.Left(n);
}

//	strFileを .bak をつけたファイルにコピー. 古い .bak があればそれは削除.
void	CopyToBackupFile(const CString& strFileName)
{
	CString 	strBakName = strFileName + _T(".bak");
	if (::PathFileExists(strFileName))
		::CopyFile(strFileName, strBakName, FALSE);		// 既存のファイルをバックアップファイルにする.
}


/// .exeのあるディレクトリのフルパスを返す. 最後に'\\'を含む.
//   カレントディレクトリの取得.
const CString GetCurDirectory()
{
	TCHAR dir[MAX_PATH + 2];
	dir[0] = 0;
	::GetCurrentDirectory(MAX_PATH, dir);
	return CString(dir);
}


//+++
int SetCurDirectory(const CString& strDir)
{
	return ::SetCurrentDirectory(LPCTSTR(strDir));
}

///+++ 手抜きなフルパス化. ディレクトリの指定がなければ、undonutフォルダ下となる.
const CString GetFullPath_ForExe(const CString& strFileName)
{
	const TCHAR* p = LPCTSTR(strFileName);
	if (p == 0 || *p == 0) {
		return GetExeDirectory();
	}
	unsigned	c = *p;
	unsigned	c1 = p[1];
	unsigned	c2 = 0;
	if (c == '\\' || c == '/' || (c1 == ':' && ((c2 = p[2]) == '\\' || c2 == '/'))) {
		return strFileName; // フルパスだろうでそのまま返す.
	} else if (c1 == ':' || (c == '.' && (c1 == '\\' || c1 == '/' || (c1 == '.' && ((c2 = p[2]) == '\\' || c2 == '/'))))) {
		// 相対パスぽかったら、通常のフルパス処理を呼ぶ... 手抜きでカレントを調整.
		CString sav = GetCurDirectory();
		SetCurDirectory(GetExeDirectory());
		TCHAR	buf[MAX_PATH + 4];
		buf[0] = 0;	//+++
		::_tfullpath(buf, strFileName, MAX_PATH);
		SetCurDirectory(sav);
		return CString(buf);
	} else {	// そうでない場合はundonutのフォルダ下の指定として扱う.
		return GetExeDirectory() + strFileName;
	}
}


//------------------------------------------------------
CRect	GetMonitorWorkArea(HWND hWnd)
{
	HMONITOR hMoni = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO	moniInfo = { sizeof(MONITORINFO) };
	GetMonitorInfo(hMoni, &moniInfo);
	return moniInfo.rcWork;
}


}	// namespace Misc
