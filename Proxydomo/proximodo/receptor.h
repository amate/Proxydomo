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


#ifndef __receptor__
#define __receptor__

#include <string>

using namespace std;

/* class CDataReceptor
 * This virtual class is for classes that can receive data.
 * It allows chaining text processors. The last CDataReceptor of the
 * chain is not a text processor but the request manager itself.
 */
class CDataReceptor {

public:
    virtual ~CDataReceptor() { }
    virtual void dataReset() =0;
    virtual void dataFeed(const string& data) =0;
    virtual void dataDump() =0;
};

#endif
