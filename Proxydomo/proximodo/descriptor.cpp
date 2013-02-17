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


#include "descriptor.h"
#include "matcher.h"
#include "util.h"
#include "settings.h"
#include <sstream>

using namespace std;

/* Constructor
 */
CFilterDescriptor::CFilterDescriptor() {
    clear();
}


/* Check if all data is valid
 */
void CFilterDescriptor::testValidity() {

    errorMsg.clear();
    CSettings& settings = CSettings::ref();
    if (title.empty()) {
        errorMsg = settings.getMessage("INVALID_FILTER_TITLE");
    } else if (filterType == TEXT) {
        if (matchPattern.empty()) {
            errorMsg = settings.getMessage("INVALID_FILTER_MATCHEMPTY");
        } else if (windowWidth <= 0) {
            errorMsg = settings.getMessage("INVALID_FILTER_WIDTH");
        } else {
            CMatcher::testPattern(boundsPattern, errorMsg) &&
            CMatcher::testPattern(urlPattern,    errorMsg) &&
            CMatcher::testPattern(matchPattern,  errorMsg);
        }
    } else {
        if (headerName.empty()) {
            errorMsg = settings.getMessage("INVALID_FILTER_HEADEREMPTY");
        } else {
            CMatcher::testPattern(urlPattern,   errorMsg) &&
            CMatcher::testPattern(matchPattern, errorMsg);
        }
    }
}

/* Clear all content
 */
void CFilterDescriptor::clear() {

    title = version = author = comment = headerName = "";
    boundsPattern = urlPattern = matchPattern = replacePattern = "";
    filterType = TEXT;
    multipleMatches = false;
    windowWidth = 256;
    defaultFilter = 0;
    priority = 999;
    id = -1;
    folder = 0;
}


/* Export the filter to a string.
 * folders is needed to write the folder path as a string.
 * root can be used to write a relative folder path.
 */
string CFilterDescriptor::exportFilter(const map<int,CFolder>& folders,
                int root) const {

    stringstream out;
    
    out << "[" << title << "]" << endl;
    out << "ID=" << id << endl;
    out << "Priority=" << priority << endl;

    if (filterType == TEXT) {
        out << "Type=text" << endl;
        out << "Multiple=" << (multipleMatches?"yes":"no") << endl;
        out << "Width=" << windowWidth << endl;
    } else if (filterType == HEADOUT) {
        out << "Type=out" << endl;
    } else {
        out << "Type=in" << endl;
    }
    
    if (!version.empty())    out << "Version="       << version       << endl;
    if (!author.empty())     out << "Author="        << author        << endl;
    if (!headerName.empty()) out << "Header="        << headerName    << endl;
    if (defaultFilter)       out << "DefaultFilter=" << defaultFilter << endl;

    if (folder != 0 && folder != root) {
        string path;
        for (int i=folder; i != 0 && i != root; i = folders.find(i)->second.parent)
            path = CUtil::quote(folders.find(i)->second.name) + "/" + path;
        out << "Folder=" << path << endl;
    }
        
    if (!comment.empty())
        out << "Comment=" << CUtil::replaceAll(comment,       "\n","\n_") << endl;
    if (!urlPattern.empty())
        out << "URL="     << CUtil::replaceAll(urlPattern,    "\n","\n_") << endl;
    if (!boundsPattern.empty())
        out << "Bounds="  << CUtil::replaceAll(boundsPattern, "\n","\n_") << endl;
    if (!matchPattern.empty())
        out << "Match="   << CUtil::replaceAll(matchPattern,  "\n","\n_") << endl;
    if (!replacePattern.empty())
        out << "Replace=" << CUtil::replaceAll(replacePattern,"\n","\n_") << endl;
    out << endl;
    return out.str();
}


/* Export an entire map of filters to a string
 */
string CFilterDescriptor::exportFilters(const map<int,CFolder>& folders,
            const map<int,CFilterDescriptor>& filters) {

    stringstream out;
    for (map<int,CFilterDescriptor>::const_iterator it = filters.begin();
                it != filters.end(); it++) {
        out << it->second.exportFilter(folders);
    }
    return out.str();
}


/* Import filters from a string to a map.
 * Returns the number of filters imported.
 */
int CFilterDescriptor::importFilters(const string& text,
                map<int,CFolder>& folders,
                map<int,CFilterDescriptor>& filters, int root) {

    // Process text: lines will be terminated by \n (even from Mac text files)
    // and multiline values will have \r for inner newlines. The text will end
    // by a [] line so that we don't have to process anything after the loop.
    string str = text + "\n[]\n";
    str = CUtil::replaceAll(str, "\r\n", "\n");
    str = CUtil::replaceAll(str, "\r",   "\n");
    str = CUtil::replaceAll(str, "\n_",  "\r");
    
    CFilterDescriptor d;
    d.folder = root;
    size_t i = 0, max = str.size(), count = 0;
    while (i < max) {
        size_t j = str.find("\n", i);
        string line = str.substr(i, j - i);
        size_t eq = line.find('=');
        if (!line.empty() && line[0] == '[') {
            d.testValidity();
            if (!d.title.empty()) {
                if (d.id == -1 || filters.find(d.id) != filters.end())
                    d.id = (filters.empty() ? 1 : filters.rbegin()->second.id + 1);
                filters[d.id] = d;
                count++;
            }
            d.clear();
            d.folder = root;
            d.title = line.substr(1, line.size()-2);
        } else if (eq != string::npos) {
            string label = line.substr(0, eq);
            CUtil::trim(label);
            CUtil::upper(label);
            string value = line.substr(eq + 1);
            value = CUtil::replaceAll(value, "\r", "\n");
            if (label == "TYPE") {
                CUtil::trim(value);
                char c = toupper(value[0]);
                d.filterType = (c == 'T' ? TEXT : c == 'I' ? HEADIN : HEADOUT);
            } else if (label == "MULTIPLE") {
                CUtil::trim(value);
                d.multipleMatches = (toupper(value[0]) == 'Y');
            } else if (label == "WIDTH") {
                stringstream ss;
                ss << CUtil::trim(value);
                ss >> d.windowWidth;
            } else if (label == "PRIORITY") {
                stringstream ss;
                ss << CUtil::trim(value);
                ss >> d.priority;
            } else if (label == "ID") {
                stringstream ss;
                ss << CUtil::trim(value);
                ss >> d.id;
            }
            else if (label == "DEFAULTFILTER") {
                stringstream ss;
                ss << CUtil::trim(value);
                ss >> d.defaultFilter;
            }
            else if (label == "FOLDER" || label == "CATEGORY") {
                string name;
                int slash = -1;
                do {
                    slash = CUtil::getQuoted(value, name, slash, '/');
                    if (!name.empty()) {
                        set<int>& children = folders[d.folder].children;
                        set<int>::iterator it;
                        for (it = children.begin(); it != children.end(); it++) {
                            if (folders[*it].name == name) break;
                        }
                        if (it != children.end()) {
                            d.folder = *it;
                        } else {
                            int n = folders.rbegin()->first + 1;
                            folders[n] = CFolder(n, name, d.folder);
                            folders[d.folder].children.insert(n);
                            d.folder = n;
                        }
                    }
                } while (slash >= 0);
            }
            else if (label == "VERSION" ) CUtil::trim(d.version = value);
            else if (label == "AUTHOR"  ) CUtil::trim(d.author = value);
            else if (label == "COMMENT" ) CUtil::trim(d.comment = value);
            else if (label == "HEADER"  ) CUtil::trim(d.headerName = value);
            else if (label == "URL"     ) d.urlPattern = value;
            else if (label == "BOUNDS"  ) d.boundsPattern = value;
            else if (label == "MATCH"   ) d.matchPattern = value;
            else if (label == "REPLACE" ) d.replacePattern = value;
        }
        i = j + 1;
    }
    return count;
}


/* Import Proxomitron filters from a string to a map.
 * Returns the number of filters imported.
 */
int CFilterDescriptor::importProxomitron(const string& text,
                map<int,CFolder>& folders,
                map<int,CFilterDescriptor>& filters, int root, set<int>* config) {

    // Process text: lines will be terminated by \n (even from Mac text files)
    // and multiline values will have \r for inner newlines. The text will end
    // by a [] line so that we don't have to process anything after the loop.
    string str = text + "\n\n";
    str = CUtil::replaceAll(str, "\r\n", "\n");
    str = CUtil::replaceAll(str, "\r",   "\n");
    str = CUtil::replaceAll(str, "\"\n        \"",  "\r");
    str = CUtil::replaceAll(str, "\"\n          \"",  "\r");
    int priority = 999;

    CFilterDescriptor d;
    bool isActive = false;
    d.folder = root;
    size_t i = 0, max = str.size(), count = 0;
    while (i < max) {
        size_t j = str.find("\n", i);
        string line = str.substr(i, j - i);
        size_t eq = line.find('=');
        if (line.empty()) {
            d.testValidity();
            if (!d.title.empty()) {
                d.priority = priority--;
                d.id = (filters.empty() ? 1 : filters.rbegin()->second.id + 1);
                filters[d.id] = d;
                if (isActive && config) config->insert(d.id);
                count++;
            }
            d.clear();
            isActive = false;
            d.folder = root;
        } else if (eq != string::npos) {
            string label = line.substr(0, eq);
            CUtil::trim(label);
            CUtil::upper(label);
            string value = line.substr(eq + 1);
            CUtil::trim(value);
            CUtil::trim(value, "\"");
            value = CUtil::replaceAll(value, "\r", "\n");

            if (label == "IN") {
                if (value == "TRUE") {
                    d.filterType = HEADIN;
                    isActive = true;
                }
            }
            else if (label == "OUT") {
                if (value == "TRUE") {
                    d.filterType = HEADOUT;
                    isActive = true;
                }
            }
            else if (label == "ACTIVE") {
                if (value == "TRUE") {
                    isActive = true;
                }
            }
            else if (label == "KEY") {
                size_t colon = value.find(':');
                if (colon != string::npos) {
                    d.headerName = value.substr(0, colon);
                    d.title = value.substr(colon + 1);
                    CUtil::trim(d.headerName);
                    CUtil::trim(d.title);
                    if (d.filterType == TEXT) {
                        CUtil::lower(value);
                        size_t i = value.find('(');
                        d.filterType = ((i != string::npos &&
                            value.find("out", i) != string::npos)
                            ? HEADOUT : HEADIN);
                    }
                }
            }
            else if (label == "NAME") {
                d.title = CUtil::trim(value);
                d.filterType = TEXT;
            }
            else if (label == "MULTI") {
                d.multipleMatches = (value[0] == 'T');
            }
            else if (label == "LIMIT") {
                stringstream ss;
                ss << value;
                ss >> d.windowWidth;
            }
            else if (label == "URL"     ) d.urlPattern = value;
            else if (label == "BOUNDS"  ) d.boundsPattern = value;
            else if (label == "MATCH"   ) d.matchPattern = value;
            else if (label == "REPLACE" ) d.replacePattern = value;
        }
        i = j + 1;
    }
    return count;
}


/* Compare two filters for sorting by decreasing priority
 */
bool CFilterDescriptor::operator<(const CFilterDescriptor& d) const {
    if (filterType != d.filterType)
        return filterType < d.filterType;
    if (priority != d.priority)
        return priority > d.priority;
    return title < d.title;
}


/* Compare two filter descriptions
 */
bool CFilterDescriptor::operator ==(const CFilterDescriptor& d) const {
    return (
        id              == d.id              &&
        folder          == d.folder          &&
        title           == d.title           &&
        version         == d.version         &&
        author          == d.author          &&
        comment         == d.comment         &&
        filterType      == d.filterType      &&
        headerName      == d.headerName      &&
        multipleMatches == d.multipleMatches &&
        windowWidth     == d.windowWidth     &&
        boundsPattern   == d.boundsPattern   &&
        urlPattern      == d.urlPattern      &&
        priority        == d.priority        &&
        matchPattern    == d.matchPattern    &&
        replacePattern  == d.replacePattern  &&
        defaultFilter   == d.defaultFilter   );
}


/* Convenient function for getting the type name
 */
string CFilterDescriptor::typeName() const {

    if (filterType == HEADOUT)
        return CSettings::ref().getMessage("CONFIG_TYPE_OUT");
    else if (filterType == HEADIN)
        return CSettings::ref().getMessage("CONFIG_TYPE_IN");
    else
        return CSettings::ref().getMessage("CONFIG_TYPE_TEXT");
}
