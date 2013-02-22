/**
*	@file	FilterOwner.cpp
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
#include "stdafx.h"
#include "FilterOwner.h"
#include "proximodo\util.h"


std::string	CFilterOwner::GetHeader(const HeadPairList& headers, const std::string& name)
{
    for (auto it = headers.cbegin(); it != headers.cend(); it++) {
		if (CUtil::noCaseEqual(name, it->first)) 
			return it->second;
    }
    return "";
}

void		CFilterOwner::SetHeader(HeadPairList& headers, const std::string& name, const std::string& value)
{
	auto it = headers.begin();
    for (; it != headers.end(); it++) {
        if (CUtil::noCaseEqual(name, it->first)) {
            it->second = value;
            break;
        }
    }
    if (it == headers.end()) {
        headers.emplace_back(name, value);
    } else {
        ++it;
        auto it2 = it;
        while (it2 != headers.end()) {
            if (CUtil::noCaseEqual(name, it2->first)) {
                headers.erase(it2);
                it2 = it;
            } else {
                it2++;
            }
        }
    }
}

void		CFilterOwner::RemoveHeader(HeadPairList& headers, const std::string& name)
{
	for (auto it = headers.begin(); it != headers.end(); ++it) {
        if (CUtil::noCaseEqual(name, it->first)) {
            headers.erase(it);
            it = headers.begin();
        }
    }
}


/* Remove headers with empty value from a vector of headers.
 */
void CFilterOwner::CleanHeader(HeadPairList& headers)
{
	for (auto it = headers.begin(); it != headers.end(); ++it) {
		if (it->second.empty()) {
            headers.erase(it);
            it = headers.begin();
        }
    }    
}

