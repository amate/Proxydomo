//------------------------------------------------------------------
//
//this file is part of Proximodo
//Copyright (C) 2004-2005 Antony BOUCHER ( kuruden@users.sourceforge.net )
//                        Paul Rupe ( prupe@users.sourceforge.net )
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

#ifndef __textbuffer__
#define __textbuffer__

#include "receptor.h"
#include <vector>
#include <string>
class CFilterOwner;
class CTextFilter;

using namespace std;

/* This class encapsulates a string buffer and processes data
 * with the text filters. Processed data is automatically forwarded
 * to the provided CDataReceptor object.
 */
class CTextBuffer : public CDataReceptor {

public:
    // Constructor
    CTextBuffer(CFilterOwner& owner, CDataReceptor* output);
    
    // Destructor
    ~CTextBuffer();

    // CDataReceptor methods
    void dataReset();
    void dataFeed(const string& data);
    void dataDump();

private:
    // the object which contains header values and variables
    CFilterOwner& owner;
    
    // where to send processed data
    CDataReceptor* output;

    // the filters
    vector<CTextFilter*> TEXTfilters;
    
    // the next filter to try (if a filter needs more data,
    // we'll start with it on next data arrival)
    vector<CTextFilter*>::iterator currentFilter;

    // pass string to output, escaping HTML chars as needed
    void escapeOutput(stringstream& out, const char *data, size_t len);
    
    // the actual buffer
    string buffer;
};

#endif
// vi:ts=4:sw=4:et
