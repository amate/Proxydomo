/**
*	@file	Settings.h
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
#pragma once

#include <vector>
#include <deque>
#include <unordered_map>
#include <memory>
#include <string>
#include <mutex>
#include <array>
#include <thread>
#include <atlsync.h>
#include <atlstr.h>
#include <boost\thread.hpp>	// for shared_mutex
#include "FilterDescriptor.h"

namespace Proxydomo { class CNode; }

/// １つのリストにあるすべてのパータンを持っている
struct HashedListCollection {
	boost::shared_mutex	mutex;

	// 固定プレフィックスリスト
	struct PreHashWord {
		std::unordered_map<wchar_t, std::unique_ptr<PreHashWord>>	mapChildPreHashWord;
		std::vector<std::shared_ptr<Proxydomo::CNode>>	vecNode;
	};
	std::unordered_map<wchar_t, std::unique_ptr<PreHashWord>>	PreHashWordList;

	// URLハッシュリスト
	struct URLHash {
		std::unordered_map<std::wstring, std::unique_ptr<URLHash>>	mapChildURLHash;
		std::vector<std::pair<std::shared_ptr<Proxydomo::CNode>, std::shared_ptr<Proxydomo::CNode>>>	vecpairNode;
	};
	std::unordered_map<std::wstring, std::unique_ptr<URLHash>>	URLHashList;

	// Normalリスト
	std::deque<std::shared_ptr<Proxydomo::CNode>>	deqNormalNode;
};

struct FilterItem
{
	CString	name;
	bool	active;
	std::unique_ptr<CFilterDescriptor>	pFilter;
	std::unique_ptr<std::vector<std::unique_ptr<FilterItem>>>	pvecpChildFolder;

	FilterItem() : active(false) { }

	FilterItem(const FilterItem& item) : name(item.name), active(item.active)
	{
		if (item.pFilter)
			pFilter.reset(new CFilterDescriptor(*item.pFilter));
	}
};

class CSettings
{
public:
	static uint16_t		s_proxyPort;
	static bool			s_filterText;
	static bool			s_filterIn;
	static bool			s_filterOut;

	static bool			s_WebFilterDebug;

	static char			s_urlCommandPrefix[16];

	static std::thread	s_threadSaveFilter;
	
	static void	LoadSettings();
	static void	SaveSettings();

	// CCritSecLock	lock(CSettings::s_csFilters);
	static std::vector<std::unique_ptr<FilterItem>>	s_vecpFilters;
	static CCriticalSection									s_csFilters;

	/// アクティブなフィルターを返す (※ lockは必要ない)
	static void EnumActiveFilter(std::function<void (CFilterDescriptor*)> func);

	// std::lock_guard<std::recursive_mutex> lock(CSettings::s_mutexHashedLists);
	static std::recursive_mutex								s_mutexHashedLists;
	static std::unordered_map<std::string, std::unique_ptr<HashedListCollection>>	s_mapHashedLists;

	static void	LoadFilter();
	static void SaveFilter();

	static void LoadList(const CString& filePath);
private:
	static void _CreatePattern(std::wstring& pattern, HashedListCollection& listCollection);

};