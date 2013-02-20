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


#include "filter.h"

/* Constructor
 */
CFilter::CFilter(CFilterOwner& owner) : owner(owner), bypassed(false), locked(false) {
}


/* Check and unlock the filter mutex
 */
void CFilter::unlock() {
    if (locked) {        // The locking must stop after each matching
        //CLog::ref().filterLock.Unlock();
        locked = false;
    }
}


/* Clear memory table (if used) and memory stack
 */
void CFilter::clearMemory() {
    for (int i=0; i<10; i++)   memoryTable[i].clear();
    if (!memoryStack.empty())  memoryStack.clear();
}

