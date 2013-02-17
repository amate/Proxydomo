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


#ifndef __parser__
#define __parser__

#include <string>
class CNode;
class CFilter;

using namespace std;

/* Class parsing_exception
 * This exception is invoked when CMatcher cannot parse an invalid
 * pattern (e.g malformed +{n,m} )
 */
class parsing_exception {
    public:
        string message;
        int position;
        parsing_exception(string msg, int pos) : message(msg), position(pos) { }
};


/* class CMatcher
 * Builds a search tree from a pattern,
 * Matches the tree with the _beginning_ of a text.
 */
class CMatcher {

public:
    CMatcher(const string& pattern, CFilter& filter);
    ~CMatcher();

    static bool testPattern(const string& pattern, string& errmsg);
    static bool testPattern(const string& pattern);

    void mayMatch(bool* tab);

    bool match(const char* start, const char* stop,
               const char*& end, const char*& reached);

    // Static version, that builds a search tree at each call
    static bool match(const string& pattern, CFilter& filter,
                      const char* start, const char* stop,
                      const char*& end, const char*& reached);

    // Filter that contains memory stack and table
    CFilter& filter;
    
    // Tells if pattern is * or \0-9
    bool isStar();

private:
    // Root of the Search Tree
    CNode* root;
    
    // Farthest position reached during matching
    const char* reached;
    
    // Decodes n,m in a pattern
    void readMinMax(const string& pattern, int& pos, int stop,
                    char sep, int& min, int& max);
    int  readNumber(const string& pattern, int& pos, int stop, int &num);
    
    // Decodes a command
    int decodeCommand(const string& pattern, int pos, int stop,
                        string& command, string& content);

    // Search Tree building functions
    CNode* expr  (const string& pattern, int& pos, int stop, int level = 0);
    CNode* run   (const string& pattern, int& pos, int stop);
    CNode* code  (const string& pattern, int& pos, int stop);
    CNode* single(const string& pattern, int& pos, int stop);

    // The following classes need access to tree building functions
    friend class CNode_List;
    friend class CNode_Command;
};

#endif
