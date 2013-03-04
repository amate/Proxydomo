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
#include <atlsync.h>
#include <atlstr.h>
#include "FilterDescriptor.h"

namespace Proxydomo { class CNode; }


struct HashedListCollection {
	std::recursive_mutex	mutexHashedArray;
	struct SListItem {
		std::shared_ptr<Proxydomo::CNode>	node;
		int	flags;	// 0x1: is a tilde pattern,  0x2: is hashable
	};
	std::shared_ptr<std::array<std::deque<SListItem>, 256>>	spHashedArray;
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

	static void	LoadSettings();
	static void	SaveSettings();

	// CCritSecLock	lock(CSettings::s_csFilters);
	static std::vector<std::unique_ptr<CFilterDescriptor>>	s_vecpFilters;
	static CCriticalSection									s_csFilters;

	// std::lock_guard<std::recursive_mutex> lock(CSettings::s_mutexHashedLists);
	static std::recursive_mutex								s_mutexHashedLists;
	static std::unordered_map<std::string, std::unique_ptr<HashedListCollection>>	s_mapHashedLists;

	static void	LoadFilter();
	static void SaveFilter();

	static void LoadList(const CString& filePath);
private:
	static void _CreatePattern(const std::wstring& pattern, 
							   std::array<std::deque<HashedListCollection::SListItem>, 256>& hashed);

};