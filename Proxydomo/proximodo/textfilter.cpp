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


#include "textfilter.h"
#include "url.h"
#include "expander.h"
#include "const.h"
#include "..\Log.h"
#include "..\FilterDescriptor.h"
#include "..\FilterOwner.h"
//#include "matcher.h"
#include "..\Node.h"
#include "..\Matcher.h"
#include <vector>
#include <map>
#include <sstream>

using namespace std;

/* Constructor
 */
CTextFilter::CTextFilter(CFilterOwner& owner, const CFilterDescriptor& desc) :
                                CFilter(owner), owner(owner)
{
    title           =  desc.title;
    windowWidth     =  desc.windowWidth;
    multipleMatches =  desc.multipleMatches;
    replacePattern  =  desc.replacePattern;

    if (!desc.urlPattern.empty()) {
		urlMatcher = desc.spURLMatcher;
        // (it can throw a parsing_exception)
    }

    isForStart = isSpecial = false;
    if (desc.matchPattern == L"<start>") {
        isSpecial = true;
        isForStart = true;
        memset(okayChars, 0xFF, sizeof(okayChars)); // match whatever the first char is
        return;
    } else if (desc.matchPattern == L"<end>") {
        isSpecial = true;
        memset(okayChars, 0x00, sizeof(okayChars)); // can't match as long as there is a char
        return;
    }
    
	textMatcher = desc.spTextMatcher;
    textMatcher->mayMatch(okayChars);

    if (!desc.boundsPattern.empty()) {
		boundsMatcher = desc.spBoundsMatcher;
        bool tab[256];
        boundsMatcher->mayMatch(tab);
        for (int i=0; i<256; i++) okayChars[i] = okayChars[i] && tab[i];
    }
}


/* Destructor
 */
CTextFilter::~CTextFilter() {
}


/* Generates the replacement text for the previous occurrence
 */
std::wstring CTextFilter::getReplaceText() {

    std::wstring text = CExpander::expand(replacePattern, *this);
    unlock();
    return text;
}


/* Reset internal state and get ready for a new stream
 */
void CTextFilter::reset() {

    clearMemory();
    bypassed = false;
    isComplete = false;

    if (urlMatcher) {
        // The filter will be inactive if the URL does not match
        bypassed = !urlMatcher->match(owner.url.getFromHost(), this);
        unlock();
    }
}


/* Informs the filter that 'stop' in match() is the end of stream
 */
void CTextFilter::endReached() {

    isComplete = true;
}


/* Run the filter on string [start,stop)
 */
int CTextFilter::match(const wchar_t* index, const wchar_t* bufTail) {

    // for special <start> and <end> filters
    if (isSpecial) {
        endOfMatched = index;
        return (isForStart || (isComplete && index == bufTail ? (bypassed = true, 1) : 0));
    }
    
    // compute up to where we want to match
    const wchar_t* reached = nullptr;
	const wchar_t* stop = index + windowWidth;
    if (stop > bufTail) 
		stop = bufTail;

    // clear memory
    clearMemory();

	Proxydomo::MatchData matchData(this);

    if (boundsMatcher) {
        // let's try and find the bounds first
        bool matched = boundsMatcher->match(index, stop, endOfMatched, &matchData);
        unlock();
        
        // could have had a different result with more data, we'll wait for it
        if (matchData.reached == bufTail && !isComplete) return -1;
        // bounds not matching
        if (!matched) return 0;
        // bounds matching: we'll limit matching to what was found
        stop = endOfMatched;
    }

    // try matching
    bool matched = textMatcher->match(index, stop, endOfMatched, &matchData);
    unlock();

    if (matchData.reached == bufTail && !isComplete) 
		return -1;
    if (!matched || (boundsMatcher && endOfMatched != stop)) 
		return 0;

	for (auto& matchListLog : matchData.matchListLog) {
		CLog::FilterEvent(kLogFilterListMatch, owner.requestNumber, matchListLog.first, std::to_string(matchListLog.second));
	}
    return 1;
}
