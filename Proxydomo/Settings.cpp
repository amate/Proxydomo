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
#include "proximodo\matcher.h"


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

std::vector<std::unique_ptr<CFilterDescriptor>>	CSettings::s_vecpFilters;
CCriticalSection								CSettings::s_csFilters;
std::map<std::string, std::string>				CSettings::s_mapListName;	// list name : file path
std::map<std::string, std::deque<std::string>>	CSettings::s_mapLists;		// list name : contents

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
		CString filename = Misc::GetFileBaseNoExt(filePath);
		std::ifstream fs(filePath);
		if (!fs)
			return ;

		std::deque<std::string>& deqLine = CSettings::s_mapLists[std::string(CT2A(filename))];

		std::string strLine;
		while (std::getline(fs, strLine).good()) {
			CUtil::trim(strLine);
			if (strLine.size() > 0 && strLine[0] != '#' && CMatcher::testPattern(strLine))
				deqLine.emplace_back(strLine);
		}
	});
}

void	CSettings::SaveSettings()
{
	std::string settingsPath = CT2A(Misc::GetExeDirectory() + _T("settings.ini"));
	ptree pt;
	read_ini(settingsPath, pt);

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





