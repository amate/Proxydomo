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


#ifndef __filter__
#define __filter__

#include "memory.h"
#include <vector>
#include <string>

class CFilterOwner;

/* class CFilter
 * This class only contains and implements what is common to
 * text and header filters. It allows CMatcher and CGenerator
 * to interact with the filter, be it text ou header filter.
 * It also gives access to the filter owner, where variables
 * and URL lie.
 */
class CFilter {

public:
    CFilter(CFilterOwner& owner);

    CFilterOwner& owner;
    
    // Is the filter bypassed?
    bool bypassed;
    
    // Filter title (mainly for log events)
    std::wstring title;

    // CMemory for \0-9 and \#
    CMemory memoryTable[10];
    std::vector<CMemory> memoryStack;
    void clearMemory();
    
    // Indicates that the filter mutex has been locked, and should
    // be unlocked after matching/replacing
    bool locked;
    void unlock();  // to call after each match() or expand() in a filter
};

#endif
