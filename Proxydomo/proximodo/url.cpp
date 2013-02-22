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
    if (pos1 == string::npos) pos1 = 0; else pos1 += 3;
    url       = str;
    protocol  = (pos1 ? str.substr(0, pos1-3) : string("http"));

    bypassIn = bypassOut = bypassText = debug = source = false;
#if 0
    //if (CSettings::ref().enableUrlCmd &&
    //    str.substr(pos1, CSettings::ref().urlCmdPrefix.length()) ==
    //    CSettings::ref().urlCmdPrefix) {
        bool foundUrlCmd = false;
        //pos1 += CSettings::ref().urlCmdPrefix.length();
        while (1) {
            string s5 = str.substr(pos1, 5);
            string s6;
            if (s5 == "bin..") {
                bypassIn = true;
                pos1 += 5;
            } else if ((s6 = str.substr(pos1, 6)) == "bout..") {
                bypassOut = true;
                pos1 += 6;
            } else if (s6 == "bweb..") {
                bypassText = true;
                pos1 += 6;
            } else if (str.substr(pos1, 8) == "bypass..") {
                bypassIn = bypassOut = bypassText = true;
                pos1 += 8;
            } else if (s6 == "dbug..") {
                debug = true;
                pos1 += 6;
            } else if (s5 == "src..") {
                source = true;
                pos1 += 5;
            } else {
                break;
            }
            foundUrlCmd = true;
        }
        //if (!foundUrlCmd)
        //    pos1 -= CSettings::ref().urlCmdPrefix.length();
    //}
#endif
		debug	= ::GetAsyncKeyState(VK_PAUSE) < 0;

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
