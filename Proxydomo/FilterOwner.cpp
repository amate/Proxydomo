/**
*	@file	FilterOwner.cpp
*	
*/

#include "stdafx.h"
#include "FilterOwner.h"

namespace CUtil {
	bool noCaseEqual(const std::string& s1, const std::string& s2);
}

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