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


#ifndef __descriptor__
#define __descriptor__

#include <string>
#include <map>
#include <set>

using namespace std;


/* This class is used to represent a folder
 */
class CFolder {

public:
    int id;
    string name;
    int parent;
    int defaultFolder;
    set<int> children;
    bool operator==(const CFolder& d) const {
        return id == d.id &&
               name == d.name &&
               parent == d.parent &&
               defaultFolder == d.defaultFolder &&
               children == d.children;
    }
    CFolder() { }
    CFolder(int id, string name, int parent, int defaultFolder = 0) :
            id(id), name(name), parent(parent), defaultFolder(defaultFolder) { }
};


/* class CFilterDescriptor
 * Describes a filter.
 * All filters will be stored in a repository (CSettings), managed 
 * via the GUI. No two filters can have the same title, as the _title_
 * is the key in the repository.
 * When the proxy needs to process an HTTP request,
 * the descriptors which titles are in the current config's list
 * are used to build a chain of filters.
 */
class CFilterDescriptor {

public:
    CFilterDescriptor();

    // The following data is used for organizing/editing filters
    int id;             // Unique ID of the filter
    string title;       // Title of the filter
    string version;     // Version number
    string author;      // Name of author
    string comment;     // Comment (such as description of what the filter does)
    int folder;         // parent folder's id

    // Type of filter
    enum TYPE { HEADOUT, HEADIN, TEXT };
    TYPE filterType;

    // Data specific to header filters
    string headerName;

    // Data specific to text filters
    bool   multipleMatches;
    int    windowWidth;
    string boundsPattern;

    // Data commom to both
    string urlPattern;
    string matchPattern;
    string replacePattern;
    int    priority;
    
    // Default filter number (set to 0 for new/modified filters)
    int defaultFilter;
    
    // Check if all data is valid
    void testValidity();
    string errorMsg;

    // Compare filters
    bool operator<(const CFilterDescriptor& d) const;
    bool operator==(const CFilterDescriptor& d) const;

    // Clear all content
    void clear();
    
    // Convenient function for getting the type name
    string typeName() const;

    // Export the filter to a string
    string exportFilter(const map<int,CFolder>& folders, int root = 0) const;
    
    // Export an entire map of filters to a string
    static string exportFilters(const map<int,CFolder>& folders,
                                const map<int,CFilterDescriptor>& filters);
    
    // Import filters from a string to a map
    static int importFilters(const string& text,
                    map<int,CFolder>& folders,
                    map<int,CFilterDescriptor>& filters,
                    int root = 0);

    // Import Proxomitron filters from a string to a map
    static int importProxomitron(const string& text,
                    map<int,CFolder>& folders,
                    map<int,CFilterDescriptor>& filters,
                    int root = 0,
                    set<int>* config = NULL);
};

#endif
