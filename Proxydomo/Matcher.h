/**
*	@file	Matcher.h
*	@brief	
*/
/**
	this file is part of Proxydomo
	Copyright (C) amate 2013-

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#include <string>
#include <memory>
#include "Node.h"

class CFilter;
class CSettings;

namespace Proxydomo {

/* Class parsing_exception
 * This exception is invoked when CMatcher cannot parse an invalid
 * pattern (e.g malformed +{n,m} )
 */
class parsing_exception {
    public:
        std::string message;
        int position;
        parsing_exception(const std::string& msg, int pos) : message(msg), position(pos) { }
};


/* class CMatcher
 * Builds a search tree from a pattern,
 * Matches the tree with the _beginning_ of a text.
 */
class CMatcher
{
public:
	CMatcher(const std::string& pattern);
	~CMatcher();

	/// pattern Ç©ÇÁ CMatcherÇçÏê¨Ç∑ÇÈ
	static std::shared_ptr<CMatcher>	CreateMatcher(const std::string& pattern);
	static std::shared_ptr<CMatcher>	CreateMatcher(const std::string& pattern, std::string& errmsg);

    //static bool testPattern(const std::string& pattern, std::string& errmsg);
    //static bool testPattern(const std::string& pattern);

    void mayMatch(bool* tab);

    bool match(const char* start, const char* stop,
               const char*& end, MatchData* pMatch);

    // Static version, that builds a search tree at each call
    static bool match(const std::string& pattern, CFilter& filter,
                      const char* start, const char* stop,
                      const char*& end, const char*& reached);

    // Filter that contains memory stack and table
    //CFilter& filter;
    
    // Tells if pattern is * or \0-9
    bool isStar();

private:
    // Root of the Search Tree
    std::shared_ptr<CNode>	m_root;
    
    // Farthest position reached during matching
    //const char* reached;

    // Search Tree building functions
    static CNode* expr  (const std::string& pattern, int& pos, int stop, int level = 0);
    static CNode* run   (const std::string& pattern, int& pos, int stop);
    static CNode* code  (const std::string& pattern, int& pos, int stop);
    static CNode* single(const std::string& pattern, int& pos, int stop);

    // The following classes need access to tree building functions
    friend class CNode_List;
    friend class CNode_Command;
	friend class CSettings;
};


}	// namespace Proxydomo



