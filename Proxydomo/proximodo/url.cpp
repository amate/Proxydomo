//------------------------------------------------------------------
//
//this file is part of Proximodo
//Copyright (C) 2004-2005 Antony BOUCHER ( kuruden@users.sourceforge.net )
//                        Paul Rupe      ( prupe@users.sourceforge.net )
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


//#include "stdafx.h"
//#include "settings.h"
#include "url.h"
#include <windows.h>
#include "..\Settings.h"
#include "util.h"

using namespace std;

/* Constructor with parsing
 */
CUrl::CUrl(const string& str) {
    parseUrl(str);
}


/* Parses the URL string and stores the URL and each part in
 * different strings
 */
void CUrl::parseUrl(const string& str) {

    size_t pos1 = str.find("://");
    if (pos1 == string::npos) 
		pos1 = 0;
	else
		pos1 += 3;
    url       = str;
    protocol  = (pos1 ? str.substr(0, pos1-3) : string("http"));

    bypassIn = bypassOut = bypassText = debug = source = false;

	if (str.substr(pos1, _countof(CSettings::s_urlCommandPrefix)) == CSettings::s_urlCommandPrefix) {
        bool foundUrlCmd = false;        
        for (;;) {
			if (str.length() <= pos1 + _countof(CSettings::s_urlCommandPrefix))
				break;
			pos1 += _countof(CSettings::s_urlCommandPrefix);
            string begin = str.substr(pos1);
            if (CUtil::noCaseBeginsWith("bin.", begin)) {
                bypassIn = true;
                pos1 += 4;
            } else if (CUtil::noCaseBeginsWith("bout.", begin)) {
                bypassOut = true;
                pos1 += 5;
            } else if (CUtil::noCaseBeginsWith("bweb.", begin)) {
                bypassText = true;
                pos1 += 5;
            } else if (CUtil::noCaseBeginsWith("bypass.", begin)) {
                bypassIn = bypassOut = bypassText = true;
                pos1 += 7;
            } else if (CUtil::noCaseBeginsWith("dbug.", begin)) {
                debug = true;
                pos1 += 5;
            } else if (CUtil::noCaseBeginsWith("src.", begin)) {
                source = true;
                pos1 += 4;
            } else {
				if (foundUrlCmd)
					pos1 -= _countof(CSettings::s_urlCommandPrefix);
                break;
            }
            foundUrlCmd = true;
        }
        if (foundUrlCmd == false)
            pos1 -= _countof(CSettings::s_urlCommandPrefix);
    }
	debug	|= CSettings::s_WebFilterDebug;

    size_t pos2 = str.find_first_of("/?#", pos1);
    if (pos2 == string::npos) pos2 = str.length();
    size_t pos3 = str.find_first_of("?#", pos2);
    if (pos3 == string::npos) pos3 = str.length();
    size_t pos4 = str.find_first_of("#", pos3);
    if (pos4 == string::npos) pos4 = str.length();

    fromhost  = str.substr(pos1);
    afterhost = str.substr(pos2);
    host      = str.substr(pos1, pos2 - pos1);
    path      = str.substr(pos2, pos3 - pos2);
    query     = str.substr(pos3, pos4 - pos3);
    anchor    = str.substr(pos4);
    hostport  = host;
    if (hostport.find(":") == string::npos)
        hostport += ':' + protocol;
}
// vi:ts=4:sw=4:et
