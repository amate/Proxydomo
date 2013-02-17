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


#include "filterowner.h"
#include "util.h"


/* Remove headers with empty value from a vector of headers.
 */
void CFilterOwner::cleanHeaders(vector<SHeader>& headers) {

    vector<SHeader>::iterator it = headers.begin();
    while (it != headers.end()) {
        if (it->value.empty()) {
            headers.erase(it);
            it = headers.begin();
        } else {
            it++;
        }
    }    
}


/* Find the first header with the right name in a vector of headers.
 */
string& CFilterOwner::getHeader(vector<SHeader>& headers, const string& name) {

    static string emptyString;
    for (vector<SHeader>::iterator it = headers.begin(); it != headers.end(); it++) {
        if (CUtil::noCaseEqual(name, it->name)) return it->value;
    }
    return emptyString = "";
}


/* Set the value of header in a vector of headers.
 * If the header does not exist, it is inserted at the back.
 * Multiple headers are removed.
 */
void CFilterOwner::setHeader(vector<SHeader>& headers, const string& name,
                                                       const string& value) {

    vector<SHeader>::iterator it;
    for (it = headers.begin(); it != headers.end(); it++) {
        if (CUtil::noCaseEqual(name, it->name)) {
            it->value = value;
            break;
        }
    }
    if (it == headers.end()) {
        SHeader header = {name, value};
        headers.push_back(header);
    } else {
        ++it;
        vector<SHeader>::iterator it2 = it;
        while (it2 != headers.end()) {
            if (CUtil::noCaseEqual(name, it2->name)) {
                headers.erase(it2);
                it2 = it;
            } else {
                it2++;
            }
        }
    }
}


/* Remove certain headers from a vector of headers.
 */
void CFilterOwner::removeHeader(vector<SHeader>& headers, const string& name) {

    vector<SHeader>::iterator it = headers.begin();
    while (it != headers.end()) {
        if (CUtil::noCaseEqual(name, it->name)) {
            headers.erase(it);
            it = headers.begin();
        } else {
            it++;
        }
    }
}
