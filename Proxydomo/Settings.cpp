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
#include <boost\property_tree\ptree.hpp>
#include <boost\property_tree\json_parser.hpp>
#include <boost\property_tree\ini_parser.hpp>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#include "Misc.h"
#include "proximodo\util.h"
//#include "proximodo\matcher.h"
#include "Log.h"
#include "Matcher.h"


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


std::vector<std::unique_ptr<CFilterDescriptor>>	CSettings::s_vecpFilters;
CCriticalSection								CSettings::s_csFilters;

std::recursive_mutex							CSettings::s_mutexHashedLists;
std::unordered_map<std::string, std::unique_ptr<HashedListCollection>>	CSettings::s_mapHashedLists;

using namespace boost::property_tree;

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

void CSettings::LoadFilter()
{
	CString filterPath = Misc::GetExeDirectory() + _T("filter.json");
	if (::PathFileExists(filterPath) == FALSE)
		return ;

	std::ifstream	fs(filterPath);
	if (!fs) {
		MessageBox(NULL, _T("filter.jsonのオープンに失敗"), NULL, MB_ICONERROR);
		return ;
	}

	ptree pt;
	read_json(fs, pt);
	if (auto& opChild = pt.get_child_optional("ProxydomoFilter")) {
		ptree& ptChild = opChild.get();
		for (auto& ptIt : ptChild) {
			ptree& ptFilter = ptIt.second;
			std::unique_ptr<CFilterDescriptor> pFilter(new CFilterDescriptor);
			pFilter->Active		= ptFilter.get<bool>("Active", true);
			pFilter->title		= ptFilter.get("title", "");
			pFilter->version	= ptFilter.get("version", "");
			pFilter->author		= ptFilter.get("author", "");
			pFilter->comment	= ptFilter.get("comment", "");
			pFilter->filterType	= 
				static_cast<CFilterDescriptor::FilterType>(
					ptFilter.get<int>("filterType", (int)CFilterDescriptor::kFilterText));
			pFilter->headerName	= ptFilter.get("headerName", "");
			pFilter->multipleMatches= ptFilter.get<bool>("multipleMatches", false);
			pFilter->windowWidth	= ptFilter.get<int>("windowWidth", 128);
			pFilter->boundsPattern	= ptFilter.get("boundsPattern", "");
			pFilter->urlPattern		= ptFilter.get("urlPattern", "");
			pFilter->matchPattern	= ptFilter.get("matchPattern", "");
			pFilter->replacePattern	= ptFilter.get("replacePattern", "");
			
			pFilter->CreateMatcher();

			s_vecpFilters.push_back(std::move(pFilter));
		}
	}
}

void CSettings::SaveFilter()
{
	CString filterPath = Misc::GetExeDirectory() + _T("filter.json");
	std::ofstream	fs(filterPath);
	if (!fs) {
		MessageBox(NULL, _T("filter.jsonのオープンに失敗"), NULL, MB_ICONERROR);
		return ;
	}

	ptree pt;
	for (auto& filter : s_vecpFilters) {
		ptree ptFilter;
		ptFilter.put("Active", filter->Active);
		ptFilter.put("title", filter->title);
		ptFilter.put("version", filter->version);
		ptFilter.put("author", filter->author);
		ptFilter.put("comment", filter->comment);
		ptFilter.put("filterType", (int)filter->filterType);
		ptFilter.put("headerName", filter->headerName);
		ptFilter.put("multipleMatches", filter->multipleMatches);
		ptFilter.put("windowWidth", filter->windowWidth);
		ptFilter.put("boundsPattern", filter->boundsPattern);
		ptFilter.put("urlPattern", filter->urlPattern);
		ptFilter.put("matchPattern", filter->matchPattern);
		ptFilter.put("replacePattern", filter->replacePattern);
		pt.add_child("ProxydomoFilter.filter", ptFilter);
	}
	write_json(fs, pt);
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

	std::ifstream fs(filePath);
	if (!fs)
		return ;

	{
		s_mutexHashedLists.lock();
		auto& hashedLists = s_mapHashedLists[filename];
		if (hashedLists == nullptr)
			hashedLists.reset(new HashedListCollection);
		s_mutexHashedLists.unlock();

		std::shared_ptr<std::array<std::deque<HashedListCollection::SListItem>, 256>>	
			spHashedArray(new std::array<std::deque<HashedListCollection::SListItem>, 256>);

		std::string strLine;
		while (std::getline(fs, strLine).good()) {
			CUtil::trim(strLine);
			if (strLine.size() > 0 && strLine[0] != '#'
				/* && Proxydomo::CMatcher::CreateMatcher(strLine)*/) {
				_CreatePattern(strLine, *spHashedArray);
			}
		}
		std::lock_guard<std::recursive_mutex> lock(hashedLists->mutexHashedArray);
		hashedLists->spHashedArray = spHashedArray;
	}
	CLog::FilterEvent(kLogFilterListReload, -1, filename, "");
}


void CSettings::_CreatePattern(const std::string& pattern, 
							   std::array<std::deque<HashedListCollection::SListItem>, 256>& hashed)
{
    // we'll record the built node and its flags in a structure
    HashedListCollection::SListItem item = { NULL, 0 };
    if (pattern[0] == '~') 
		item.flags |= 0x1;
    int start = (item.flags & 0x1) ? 1 : 0;
    char c = pattern[start];
    if (isHashable(c)) item.flags |= 0x2;
    // parse the pattern
	try {
		item.node.reset(Proxydomo::CMatcher::expr(pattern, start, pattern.size()));
	} catch (...) {
		return ;
	}

    item.node->setNextNode(NULL);
    
    if (item.flags & 0x2) {
        // hashable pattern goes in a single hashed bucket (lowercase)
        hashed[hashBucket(c)].push_back(item);
    } else {
        // Expressions that start with a special char cannot be hashed
        // Push them on every hashed list
        for (size_t i = 0; i < hashed.size(); i++) {
            if (!isupper((char)i))
                hashed[i].push_back(item);
		}
    }
}