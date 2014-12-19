/**
*	@file	DirectoryWatcher.h
*	@brief	フォルダ内のファイルの追加を監視します
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

#include <thread>
#include <functional>
#include <Windows.h>


////////////////////////////////////////////////////////////////////
/// CDirectoryWatcher : 
/// フォルダ内のファイルの変更を監視する
/// 指定できるフィルタは下記のURLを参照
/// http://msdn.microsoft.com/ja-jp/library/cc429676.aspx

class CDirectoryWatcher
{
public:

	CDirectoryWatcher(DWORD dwFilter = FILE_NOTIFY_CHANGE_FILE_NAME) : 
		m_hDir(INVALID_HANDLE_VALUE), 
		m_hExitEvent(INVALID_HANDLE_VALUE),
		m_dwNotifyFilter(dwFilter)
	{	}

	~CDirectoryWatcher()
	{
		StopWatchDirectory();
	}

	/// 監視が開始されていれば true を返す
	bool	IsWatching() const { return m_hDir != INVALID_HANDLE_VALUE; }

	/// コールバック関数を登録する  (コールバック関数は別スレッドから呼ばれることに注意！)
	void	SetCallbackFunction(std::function<void (const CString&)> func) { m_funcCallback = func; }

	/// 監視を開始する
	bool	WatchDirectory(const CString& DirPath)
	{
		if (m_DirectoryWatchThread.joinable())
			return false;

		ATLASSERT( m_funcCallback );

		m_hDir = CreateFile(
								DirPath,                            // ファイル名へのポインタ
								FILE_LIST_DIRECTORY,                // アクセス（ 読み書き）モード
								FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,  // 共有モード
								NULL,                               // セキュリティ記述子
								OPEN_EXISTING,                      // 作成方法
								FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,         // ファイル属性
								NULL                                // コピーするファイルと属性
								);
		if (m_hDir == INVALID_HANDLE_VALUE )
			return false;

		m_DirPath = DirPath;
		if (m_DirPath[m_DirPath.GetLength() - 1] != _T('\\'))
			m_DirPath += _T('\\');

		m_hExitEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		ATLASSERT(m_hExitEvent != INVALID_HANDLE_VALUE);

		m_DirectoryWatchThread.swap(std::thread(&CDirectoryWatcher::_watchThread, this));
		return true;
	}

	/// 監視を終わらせる
	void	StopWatchDirectory()
	{
		if (m_DirectoryWatchThread.joinable() == false)
			return ;

		ATLVERIFY( ::SetEvent(m_hExitEvent) );

		m_DirectoryWatchThread.join();

		CloseHandle(m_hExitEvent);
		m_hExitEvent = INVALID_HANDLE_VALUE;

		CloseHandle(m_hDir);
		m_hDir = INVALID_HANDLE_VALUE;
	}


private:

	void	_watchThread()
	{
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		for (;;) {
			ResetEvent(hEvent);

			enum { kTempBuffSize = 8000 };
			BYTE tempBuff[kTempBuffSize] = { 0 };
			DWORD dwBytesReturned = 0;
			OVERLAPPED overLapped = { 0 };
			overLapped.hEvent = hEvent;
			if (!ReadDirectoryChangesW(m_hDir, static_cast<LPVOID>(tempBuff), kTempBuffSize, FALSE, 
				m_dwNotifyFilter, nullptr, &overLapped, nullptr))
				return ;

			// 通知待ち
			HANDLE handles[] = { m_hExitEvent, hEvent };
			DWORD dwResult = ::WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE);
			if (dwResult == WAIT_OBJECT_0) {
				CancelIo(m_hDir);
				WaitForSingleObject(hEvent, INFINITE);
				return ;	// 終了

			} else if (dwResult == WAIT_OBJECT_0 + 1) {
				DWORD dwRetSize = 0;
				if (!GetOverlappedResult(m_hDir, &overLapped, &dwRetSize, FALSE)) {
					ATLASSERT( FALSE );
					break;
				}
					
				if (dwRetSize == 0) {	// バッファオーバー
					ATLASSERT( FALSE );

				} else {
					FILE_NOTIFY_INFORMATION* pData = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(tempBuff);
					for (;;) {
						//switch (pData->Action) {
						//case FILE_ACTION_ADDED:
						//case FILE_ACTION_MODIFIED:
							{
								std::vector<WCHAR> filename((pData->FileNameLength / sizeof(WCHAR)) + 1);
								memcpy(reinterpret_cast<LPVOID>(&filename[0]), static_cast<LPVOID>(pData->FileName), pData->FileNameLength);

								CString filepath = m_DirPath + filename.data();

								// ファイルの追加を通知する
								m_funcCallback(filepath);
							}
						//}
						if (pData->NextEntryOffset == 0)
							break;

						pData = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<BYTE*>(pData) + pData->NextEntryOffset);
					}
				}
			}
		}

		// 後片付け
		CloseHandle(hEvent);
	}

	// Data members
	std::thread	m_DirectoryWatchThread;
	std::function<void (const CString&)>	m_funcCallback;
	CString		m_DirPath;
	HANDLE		m_hDir;
	HANDLE		m_hExitEvent;
	DWORD		m_dwNotifyFilter;

};
















