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
#include "log.h"
#include "descriptor.h"
#include "filterowner.h"
#include "matcher.h"
#include <vector>
#include <map>
#include <sstream>

using namespace std;

/* Constructor
 */
CTextFilter::CTextFilter(CFilterOwner& owner, const CFilterDescriptor& desc) :
                                CFilter(owner), owner(owner) {

    textMatcher = boundsMatcher = urlMatcher = NULL;

    title           =  desc.title;
    windowWidth     =  desc.windowWidth;
    multipleMatches =  desc.multipleMatches;
    replacePattern  =  desc.replacePattern;

    if (!desc.urlPattern.empty()) {
        urlMatcher = new CMatcher(desc.urlPattern, *this);
        // (it can throw a parsing_exception)
    }

    isForStart = isSpecial = false;
    if (desc.matchPattern == "<start>") {
        isSpecial = true;
        isForStart = true;
        memset(okayChars, 0xFF, sizeof(okayChars)); // match whatever the first char is
        return;
    } else if (desc.matchPattern == "<end>") {
        isSpecial = true;
        memset(okayChars, 0x00, sizeof(okayChars)); // can't match as long as there is a char
        return;
    }
    
    try {
        textMatcher = new CMatcher(desc.matchPattern, *this);
        textMatcher->mayMatch(okayChars);
    } catch (parsing_exception e) {
        if (urlMatcher) delete urlMatcher;
        throw e;
    }

    if (!desc.boundsPattern.empty()) {
        try {
            boundsMatcher = new CMatcher(desc.boundsPattern, *this);
            bool tab[256];
            boundsMatcher->mayMatch(tab);
            for (int i=0; i<256; i++) okayChars[i] = okayChars[i] && tab[i];
        } catch (parsing_exception e) {
            if (urlMatcher) delete urlMatcher;
            if (textMatcher) delete textMatcher;
            throw e;
        }
    }
}


/* Destructor
 */
CTextFilter::~CTextFilter() {

    if(urlMatcher)    delete urlMatcher;
    if(textMatcher)   delete textMatcher;
    if(boundsMatcher) delete boundsMatcher;
}


/* Generates the replacement text for the previous occurrence
 */
string CTextFilter::getReplaceText() {

    string text = CExpander::expand(replacePattern, *this);
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
        const char *tStart, *tStop, *tEnd, *tReached;
        tStart = owner.url.getFromHost().c_str();
        tStop = tStart + owner.url.getFromHost().size();
        bypassed = !urlMatcher->match(tStart, tStop, tEnd, tReached);
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
int CTextFilter::match(const char* index, const char* bufTail) {

    // for special <start> and <end> filters
    if (isSpecial) {
        endOfMatched = index;
        return (isForStart || (isComplete && index == bufTail ? (bypassed = true, 1) : 0));
    }
    
    // compute up to where we want to match
    const char *reached, *stop = index + windowWidth;
    if (stop > bufTail) stop = bufTail;

    // clear memory
    clearMemory();

    if (boundsMatcher) {
        // let's try and find the bounds first
        bool matched = boundsMatcher->match(index, stop, endOfMatched, reached);
        unlock();
        
        // could have had a different result with more data, we'll wait for it
        if (reached == bufTail && !isComplete) return -1;
        // bounds not matching
        if (!matched) return 0;
        // bounds matching: we'll limit matching to what was found
        stop = endOfMatched;
    }

    // try matching
    bool matched = textMatcher->match(index, stop, endOfMatched, reached);
    unlock();

    if (reached == bufTail && !isComplete) return -1;
    if (!matched || (boundsMatcher && endOfMatched != stop)) return 0;
    return 1;
}
