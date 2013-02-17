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


#ifndef __filterowner__
#define __filterowner__

#include "url.h"
#include <string>
#include <vector>
#include <map>

using namespace std;

struct SHeader { string name, value; };

/* class CFilterOwner
 * This class only contains and implements what is common to
 * filter owners (such as CRequestManager and filter tester window).
 * URL and variables are such common members.
 */
class CFilterOwner {

public:
    CUrl   url;                         // URL of the document to filter

    string responseCode;                // response code from website

    int    reqNumber;                   // number of the request
    int    cnxNumber;                   // number of the connection

    bool   useSettingsProxy;            // can be overridden by $USEPROXY
    string contactHost;                 // can be overridden by $SETPROXY
    string rdirToHost;                  // set by $RDIR and $JUMP
    int    rdirMode;                    // 0: 302 response, 1: transparent

    bool   bypassIn;                    // tells if we can filter incoming headers
    bool   bypassOut;                   // tells if we can filter outgoing headers
    bool   bypassBody;                  // will tell if we can filter the body or not
    bool   bypassBodyForced;            // set to true if $FILTER changed bypassBody

    bool   killed;                      // Has a pattern executed a \k ?

    map<string,string> variables;       // variables for $SET and $GET

    vector<SHeader> outHeaders;         // Outgoing headers
    vector<SHeader> inHeaders;          // Incoming headers
    
    string fileType;                    // useable by $TYPE

    // Clear Variables
    void clearVariables();

    // Manipulate headers
    static void    cleanHeaders(vector<SHeader>& headers);
    static string& getHeader   (vector<SHeader>& headers, const string& name);
    static void    removeHeader(vector<SHeader>& headers, const string& name);
    static void    setHeader   (vector<SHeader>& headers, const string& name,
                                                          const string& value);
};

#endif
