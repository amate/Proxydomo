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


#ifndef __textfilter__
#define __textfilter__

#include "filter.h"
#include "receptor.h"
#include <string>
class CFilterDescriptor;
class CMatcher;

using namespace std;

/* class CTextFilter
 */
class CTextFilter : public CFilter {

public:

    // Constructor
    CTextFilter(CFilterOwner& owner, const CFilterDescriptor& desc);

    // Destructor
    ~CTextFilter();
    
    // Reset the filter (tests the URL for self-disabling)
    void reset();
    
    // Informs the filter that 'stop' in match() is the end of stream
    void endReached();

    // Run the filter on string [index,bufTail)
    // Returns 1 (match), 0 (fail) or -1 (needs more data)
    int match(const char* index, const char* bufTail);
    
    // Generates the replacement text for the previous occurrence
    string getReplaceText();

    // If we match, end position of occurrence
    const char* endOfMatched;

    // If we match, shall we replace the matched text in the buffer
    // instead of sending it to the output stream?
    bool multipleMatches;

    // Characters at which the filter is worth trying. The CTextBuffer
    // will look at it before calling match()
    bool okayChars[256];

private:

    // For special <start> and <end> match patterns
    bool isSpecial;
    bool isForStart;

    // true if we are reached the end of the stream
    bool isComplete;

    // filter owner (the object which contains header values and variables)
    CFilterOwner& owner;

    // Window size
    int windowWidth;
    
    // URL matcher
    CMatcher* urlMatcher;

    // Bounds matcher, if needed
    CMatcher* boundsMatcher;
    
    // Text matcher
    CMatcher* textMatcher;
    
    // Replace pattern
    string replacePattern;
};

#endif
