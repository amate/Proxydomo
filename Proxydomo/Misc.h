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

#include <atlstr.h>
#include <atlapp.h>
#define _WTL_NO_CSTRING
#include <atlmisc.h>

namespace Misc {

/// クリップボードにあるテキストを取得する
CString GetClipboardText(bool bUseOLE = false);

// ==========================================================================

///+++ printf書式指定で文字列を作る
const CString StrFmt(const TCHAR* fmt, ...);
const CString StrVFmt(const TCHAR* fmt, va_list app);

///+++ 空白で区切られた最初の文字列を返す.
const CString GetStrWord(const TCHAR* str, const TCHAR** ppNext=0);

///+++ 空白で区切られた複数単語を分解する
void SeptTextToWords(std::vector<CString>& strs, const TCHAR* line);

///+++ 文字列を改行文字で分解. (空行は無視)
void SeptTextToLines(std::vector<CString>& strs, const TCHAR* text);


///+++ 簡易な、半角文字換算での文字数もどきカウント。
unsigned	eseHankakuStrLen(const TCHAR* s);

///+++ 似非半角指定で指定された文字数までの文字列を返す.
const CString eseHankakuStrLeft(const CString& str, int len);


// ==========================================================================
// +++ 文字コード変換.

const std::vector<char> 	sjis_to_sjis (const char*    pSjis);	//+++
const std::vector<wchar_t> 	wcs_to_wcs   (const wchar_t* pWcs );	//+++

const std::vector<char> 	wcs_to_sjis  (const wchar_t* pWcs);		//+++
const std::vector<wchar_t> 	sjis_to_wcs  (const char*    pSjis);	//+++

const std::vector<char> 	sjis_to_utf8 (const char*    pSjis);	//+++
const std::vector<char> 	utf8_to_sjis (const char*    pUtf8);	//+++

const std::vector<char> 	wcs_to_utf8  (const wchar_t* pWcs);		//+++
const std::vector<wchar_t> 	utf8_to_wcs  (const char*    pUtf8);	//+++

const std::vector<char> 	sjis_to_eucjp(const char*    pSjis);	//+++
const std::vector<char> 	eucjp_to_sjis(const char*    pEucjp);	//+++

const std::vector<char> 	wcs_to_eucjp (const wchar_t* pWcs);		//+++
const std::vector<wchar_t> 	eucjp_to_wcs (const char*    pEucjp);	//+++

//+++
#ifdef UNICODE	// Unicode文字ベースでコンパイルした場合
__inline const std::vector<char>    tcs_to_sjis (const TCHAR*   pTcs  ) { return wcs_to_sjis (pTcs); }
__inline const std::vector<char>    tcs_to_utf8 (const TCHAR*   pTcs  ) { return wcs_to_utf8 (pTcs); }
__inline const std::vector<char>    tcs_to_eucjp(const TCHAR*   pTcs  ) { return wcs_to_eucjp(pTcs); }
__inline const std::vector<wchar_t> tcs_to_wcs  (const TCHAR*   pTcs  ) { return wcs_to_wcs  (pTcs); }

__inline const std::vector<TCHAR>   sjis_to_tcs (const char*    pSjis ) { return sjis_to_wcs (pSjis ); }
__inline const std::vector<TCHAR>   utf8_to_tcs (const char*    pUtf8 ) { return utf8_to_wcs (pUtf8 ); }
__inline const std::vector<TCHAR>   eucjp_to_tcs(const char*    pEucjp) { return eucjp_to_wcs(pEucjp); }
__inline const std::vector<TCHAR>   wcs_to_tcs  (const wchar_t* pWcs  ) { return wcs_to_wcs  (pWcs); }
#else			// MultiByte(sjis)ベースでコンパイルした場合.
__inline const std::vector<char>    tcs_to_sjis (const TCHAR*   pTcs  ) { return sjis_to_sjis (pTcs); }
__inline const std::vector<char>    tcs_to_eucjp(const TCHAR*   pTcs  ) { return sjis_to_eucjp(pTcs); }
__inline const std::vector<char>    tcs_to_utf8 (const TCHAR*   pTcs  ) { return sjis_to_utf8 (pTcs); }
__inline const std::vector<wchar_t> tcs_to_wcs  (const TCHAR*   pTcs  ) { return sjis_to_wcs  (pTcs); }

__inline const std::vector<TCHAR>   sjis_to_tcs (const char*    pSjis ) { return sjis_to_sjis (pSjis ); }
__inline const std::vector<TCHAR>   utf8_to_tcs (const char*    pUtf8 ) { return utf8_to_sjis (pUtf8 ); }
__inline const std::vector<TCHAR>   eucjp_to_tcs(const char*    pEucjp) { return eucjp_to_sjis(pEucjp); }
__inline const std::vector<TCHAR>   wcs_to_tcs  (const wchar_t* pWcs  ) { return wcs_to_sjis  (pWcs  ); }
#endif

__inline const CString sjis_to_CString(const char* pSjis)   { return CString( pSjis ); }
__inline const CString utf8_to_CString(const char* pUtf8)   { return CString(& utf8_to_tcs (pUtf8 )[0]); }
__inline const CString eucjp_to_CString(const char* pEucjp) { return CString(& eucjp_to_tcs(pEucjp)[0]); }

__inline const CString sjis_to_CString(const std::vector<char>& sjis)   { return CString( &sjis[0] ); }
__inline const CString utf8_to_CString(const std::vector<char>& utf8)   { return CString(& utf8_to_tcs (&utf8[0])[0]); }
__inline const CString eucjp_to_CString(const std::vector<char>& eucjp) { return CString(& eucjp_to_tcs(&eucjp[0])[0]); }


CString	UnknownToCString(const std::vector<char>& src);

//---------------------------------------------------
/// strがShift-JIS(CF文字列ならtrueを返す
bool	IsShiftJIS(LPCSTR str, int nCount);


// ==========================================================================

///+++ %20 等が使われたurlを通常の文字列に変換.
const std::vector<char>		urlstr_decode(const TCHAR* url);
const CString				urlstr_decodeJpn(const TCHAR* url, int dfltCode=0);

///+++ 通常の文字列を %20 等が使われたurlに変換.
const CString				urlstr_encode(const char* str);
__inline const CString		urlstr_encode(const std::vector<char>& str) { return urlstr_encode(&str[0]); }

///+++ 文字列中に、SJISに変換できないUNICODE文字が混ざっていた場合、文字列全体をurlstrに変換する.
const CString 				urlstr_encodeWhenNeedUnicode(const CString& str);

///+++ 文字列が半角英数記号のみでかつ%が混ざっている場合、utf8をエンコードしたものとしてデコードする.
const CString 				urlstr_decodeWhenASC(const CString& str);

///+++ UNICODE文字列を欠損なくSJISに変換できるか?
bool IsEnableUnicodeToSJIS(LPCWSTR pStr);

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


// ==========================================================================

inline bool IsExistFile(LPCTSTR fname) { return ::GetFileAttributes(fname) != 0xFFFFFFFF; }

//+++	strFileを .bak をつけたファイルに変名. 古い .bak があればそれは削除.
void	MoveToBackupFile(const CString& strFileName);

//	strFileを .bak をつけたファイルにコピー. 古い .bak があればそれは削除.
void	CopyToBackupFile(const CString& strFileName);

///+++ カレントディレクトリの取得.
const CString GetCurDirectory();

//+++
int SetCurDirectory(const CString& strDir);


///+++ 手抜きなフルパス化. 入力の区切りは'/'.
const CString MakeFullPath(const CString& strDefaultDir, const CString& strFileName);

///+++
void 	StrBkSl2Sl(CString& str);

///+++ テキストファイルを文字列として取得.
const CString FileLoadTextString(LPCTSTR strFile);

///+++ ファイルサイズを返す.
size_t			GetFileSize(const TCHAR* fname);

///+++ ファイルのロード.
void*			FileLoad(const TCHAR* fname, void* mem, size_t size);

///+++ ファイルのロード.
template<class CHAR_VECTOR>
size_t	FileLoad(const TCHAR* fname, CHAR_VECTOR& vec) {
	enum { CHAR_SIZE = sizeof(vec[0]) };
	vec.clear();
	size_t	l = GetFileSize(fname);
	if (l > 0x1000000)		//+++ とりあえず、適当に 16Mバイトで打ち止めにしとく
		l = 0x1000000;
	size_t	n = l / CHAR_SIZE;
	if (n > 0) {
		vec.resize(n);
		FileLoad(fname, &vec[0], n * CHAR_SIZE);
	}
	return n;
}



// ==========================================================================

///+++ undonut.exeのフルパス名を返す.  (MtlGetModuleFileNameと一緒だった...)
const CString 	GetExeFileName();

///+++ exe(dll)のあるフォルダを返す. 最後に'\\'が付く
const CString 	GetExeDirectory();

///+++ exe(dll)のあるフォルダを返す. 最後に'\\'が付かない
const CString 	GetExeDirName();

//+++ .manifestの存在チェック	※ CThemeDLLLoader から独立 & ちょっとリネーム
bool  			IsExistManifestFile();

///+++ 手抜きなフルパス化. ディレクトリの指定がなければ、undonutフォルダ下となる.
const CString 	GetFullPath_ForExe(const CString& strFileName);



// ==========================================================================

//+++ エラーログ出力.
void ErrorLogPrintf(const TCHAR* fmt, ...);


// ==========================================================================

BOOL	IsWow64(); 					//+++
unsigned getIEMejourVersion();		//+++
bool	IsGpuRendering();
bool	IsVistalater();


// ==========================================================================

#if _ATL_VER >= 0x700
typedef ATL::CRegKey  CRegKey;
#else	//+++ wtl7以降 仕様が変わったようなので、atl3の場合はラップして似せておく.
class CRegKey : public ATL::CRegKey {
public:
	CRegKey() throw() {;}
	// CRegKey( CRegKey& key ) throw() : ATL::CRegKey(key) { ; }
	// explicit CRegKey(HKEY hKey) throw() : ATL::CRegKey(hKey) {;}

	//+++	ATL3系でコンパイルできるようにするためのラッパー関数
	HRESULT 	QueryStringValue(const TCHAR* key, TCHAR* dst, DWORD* pCount) { return this->QueryValue(dst, key, pCount); }
	HRESULT 	SetStringValue(const TCHAR* value, const TCHAR* key) { return this->SetValue(value ? value : _T(""), key); }
	HRESULT 	QueryDWORDValue(const TCHAR* key, DWORD& value) { return this->QueryValue(value, key); }
	HRESULT 	SetDWORDValue(const TCHAR* key, DWORD value) { return this->SetValue(value, key); }
};
#endif


bool setHeapAllocLowFlagmentationMode();
bool mallocHeapCompact();

//------------------------------------------------------
CRect	GetMonitorWorkArea(HWND hWnd);


}	// namespace Misc


//using namespace Misc;			//+++ 手抜き
using Misc::ErrorLogPrintf;		//+++ デバッグ用なんでお手軽に...

#endif
