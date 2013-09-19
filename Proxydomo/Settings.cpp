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
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#include "Misc.h"
#include "proximodo\util.h"
//#include "proximodo\matcher.h"
#include "Log.h"
#include "Matcher.h"
#include "CodeConvert.h"

using namespace CodeConvert;
using namespace boost::property_tree;

template <class _Function>
bool ForEachFile(const CString &strDirectoryPath, _Function __f)
{
	CString 		strPathFind = strDirectoryPath;

	::PathAddBackslash(strPathFind.GetBuffer(MAX_PATH));
	strPathFind.ReleaseBuffer();

	CString 		strPath 	= strPathFind;
	strPathFind += _T("*.*");

	WIN32_FIND_DATA wfd;
	HANDLE	h = ::FindFirstFile(strPathFind, &wfd);
	if (h == INVALID_HANDLE_VALUE)
		return false;

	// Now scan the directory
	do {
		// it is a file
		if ( ( wfd.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM) ) == 0 ) {
			__f(strPath + wfd.cFileName);
		}
	} while ( ::FindNextFile(h, &wfd) );

	::FindClose(h);

	return true;
}


////////////////////////////////////////////////////////////
// CSettings

enum { DEFAULTPECA_PORT = 6060 };
uint16_t		CSettings::s_proxyPort	= DEFAULTPECA_PORT;
bool			CSettings::s_filterText	= true;
bool			CSettings::s_filterIn	= true;
bool			CSettings::s_filterOut	= true;

bool			CSettings::s_WebFilterDebug	= false;

char			CSettings::s_urlCommandPrefix[16] = {};

std::thread		CSettings::s_threadSaveFilter;

std::vector<std::unique_ptr<FilterItem>>	CSettings::s_vecpFilters;
CCriticalSection							CSettings::s_csFilters;

std::recursive_mutex						CSettings::s_mutexHashedLists;
std::unordered_map<std::string, std::unique_ptr<HashedListCollection>>	CSettings::s_mapHashedLists;



void	CSettings::LoadSettings()
{
	std::ifstream fs(Misc::GetExeDirectory() + _T("settings.ini"));
	if (fs) {
		ptree pt;
		read_ini(fs, pt);
		if (auto value = pt.get_optional<uint16_t>("Setting.ProxyPort"))
			s_proxyPort	= value.get();
		if (auto value = pt.get_optional<bool>("Setting.filterText"))
			s_filterText	= value.get();
		if (auto value = pt.get_optional<bool>("Setting.filterIn"))
			s_filterIn	= value.get();
		if (auto value = pt.get_optional<bool>("Setting.filterOut"))
			s_filterOut	= value.get();
	}

	// prefixを設定
	static const char charactorSelection[] = "abcdefghijklmnopqrstuvqxyz0123456789_";
	std::random_device	randEngine;
	std::uniform_int_distribution<int> dist(0, _countof(charactorSelection) - 2);
	for (auto& c : s_urlCommandPrefix)
		c = charactorSelection[dist(randEngine)];

	CSettings::LoadFilter();

	ForEachFile(Misc::GetExeDirectory() + _T("lists\\"), [](const CString& filePath) {
		LoadList(filePath);
	});
}

void	CSettings::SaveSettings()
{
	std::string settingsPath = CT2A(Misc::GetExeDirectory() + _T("settings.ini"));
	ptree pt;
	try {
		read_ini(settingsPath, pt);
	} catch (...) {
	}

	pt.put("Setting.ProxyPort", s_proxyPort);
	pt.put("Setting.filterText"	, s_filterText);
	pt.put("Setting.filterIn"	, s_filterIn);
	pt.put("Setting.filterOut"	, s_filterOut);

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
			pFilter->headerName	= UTF8fromUTF16(ptFilter.get(L"headerName", L""));
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
		MessageBox(NULL, _T("filter.xmlのオープンに失敗"), NULL, MB_ICONERROR);
		return ;
	}
	fs.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t>));

	wptree pt;
	try {
		read_xml(fs, pt);
	} catch (...) {
		return ;
	}
	if (auto& opChild = pt.get_child_optional(L"ProxydomoFilter")) {
		wptree& ptChild = opChild.get();
		LoadFilterItem(ptChild, s_vecpFilters);
	}
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
			ptFilter.put(L"headerName", UTF16fromUTF8(filter->headerName));
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

					write_xml(fs, pt, xml_parser::xml_writer_make_settings(L' ', 2, xml_parser::widen<wchar_t>("UTF-8")));

					fs.close();
					::MoveFileEx(tempPath, filterPath, MOVEFILE_REPLACE_EXISTING);

				}
				catch (const boost::property_tree::ptree_error& error) {
					CString strError = _T("filter.xmlへの書き込みに失敗\n");
					strError += error.what();
					MessageBox(NULL, strError, NULL, MB_ICONERROR);
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

void CSettings::EnumActiveFilter(std::function<void (CFilterDescriptor*)> func)
{
	CCritSecLock	lock(CSettings::s_csFilters);
	ActiveFilterCallFunc(s_vecpFilters, func);
}


static bool isHashable(char c) {
    return (c != '\\' && c != '[' && c != '$'  && c != '('  && c != ')'  &&
            c != '|'  && c != '&' && c != '?'  && c != ' '  && c != '\t' &&
            c != '='  && c != '*' && c != '\'' && c != '\"' );
}

static inline int hashBucket(char c)
    { return tolower(c) & 0xff; }

void CSettings::LoadList(const CString& filePath)
{
	// 最終書き込み時刻を取得
	HANDLE hFile = CreateFile(filePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) 
		return ;

	FILETIME lastWriteTime = {};
	::GetFileTime(hFile, NULL, NULL, &lastWriteTime);
	uint64_t time = lastWriteTime.dwHighDateTime;
	time <<= 32;
	time |= lastWriteTime.dwLowDateTime;
	::CloseHandle(hFile);

	std::string filename = CT2A(Misc::GetFileBaseNoExt(filePath));
	static std::unordered_map<std::string, uint64_t> s_mapFileLastWriteTime;
	uint64_t& prevLastWriteTime = s_mapFileLastWriteTime[filename];
	if (prevLastWriteTime == time)
		return ;	// 更新されていなかったら帰る
	prevLastWriteTime = time;

	std::wifstream fs(filePath);
	if (!fs)
		return ;
	fs.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t>));
	{
		s_mutexHashedLists.lock();
		auto& hashedLists = s_mapHashedLists[filename];
		if (hashedLists == nullptr)
			hashedLists.reset(new HashedListCollection);
		s_mutexHashedLists.unlock();

		std::shared_ptr<std::array<std::deque<HashedListCollection::SListItem>, 256>>	
			spHashedArray(new std::array<std::deque<HashedListCollection::SListItem>, 256>);

		std::wstring strLine;
		while (std::getline(fs, strLine).good()) {
			CUtil::trim(strLine);
			if (strLine.size() > 0 && strLine[0] != L'#') {
				_CreatePattern(strLine, *spHashedArray);
			}
		}
		std::lock_guard<std::recursive_mutex> lock(hashedLists->mutexHashedArray);
		hashedLists->spHashedArray = spHashedArray;
	}
	CLog::FilterEvent(kLogFilterListReload, -1, filename, "");
}


void CSettings::_CreatePattern(const std::wstring& pattern, 
							   std::array<std::deque<HashedListCollection::SListItem>, 256>& hashed)
{
    // we'll record the built node and its flags in a structure
    HashedListCollection::SListItem item = { NULL, 0 };
    if (pattern[0] == L'~') 
		item.flags |= 0x1;
    int start = (item.flags & 0x1) ? 1 : 0;
    wchar_t c = pattern[start];
    if (isHashable((char)c)) item.flags |= 0x2;
    // parse the pattern
	try {
		UnicodeString pat(pattern.c_str(), pattern.length());
		StringCharacterIterator patternIt(pat);
		item.node.reset(Proxydomo::CMatcher::expr(patternIt));
	} catch (...) {
		return ;
	}

    item.node->setNextNode(NULL);
    
    if (item.flags & 0x2) {
        // hashable pattern goes in a single hashed bucket (lowercase)
        hashed[hashBucket((char)c)].push_back(item);
    } else {
        // Expressions that start with a special char cannot be hashed
        // Push them on every hashed list
        for (size_t i = 0; i < hashed.size(); i++) {
            if (!isupper((char)i))
                hashed[i].push_back(item);
		}
    }
}