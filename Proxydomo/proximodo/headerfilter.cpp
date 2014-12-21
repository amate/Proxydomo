//------------------------------------------------------------------
//
//this file is part of Proximodo
//Copyright (C) 2004 Antony BOUCHER ( kuruden@users.sourceforge.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//------------------------------------------------------------------
// Modifications: (date, author, description)
//
//------------------------------------------------------------------


#include "headerfilter.h"
#include "url.h"
#include "expander.h"
#include "util.h"
//#include "log.h"
#include "..\Log.h"
//#include "descriptor.h"
#include "..\FilterDescriptor.h"
//#include "filterowner.h"
#include "..\FilterOwner.h"
#include "..\Matcher.h"
#include <vector>
#include <map>
#include "..\CodeConvert.h"

using namespace std;
using namespace CodeConvert;

/* Constructor
 */
CHeaderFilter::CHeaderFilter(const CFilterDescriptor& desc, CFilterOwner& owner) :
                                CFilter(owner) {

    headerName = desc.headerName;
    title = desc.title;
    urlMatcher = textMatcher = NULL;
    replacePattern = desc.replacePattern;
    if (!desc.urlPattern.empty()) {
		urlMatcher = desc.spURLMatcher;
    }
    if (!desc.matchPattern.empty()) {
		textMatcher	= desc.spTextMatcher;
    }
}


/* Destructor
 */
CHeaderFilter::~CHeaderFilter() {
}


/* Transform header according to filter description
 */
bool CHeaderFilter::filter(string& content) {

    if (bypassed || (content.empty() && textMatcher && textMatcher->isStar()))
        return false;

    // clear memory
    clearMemory();

	Proxydomo::MatchData matchData(this);
    // check if URL matches
    if (urlMatcher) {		
		std::wstring url =  UTF16fromUTF8(owner.url.getFromHost());
		const UChar* end = nullptr;
		bool ret = urlMatcher->match(url.c_str(), url.c_str() + url.length(), end, &matchData);
        unlock();
        if (!ret) return false;
    }

    // Check if content matches
    if (textMatcher) {
        this->content = content;
        bool ret = textMatcher->match(content, this);
        unlock();
        if (!ret) return false;
    }

    // Log match event
	CLog::FilterEvent(kLogFilterHeaderMatch, owner.requestNumber, UTF8fromUTF16(title), content);
                               
    // Compute new header content
    content = UTF8fromUTF16(CExpander::expand(replacePattern, *this));
    unlock();
    CUtil::trim(content);

    // Ensure the header is still standard compliant
    // PENDING: I only do the minimum amount of tests here...
    size_t index = 0;
    while ((index = content.find_first_of("\r\n", index)) != string::npos) {
        // Multiligne must be formatted as CR LF SPACE
        int pos = index+1;
        while (string(" \t\r\n").find(content[pos])) ++pos;
        content.replace(index, pos-index, "\r\n ");
        index += 3;
    }

    // Log replace event
	CLog::FilterEvent(kLogFilterHeaderReplace, owner.requestNumber, UTF8fromUTF16(title), content);

	for (auto& matchListLog : matchData.matchListLog) {
		CLog::FilterEvent(kLogFilterListMatch, owner.requestNumber, matchListLog.first, std::to_string(matchListLog.second));
	}

    return true;
}

