/**
*	@file	Settings.cpp
*	@brief	全フィルターと全リストを持っている
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
#include "Settings.h"
#include <fstream>
#include <codecvt>
#include <random>
#include <atomic>
#include <boost\property_tree\ptree.hpp>
#include <boost\property_tree\ini_parser.hpp>
#include <boost\property_tree\xml_parser.hpp>
#include <boost\format.hpp>
#include <boost\algorithm\string\predicate.hpp>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#include "Misc.h"
#include "proximodo\util.h"
//#include "proximodo\matcher.h"
#include "Log.h"
#include "Matcher.h"
#include "CodeConvert.h"
#include "Logger.h"
#include "Matcher.h"
#include "DirectoryWatcher.h"
#include "UITranslator.h"
#include "AdblockFilter.h"
#include "BlockListDatabase.h"

using namespace CodeConvert;
using namespace boost::property_tree;
using namespace UITranslator;

////////////////////////////////////////////////////////////
// CSettings

enum { DEFAULTPECA_PORT = 6060 };
LPCWSTR kDefaultLanguage = L"English";

uint16_t		CSettings::s_proxyPort	= DEFAULTPECA_PORT;
bool			CSettings::s_privateConnection = true;
bool			CSettings::s_filterText	= true;
bool			CSettings::s_filterIn	= true;
bool			CSettings::s_filterOut	= true;
bool			CSettings::s_bypass	= false;
bool			CSettings::s_SSLFilter = false;
bool			CSettings::s_WebFilterDebug	= false;

bool			CSettings::s_useRemoteProxy	= false;
std::string		CSettings::s_defaultRemoteProxy;
std::set<std::string> CSettings::s_setRemoteProxy;

std::unique_ptr<wchar_t[]>	CSettings::s_urlCommandPrefix;

std::wstring	CSettings::s_language = kDefaultLanguage;

bool			CSettings::s_tasktrayOnCloseBotton = false;
bool			CSettings::s_saveBlockListUsageSituation = false;

int				CSettings::s_logLevel = boost::log::trivial::warning;

std::thread		CSettings::s_threadSaveFilter;

std::vector<std::unique_ptr<FilterItem>>	CSettings::s_vecpFilters;
std::chrono::steady_clock::time_point		CSettings::s_lastFiltersSaveTime;
CCriticalSection							CSettings::s_csFilters;

std::shared_ptr<Proxydomo::CMatcher>	CSettings::s_pBypassMatcher;

std::shared_ptr<Proxydomo::CMatcher>	CSettings::s_pPriorityCharsetMatcher;

std::recursive_mutex						CSettings::s_mutexHashedLists;
std::unordered_map<std::string, std::unique_ptr<HashedListCollection>>	CSettings::s_mapHashedLists;

CDirectoryWatcher	g_listChangeWatcher(FILE_NOTIFY_CHANGE_LAST_WRITE);
CCriticalSection	g_csListChangeWatcher;


void	CSettings::LoadSettings()
{
	std::ifstream fs(Misc::GetExeDirectory() + _T("settings.ini"));
	if (fs) {
		ptree pt;
		try {
			read_ini(fs, pt);

			if (auto value = pt.get_optional<uint16_t>("Setting.ProxyPort"))
				s_proxyPort	= value.get();
			if (auto value = pt.get_optional<bool>("Setting.privateConnection"))
				s_privateConnection = value.get();
			if (auto value = pt.get_optional<bool>("Setting.filterText"))
				s_filterText	= value.get();
			if (auto value = pt.get_optional<bool>("Setting.filterIn"))
				s_filterIn	= value.get();
			if (auto value = pt.get_optional<bool>("Setting.filterOut"))
				s_filterOut	= value.get();
			if (auto value = pt.get_optional<bool>("Setting.bypass"))
				s_bypass = value.get();

			s_useRemoteProxy = pt.get<bool>("RemoteProxy.UseRemoteProxy", s_useRemoteProxy);
			s_defaultRemoteProxy = pt.get("RemoteProxy.defaultRemoteProxy", "");
			const int remoteProxyCount = pt.get("RemoteProxy.Count", 0);
			for (int i = 0; i < remoteProxyCount; ++i) {
				std::string path = boost::io::str(boost::format("RemoteProxy.proxy%d") % i);
				std::string remoteproxy = pt.get(path, "");
				if (remoteproxy.length())
					s_setRemoteProxy.insert(remoteproxy);
			}

			if (auto value = pt.get_optional<std::string>("Setting.language")) {
				s_language = UTF16fromUTF8(value.get());
			} else {
				CString language = kDefaultLanguage;
				int len = GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SENGLANGUAGE, nullptr, 0);
				if (len) {			
					GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SENGLANGUAGE, language.GetBuffer(len + 1), len);
					language.ReleaseBuffer();

					CString traslateFilePath = Misc::GetExeDirectory() + L"lang\\" + language + L".lng";
					if (::PathFileExists(traslateFilePath) == FALSE) {
						language = kDefaultLanguage;
					}
				}
				s_language = language;
			}

			s_logLevel = pt.get<int>("Setting.logLevel", s_logLevel);

			if (auto value = pt.get_optional<bool>("Setting.tasktrayOnCloseBotton"))
				s_tasktrayOnCloseBotton = value.get();
			if (auto value = pt.get_optional<bool>("Setting.saveBlockListUsageSituation"))
				s_saveBlockListUsageSituation = value.get();
		}
		catch (...) {
			MessageBox(NULL, GetTranslateMessage(ID_LOADSETTINGFAILED).c_str(), GetTranslateMessage(ID_TRANS_ERROR).c_str(), MB_ICONERROR);
			fs.close();
			MoveFile(Misc::GetExeDirectory() + _T("settings.ini"), Misc::GetExeDirectory() + _T("settings_loadfailed.ini"));
		}
	}

	UITranslator::LoadUILanguage();

	// prefixを設定
	enum { kPrefixSize = 8 };
	const wchar_t charactorSelection[] = L"abcdefghijklmnopqrstuvqxyz0123456789";
	std::random_device	randEngine;
	std::uniform_int_distribution<int> dist(0, std::size(charactorSelection) - 2);
	s_urlCommandPrefix = std::make_unique<wchar_t[]>(kPrefixSize + 2);
	std::array<int, kPrefixSize> randnum;
	for (int i = 0; i < kPrefixSize; ++i) {
		randnum[i] = dist(randEngine);
	}
	for (int i = 0; i < kPrefixSize; ++i) {
		s_urlCommandPrefix[i] = charactorSelection[randnum[i]];

	}
	s_urlCommandPrefix[kPrefixSize] = L'_';
	s_urlCommandPrefix[kPrefixSize + 1] = L'\0';

	// Bypass matcherを作成
	s_pBypassMatcher = Proxydomo::CMatcher::CreateMatcher(L"$LST(Bypass)");

	s_pPriorityCharsetMatcher = Proxydomo::CMatcher::CreateMatcher(L"$LST(PriorityCharset)");

	CSettings::LoadFilter();

	CBlockListDatabase::GetInstance()->Init();
	{
		auto blockListDB = CBlockListDatabase::GetInstance();
		std::unique_lock<CBlockListDatabase> lockdb;
		if (blockListDB) {
			lockdb = std::unique_lock<CBlockListDatabase>(*blockListDB);
		}
		// lists フォルダからブロックリストを読み込む
		std::function<void(const CString&, bool)> funcForEach;
		funcForEach = [&funcForEach](const CString& path, bool bFolder) {
			if (bFolder) {
				if (Misc::GetFileBaseName(path).Left(1) != L"#")
					ForEachFileFolder(path, funcForEach);
			} else {
				LoadList(path);
			}
		};
		ForEachFileFolder(Misc::GetExeDirectory() + _T("lists\\"), funcForEach);

		if (blockListDB) {
			blockListDB->DeleteNoExistList();
		}
	}

	g_listChangeWatcher.SetCallbackFunction([](const CString& filePath) {
		enum { kMaxRetry = 30, kSleepTime = 100 };
		for (int i = 0; i < kMaxRetry; ++i) {
			HANDLE hTestOpen = CreateFile(filePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hTestOpen == INVALID_HANDLE_VALUE) {
				::Sleep(kSleepTime);
				continue;
			}
			CloseHandle(hTestOpen);
			break;
		}
		auto blockListDB = CBlockListDatabase::GetInstance();
		std::unique_lock<CBlockListDatabase> lockdb;
		if (blockListDB) {
			lockdb = std::unique_lock<CBlockListDatabase>(*blockListDB);
		}
		CSettings::LoadList(filePath);
	});
	g_listChangeWatcher.WatchDirectory(Misc::GetExeDirectory() + _T("lists\\"));
}

void CSettings::StopWatchDirectory()
{
	g_listChangeWatcher.StopWatchDirectory();
}

void	CSettings::SaveSettings()
{
	std::string settingsPath = CT2A(Misc::GetExeDirectory() + _T("settings.ini"));
	ptree pt;
	try {
		read_ini(settingsPath, pt);
	} catch (...) {
		ERROR_LOG << L"CSettings::SaveSettings : settings.iniの読み込みに失敗";
		pt.clear();
	}

	pt.put("Setting.ProxyPort", s_proxyPort);
	pt.put("Setting.privateConnection", s_privateConnection);
	pt.put("Setting.filterText"	, s_filterText);
	pt.put("Setting.filterIn"	, s_filterIn);
	pt.put("Setting.filterOut"	, s_filterOut);
	pt.put("Setting.bypass", s_bypass);

	pt.erase("RemoteProxy");
	pt.put<bool>("RemoteProxy.UseRemoteProxy", s_useRemoteProxy);
	pt.put("RemoteProxy.defaultRemoteProxy", s_defaultRemoteProxy);
	const int remoteProxyCount = (int)s_setRemoteProxy.size();
	pt.put("RemoteProxy.Count", remoteProxyCount);
	int i = 0;
	for (auto& remoteproxy : s_setRemoteProxy) {
		std::string path = boost::io::str(boost::format("RemoteProxy.proxy%d") % i);
		++i;
		pt.put(path, remoteproxy);
	}

	pt.put("Setting.language", UTF8fromUTF16(s_language));

	pt.put("Setting.logLevel", s_logLevel);

	pt.put<bool>("Setting.tasktrayOnCloseBotton", s_tasktrayOnCloseBotton);
	pt.put<bool>("Setting.saveBlockListUsageSituation", s_saveBlockListUsageSituation);	

	write_ini(settingsPath, pt);
}

void	LoadFilterItem(wptree& ptChild, std::vector<std::unique_ptr<FilterItem>>& vecpFilter)
{
	for (auto& ptIt : ptChild) {
		wptree& ptFilter = ptIt.second;
		std::unique_ptr<FilterItem>	pFilterItem(new FilterItem);
		if (ptIt.first == L"filter") {
			std::unique_ptr<CFilterDescriptor> pFilter(new CFilterDescriptor);
			pFilter->Active		= ptFilter.get<bool>(L"Active", true);
			pFilter->title		= ptFilter.get(L"title", L"");
			pFilter->version	= UTF8fromUTF16(ptFilter.get(L"version", L""));
			pFilter->author		= UTF8fromUTF16(ptFilter.get(L"author", L""));
			pFilter->comment	= UTF8fromUTF16(ptFilter.get(L"comment", L""));
			pFilter->filterType	= 
				static_cast<CFilterDescriptor::FilterType>(
					ptFilter.get<int>(L"filterType", (int)CFilterDescriptor::kFilterText));
			pFilter->headerName	= ptFilter.get(L"headerName", L"");
			pFilter->multipleMatches= ptFilter.get<bool>(L"multipleMatches", false);
			pFilter->windowWidth	= ptFilter.get<int>(L"windowWidth", 128);
			pFilter->boundsPattern	= (ptFilter.get(L"boundsPattern", L""));
			pFilter->urlPattern		= (ptFilter.get(L"urlPattern", L""));
			pFilter->matchPattern	= (ptFilter.get(L"matchPattern", L""));
			pFilter->replacePattern	= (ptFilter.get(L"replacePattern", L""));
			
			pFilter->CreateMatcher();

			pFilterItem->pFilter = std::move(pFilter);
			vecpFilter.push_back(std::move(pFilterItem));

		} else if (ptIt.first == L"folder") {
			pFilterItem->name = ptFilter.get(L"<xmlattr>.name", L"no name").c_str();
			pFilterItem->active = ptFilter.get<bool>(L"<xmlattr>.active", true);
			pFilterItem->pvecpChildFolder.reset(new std::vector<std::unique_ptr<FilterItem>>);
			LoadFilterItem(ptFilter, *pFilterItem->pvecpChildFolder);
			vecpFilter.push_back(std::move(pFilterItem));
		}
	}
}

void CSettings::LoadFilter()
{
	CString filterPath = Misc::GetExeDirectory() + _T("filter.xml");
	if (::PathFileExists(filterPath) == FALSE)
		return ;

	std::wifstream	fs(filterPath);
	if (!fs) {
		MessageBox(NULL, GetTranslateMessage(ID_OPENFILTERXMLFAILED).c_str(), GetTranslateMessage(ID_TRANS_ERROR).c_str(), MB_ICONERROR);
		return ;
	}
	fs.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t>));

	wptree pt;
	try {
		read_xml(fs, pt);
	} catch (...) {
		MessageBox(NULL, GetTranslateMessage(ID_LOADFILTERXMLFAILED).c_str(), GetTranslateMessage(ID_TRANS_ERROR).c_str(), MB_ICONERROR);
		return ;
	}
	if (auto& opChild = pt.get_child_optional(L"ProxydomoFilter")) {
		wptree& ptChild = opChild.get();
		LoadFilterItem(ptChild, s_vecpFilters);
	}
	s_lastFiltersSaveTime = std::chrono::steady_clock::now();
}

void	SaveFilterItem(std::vector<std::unique_ptr<FilterItem>>& vecpFilter, wptree& pt, std::atomic<bool>& bCansel)
{
	for (auto& pFilterItem : vecpFilter) {
		if (bCansel.load())
			return;

		if (pFilterItem->pvecpChildFolder) {
			wptree ptChild;
			SaveFilterItem(*pFilterItem->pvecpChildFolder, ptChild, bCansel);
			wptree& ptFolder = pt.add_child(L"folder", ptChild);
			ptFolder.put(L"<xmlattr>.name", (LPCWSTR)pFilterItem->name);
			ptFolder.put(L"<xmlattr>.active", pFilterItem->active);

		} else {
			wptree ptFilter;
			CFilterDescriptor* filter = pFilterItem->pFilter.get();
			ptFilter.put(L"Active", filter->Active);
			ptFilter.put(L"title", filter->title);
			ptFilter.put(L"version", UTF16fromUTF8(filter->version));
			ptFilter.put(L"author", UTF16fromUTF8(filter->author));
			ptFilter.put(L"comment", UTF16fromUTF8(filter->comment));
			ptFilter.put(L"filterType", (int)filter->filterType);
			ptFilter.put(L"headerName", filter->headerName);
			ptFilter.put(L"multipleMatches", filter->multipleMatches);
			ptFilter.put(L"windowWidth", filter->windowWidth);
			ptFilter.put(L"boundsPattern", (filter->boundsPattern));
			ptFilter.put(L"urlPattern", (filter->urlPattern));
			ptFilter.put(L"matchPattern", (filter->matchPattern));
			ptFilter.put(L"replacePattern", (filter->replacePattern));
			pt.add_child(L"filter", ptFilter);
		}
	}
}

void CSettings::SaveFilter()
{
	typedef std::vector<std::unique_ptr<FilterItem>>*	FilterFolder;
	auto pFilter = new std::vector<std::unique_ptr<FilterItem>>;
	//s_BookmarkList
	std::function <void (FilterFolder, FilterFolder)> funcCopy;
	funcCopy = [&](FilterFolder pFolder, FilterFolder pFolderTo) {
		for (auto it = pFolder->begin(); it != pFolder->end(); ++it) {
			FilterItem& item = *(*it);
			unique_ptr<FilterItem> pItem(new FilterItem(item));
			if (item.pvecpChildFolder) {
				pItem->pvecpChildFolder.reset(new std::vector<std::unique_ptr<FilterItem>>);
				funcCopy(item.pvecpChildFolder.get(), pItem->pvecpChildFolder.get());
			}
			pFolderTo->push_back(std::move(pItem));
		}
	};
	funcCopy(&s_vecpFilters, pFilter);

	s_lastFiltersSaveTime = std::chrono::steady_clock::now();

	static CCriticalSection	 s_cs;
	static std::atomic<bool> s_bCancel(false);

	std::thread tdSave([&, pFilter]() {
		for (;;) {
			if (s_cs.TryEnter()) {
				try {
					wptree ptChild;
					SaveFilterItem(*pFilter, ptChild, s_bCancel);
					if (s_bCancel.load()) {
						s_cs.Leave();
						s_bCancel.store(false);
						delete pFilter;
						return;
					}
					wptree pt;
					pt.add_child(L"ProxydomoFilter", ptChild);

					CString tempPath = Misc::GetExeDirectory() + _T("filter.temp.xml");
					CString filterPath = Misc::GetExeDirectory() + _T("filter.xml");

					std::wofstream	fs(tempPath);
					if (!fs) {
						//MessageBox(NULL, _T("filter.temp.xmlのオープンに失敗"), NULL, MB_ICONERROR);
						s_cs.Leave();
						s_bCancel.store(false);
						delete pFilter;
						return;
					}
					fs.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t>));

					write_xml(fs, pt, xml_parser::xml_writer_make_settings<std::wstring>(L' ', 2, xml_parser::widen<std::wstring>("UTF-8")));

					fs.close();
					::MoveFileEx(tempPath, filterPath, MOVEFILE_REPLACE_EXISTING);

				}
				catch (const boost::property_tree::ptree_error& error) {					
					MessageBox(NULL, GetTranslateMessage(ID_SAVEFILTERXMLFAILED, (LPWSTR)CA2W(error.what())).c_str(), 
								GetTranslateMessage(ID_TRANS_ERROR).c_str(), MB_ICONERROR);
				}
				s_cs.Leave();
				s_bCancel.store(false);
				delete pFilter;
				break;

			} else {
				// 他のスレッドが保存処理を実行中...
				ATLTRACE(_T("SaveFilter : TryEnter failed\n"));
				s_bCancel.store(true);
				::Sleep(100);
				continue;
			}
		}
	});
	s_threadSaveFilter.swap(tdSave);
	if (tdSave.joinable())
		tdSave.detach();
}

static void ActiveFilterCallFunc(std::vector<std::unique_ptr<FilterItem>>& vecFilters, 
								 std::function<void (CFilterDescriptor*)>& func)
{
	for (auto& filterItem : vecFilters) {
		if (filterItem->pFilter) {
			CFilterDescriptor* filter = filterItem->pFilter.get();
			if (filter->Active && filter->errorMsg.empty())
				func(filter);
		} else {
			if (filterItem->active)
				ActiveFilterCallFunc(*filterItem->pvecpChildFolder, func);
		}
	}
}

std::chrono::steady_clock::time_point CSettings::EnumActiveFilter(std::function<void (CFilterDescriptor*)> func)
{
	CCritSecLock	lock(CSettings::s_csFilters);
	ActiveFilterCallFunc(s_vecpFilters, func);
	return s_lastFiltersSaveTime;
}

/// 最終書き込み時刻を取得
static uint64_t GetFileLastWriteTime(const CString& filePath)
{
	HANDLE hFile = CreateFile(filePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return 0;

	FILETIME lastWriteTime = {};
	::GetFileTime(hFile, NULL, NULL, &lastWriteTime);
	uint64_t time = lastWriteTime.dwHighDateTime;
	time <<= 32;
	time |= lastWriteTime.dwLowDateTime;
	::CloseHandle(hFile);
	return time;
}

void CSettings::LoadList(const CString& filePath)
{
	CString ext = Misc::GetFileExt(filePath);
	ext.MakeLower();
	if (ext != _T("txt"))
		return;

	std::string filename = UTF8fromUTF16((LPCWSTR)Misc::GetFileBaseNoExt(filePath));
	std::unique_lock<std::recursive_mutex> lock(s_mutexHashedLists);
	auto& hashedLists = s_mapHashedLists[filename];
	if (hashedLists == nullptr) {
		hashedLists.reset(new HashedListCollection);
	}
	auto& whiteLists = s_mapHashedLists["*" + filename];
	if (whiteLists == nullptr) {
		whiteLists.reset(new HashedListCollection);
	}
	lock.unlock();

	boost::unique_lock<boost::shared_mutex>	lock2(hashedLists->mutex);
	if (hashedLists->filePath.empty()) {
		hashedLists->filePath = (LPCWSTR)filePath;
	} else {
		if (hashedLists->filePath != (LPCWSTR)filePath) {
			return;		// 同じファイル名では登録できないので

		} else if (hashedLists->bLogFile)
			return;		// ログファイルは何もしない
	}

	uint64_t lastWriteTime = GetFileLastWriteTime(filePath);
	if (hashedLists->prevLastWriteTime == lastWriteTime)
		return ;	// 更新されていなかったら帰る
	hashedLists->prevLastWriteTime = lastWriteTime;

	std::wifstream fs(filePath, std::ios::in);
	if (!fs) {
		ERROR_LOG << L"CSettings::LoadList file open error : " << (LPCTSTR)filePath;
		return;
	}
	fs.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t, 0x10ffff, std::codecvt_mode::consume_header>));

	auto blockListDB = CBlockListDatabase::GetInstance();
	if (blockListDB) {
		blockListDB->AddList(filename);
	}
	std::time_t updateTime = std::time(nullptr);
	int successLoadLineCount = 0;
	{
		hashedLists->Clear();
		whiteLists->Clear();

		std::wstring pattern;
		std::wstring strLine;
		int nLineCount = 0;
		bool	bAddPattern = false;
		auto funcCreatePattern = [&]() {
			CUtil::trim(strLine);
			bool isWhitePattern = boost::starts_with(pattern, L"~");
			if (isWhitePattern) {
				pattern.erase(pattern.begin());
			}
			if (pattern.length()) {
				bAddPattern = true;
				HashedListCollection& lists = isWhitePattern ? *whiteLists : *hashedLists;
				bool bSuccess = _CreatePattern(pattern, lists, nLineCount);
				if (bSuccess == false) {
					CLog::FilterEvent(kLogFilterListBadLine, nLineCount, filename, "");
				} else {
					if (blockListDB && isWhitePattern == false) {
						blockListDB->AddPatternToList(filename, pattern, nLineCount, updateTime);
					}
					++successLoadLineCount;
				}
			}
		};

		while (std::getline(fs, strLine)) {
			if (bAddPattern == false && nLineCount < 6 && strLine.length()) {
				if (strLine[0] == L'#' && boost::algorithm::contains(strLine, L"LOGFILE")) {
					hashedLists->bLogFile = true;
					break;	// LogFile

				} else if (nLineCount == 0) {
					if (boost::starts_with(strLine, L"[Adblock Plus")) {
						hashedLists->adblockFilter = LoadAdblockFilter(fs, filename);
						return ;	// 
					}
				}
			}

			if (strLine.length() && (strLine[0] == L' ' || strLine[0] == L'\t')) {
				CUtil::trim(strLine);
				pattern += strLine;

			} else {
				funcCreatePattern();

				if (strLine.empty() || strLine[0] == L'#') {
					pattern.clear();
				} else {
					pattern = strLine;
				}
			}
			++nLineCount;
			if (fs.eof())
				break;
		}
		funcCreatePattern();	// 最後の行

		hashedLists->lineCount = nLineCount;
	}
	if (blockListDB) {
		blockListDB->UpdateList(filename, "normal", successLoadLineCount);
		blockListDB->DeleteOldPatternFromList(filename, updateTime);
	}
	CLog::FilterEvent(kLogFilterListReload, successLoadLineCount, filename, "");
}

void	CSettings::AddListLine(const std::string& name, const std::wstring& addLine)
{
	if (addLine.empty())
		return;

	std::unique_lock<std::recursive_mutex> lock(s_mutexHashedLists);
	auto& hashedLists = s_mapHashedLists[name];
	if (hashedLists == nullptr) {
		hashedLists.reset(new HashedListCollection);
	}
	lock.unlock();

	boost::unique_lock<boost::shared_mutex>	lock2(hashedLists->mutex);
	if (hashedLists->bLogFile == false) {
		std::wstring pattern = addLine;
		bool bSuccess = _CreatePattern(pattern, *hashedLists, hashedLists->lineCount + 1);
		if (bSuccess == false) {
			CLog::FilterEvent(kLogFilterListBadLine, hashedLists->lineCount + 1, name, "");
			return;
		}
	}

	if (hashedLists->filePath.empty())
		return;

	++hashedLists->lineCount;
	std::wstring filePath = hashedLists->filePath;

	std::thread([filePath, addLine]() {
		std::wofstream fs(filePath, std::ios::out | std::ios::app | std::ios::binary);
		if (!fs)
			return;

		CCritSecLock	lock(g_csListChangeWatcher);
		g_listChangeWatcher.StopWatchDirectory();

		fs.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t, 0x10ffff, std::codecvt_mode::consume_header>));
		fs << addLine << L"\r\n";
		fs.close();

		g_listChangeWatcher.WatchDirectory(Misc::GetExeDirectory() + _T("lists\\"));
	}).detach();
}

static inline bool isNonWildWord(wchar_t c) {
	return (c != L'*' && c != L'\\' && c != L'[' && c != L'$' && c != L'(' && c != L')' && c != L'|' && c != L'&' && c != L'?' && c != L'~' && c != L'"' && c != L'\'' && c != L' ' && c != L'=' && c != L'^' &&  c != L'+');
}


bool CSettings::_CreatePattern(std::wstring& pattern, HashedListCollection& listCollection, int listLine)
{
	// 固定プレフィックス
	enum { kMaxPreHashWordLength = 7 };
	bool	bPreHash = true;
	std::size_t length = pattern.length();
	// (kMaxPreHashWordLength + 1)文字目に '+'があることを考慮する
	for (std::size_t i = 0; i < length && i < (kMaxPreHashWordLength + 1); ++i) {		
		if (isNonWildWord(pattern[i]) == false) {
			bPreHash = false;
			break;
		}
	}
	if (bPreHash) {
		auto pmapPreHashWord = &listCollection.PreHashWordList;

		// 最初にパターンが正常かテスト
		if (Proxydomo::CMatcher::CreateMatcher(pattern) == nullptr)
			return false;

		UnicodeString pat(pattern.c_str(), (int32_t)pattern.length());
		StringCharacterIterator patternIt(pat);
		for (; patternIt.current() != patternIt.DONE && patternIt.getIndex() < kMaxPreHashWordLength; patternIt.next()) {
			auto& pmapChildHashWord = (*pmapPreHashWord)[towlower(patternIt.current())];
			if (pmapChildHashWord == nullptr) {
				pmapChildHashWord.reset(new HashedListCollection::PreHashWord);
			}

			if (patternIt.hasNext() == false || ((patternIt.getIndex() + 1) == kMaxPreHashWordLength)) {
				patternIt.next();
				try {
					pmapChildHashWord->vecNode.emplace_back(std::shared_ptr<Proxydomo::CNode>(Proxydomo::CMatcher::expr(patternIt)), listLine);
				}
				catch (...) {
					return false;
				}
				pmapChildHashWord->vecNode.back().node->setNextNode(nullptr);
				return true;
			}
			pmapPreHashWord = &pmapChildHashWord->mapChildPreHashWord;
		}

	}

	// URLハッシュ
	std::size_t slashPos = pattern.find(L'/');
	if (slashPos != std::wstring::npos && slashPos != 0) {
		std::wstring urlHost = pattern.substr(0, slashPos);
		if (urlHost.length() >= 5) {
			auto funcSplitDomain = [&urlHost](std::wstring::const_iterator it) -> std::deque<std::wstring> {
				auto itbegin = it;
				std::deque<std::wstring>	deqDomain;
				std::wstring domain;
				for (; it != urlHost.cend(); ++it) {
					if (isNonWildWord(*it) == false) {
						deqDomain.clear();	// fail
						break;
					}
					if (*it == L'.') {
						if (std::next(urlHost.cbegin()) == it)	// *.の場合無視する
							continue;
						if (it != itbegin && domain.empty()) {	// ..が続く場合
							ATLASSERT(FALSE);
							deqDomain.clear();	// fail
							break;
						}
						deqDomain.push_back(domain);
						domain.clear();

					} else if (std::next(it) == urlHost.cend()) {
						domain.push_back(towlower(*it));
						deqDomain.push_back(domain);
						domain.clear();
						break;

					} else {
						domain.push_back(towlower(*it));
					}
				}
				ATLASSERT(domain.empty());
				return deqDomain;
			};

			auto funcGetFirstWildcard = [&urlHost](std::wstring::const_iterator& it) -> std::wstring {
				for (; it != urlHost.cend(); ++it) {
					if (*it == L'.') {
						std::wstring firstWildcard(urlHost.cbegin(), it);
						if (Proxydomo::CMatcher::CreateMatcher(firstWildcard) == nullptr) {
							return L"";
						}
						++it;
						return firstWildcard;
					}
				}
				return L"";
			};

			wchar_t first = urlHost.front();
			std::wstring firstWildcard;
			std::deque<std::wstring>	deqDomain;
			switch (first) {
				case L'*':
				{
					auto it = std::next(urlHost.cbegin());
					firstWildcard = funcGetFirstWildcard(it);
					deqDomain = funcSplitDomain(it);
				}
				break;

				case L'(':
				{
					int nkakko = 1;
					for (auto it = std::next(urlHost.cbegin()); it != urlHost.cend(); ++it) {
						if (*it == L'(') {
							++nkakko;
						} else if (*it == L')') {
							nkakko--;
							if (nkakko == 0) {
								std::advance(it, 1);
								firstWildcard = funcGetFirstWildcard(it);
								deqDomain = funcSplitDomain(it);
								break;
							}
						}
					}
				}
				break;

				case L'[':
				{
					for (auto it = std::next(urlHost.cbegin()); it != urlHost.cend(); ++it) {
						if (*it == L']') {
							if (*std::prev(it) == L'\\')	// \]は無視
								continue;
							auto next1 = std::next(it);
							if (next1 != urlHost.cend() && *next1 == L'+') {	// ]+
								auto next2 = std::next(next1);
								if (next2 != urlHost.cend() && *next2 == L'+') {	// ]++
									std::advance(next2, 1);
									firstWildcard = funcGetFirstWildcard(next2);
									deqDomain = funcSplitDomain(next2);
									break;
								} else {
									std::advance(next1, 1);
									firstWildcard = funcGetFirstWildcard(next1);
									deqDomain = funcSplitDomain(next1);
									break;
								}
							}
							std::advance(it, 1);
							firstWildcard = funcGetFirstWildcard(it);	// ]
							deqDomain = funcSplitDomain(it);
							break;
						}
					}
				}
				break;

				case L'\\':
				{
					if (*std::next(urlHost.cbegin()) == L'w') {
						auto it = std::next(urlHost.cbegin(), 2);
						firstWildcard = funcGetFirstWildcard(it);
						deqDomain = funcSplitDomain(it);
					}
				}
				break;
			}
			if (firstWildcard.length() > 0 && deqDomain.size() > 0) {
				// 最初にパターンが正常かテスト
				if (Proxydomo::CMatcher::CreateMatcher(pattern) == nullptr)
					return false;

				std::wstring lastWildcard = pattern.substr(slashPos + 1);
				auto	pmapChildURLHash = &listCollection.URLHashList;
				for (auto it = deqDomain.rbegin(); it != deqDomain.rend(); ++it) {
					auto& pURLHash = (*pmapChildURLHash)[*it];
					if (pURLHash == nullptr)
						pURLHash.reset(new HashedListCollection::URLHash);

					if (std::next(it) == deqDomain.rend()) {
						UnicodeString patfirst(firstWildcard.c_str(), (int32_t)firstWildcard.length());
						StringCharacterIterator patternfirstIt(patfirst);
						UnicodeString patlast(lastWildcard.c_str(), (int32_t)lastWildcard.length());
						StringCharacterIterator patternlastIt(patlast);
						pURLHash->vecpairNode.emplace_back(
							std::shared_ptr<Proxydomo::CNode>(Proxydomo::CMatcher::expr(patternfirstIt)),
							std::shared_ptr<Proxydomo::CNode>(Proxydomo::CMatcher::expr(patternlastIt)),
							listLine);
						auto& pair = pURLHash->vecpairNode.back();
						pair.nodeFirst->setNextNode(nullptr);
						pair.nodeLast->setNextNode(nullptr);
						return true;
					}
					pmapChildURLHash = &pURLHash->mapChildURLHash;
				}
			}
		}
	}

    // parse the pattern
	try {
		UnicodeString pat(pattern.c_str(), (int32_t)pattern.length());
		StringCharacterIterator patternIt(pat);
		listCollection.deqNormalNode.emplace_back(std::shared_ptr<Proxydomo::CNode>(Proxydomo::CMatcher::expr(patternIt)), listLine);
	} catch (...) {
		return false;
	}

	listCollection.deqNormalNode.back().node->setNextNode(nullptr);
	return true;
}