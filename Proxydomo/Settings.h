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
#include <set>
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

namespace Proxydomo { 
	class CNode;
	class CMatcher;
}

/// １つのリストにあるすべてのパターンを持っている
struct HashedListCollection {
	boost::shared_mutex	mutex;

	struct NodeData {
		std::shared_ptr<Proxydomo::CNode> node;
		int	listLine;

		NodeData(std::shared_ptr<Proxydomo::CNode> node, int listLine) : node(node), listLine(listLine) {}
	};

	struct NodePairData {
		std::shared_ptr<Proxydomo::CNode> nodeFirst;
		std::shared_ptr<Proxydomo::CNode> nodeLast;
		int	listLine;

		NodePairData(std::shared_ptr<Proxydomo::CNode> nodeFirst, std::shared_ptr<Proxydomo::CNode> nodeLast, int listLine) 
			: nodeFirst(nodeFirst), nodeLast(nodeLast), listLine(listLine) {}
	};

	// 固定プレフィックスリスト
	struct PreHashWord {
		std::unordered_map<wchar_t, std::unique_ptr<PreHashWord>>	mapChildPreHashWord;
		std::vector<NodeData>	vecNode;
	};
	std::unordered_map<wchar_t, std::unique_ptr<PreHashWord>>	PreHashWordList;

	// URLハッシュリスト
	struct URLHash {
		std::unordered_map<std::wstring, std::unique_ptr<URLHash>>	mapChildURLHash;
		std::vector<NodePairData>	vecpairNode;
	};
	std::unordered_map<std::wstring, std::unique_ptr<URLHash>>	URLHashList;

	// Normalリスト
	std::deque<NodeData>	deqNormalNode;
};

struct FilterItem
{
	CString	name;
	bool	active;
	std::unique_ptr<CFilterDescriptor>	pFilter;
	std::unique_ptr<std::vector<std::unique_ptr<FilterItem>>>	pvecpChildFolder;

	FilterItem() : active(false) { }

	FilterItem(std::unique_ptr<CFilterDescriptor>&& filter) : 
		name(filter->title.c_str()), active(filter->Active), pFilter(std::move(filter))
	{
	}

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
	static bool			s_privateConnection;
	static bool			s_filterText;
	static bool			s_filterIn;
	static bool			s_filterOut;
	static bool			s_bypass;
	static bool			s_SSLFilter;
	static bool			s_WebFilterDebug;

	static bool			s_useRemoteProxy;
	static std::string	s_defaultRemoteProxy;
	static std::set<std::string> s_setRemoteProxy;

	static std::string	s_urlCommandPrefix;

	static std::wstring	s_language;

	static bool			s_tasktrayOnCloseBotton;

	static std::thread	s_threadSaveFilter;
	
	static void	LoadSettings();
	static void	SaveSettings();

	// CCritSecLock	lock(CSettings::s_csFilters);
	static CCriticalSection		s_csFilters;
	// 全所有フィルター
	static std::vector<std::unique_ptr<FilterItem>>	s_vecpFilters;

	// Bypass フィルター
	static std::shared_ptr<Proxydomo::CMatcher>	s_pBypassMatcher;

	/// アクティブなフィルターを返す (※ lockは必要ない)
	static void EnumActiveFilter(std::function<void (CFilterDescriptor*)> func);

	// std::lock_guard<std::recursive_mutex> lock(CSettings::s_mutexHashedLists);
	static std::recursive_mutex								s_mutexHashedLists;
	static std::unordered_map<std::string, std::unique_ptr<HashedListCollection>>	s_mapHashedLists;

	static void	LoadFilter();
	static void SaveFilter();

	static void LoadList(const CString& filePath);
private:
	static bool _CreatePattern(std::wstring& pattern, HashedListCollection& listCollection, int listLine);

};