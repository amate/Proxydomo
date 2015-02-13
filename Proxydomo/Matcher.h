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
#include <unicode\schriter.h>
#include "Node.h"

class CFilter;
class CSettings;

namespace Proxydomo {

using namespace icu;

/* Class parsing_exception
 * This exception is invoked when CMatcher cannot parse an invalid
 * pattern (e.g malformed +{n,m} )
 */
class parsing_exception {
    public:
		int parsingErrorID;
        int position;

		parsing_exception(int parsingErrorID, int pos) : parsingErrorID(parsingErrorID), position(pos) { }
};


/* class CMatcher
 * Builds a search tree from a pattern,
 * Matches the tree with the _beginning_ of a text.
 */
class CMatcher
{
public:
	CMatcher(const std::wstring& pattern);	// for CNode_Command
	~CMatcher();

	/// pattern Ç©ÇÁ CMatcherÇçÏê¨Ç∑ÇÈ
	static std::shared_ptr<CMatcher>	CreateMatcher(const std::wstring& pattern);
	static std::shared_ptr<CMatcher>	CreateMatcher(const std::wstring& pattern, std::wstring& errmsg);

    void mayMatch(bool* tab);

	bool match(const UChar* start, const UChar* stop, const UChar*& end, MatchData* pMatch);
	bool match(const std::string& text, CFilter* filter);

    // Static version, that builds a search tree at each call
    static bool match(const std::wstring& pattern, CFilter& filter,
					  const UChar* start, const UChar* stop, const UChar*& end, const UChar*& reached);

    // Tells if pattern is * or \0-9
    bool isStar();

private:
	void	_CreatePattern(const UnicodeString& pattern);

    // Root of the Search Tree
    std::shared_ptr<CNode>	m_root;

    // Search Tree building functions
    static CNode* expr  (StringCharacterIterator& patternIt, int level = 0);
    static CNode* run   (StringCharacterIterator& patternIt);
    static CNode* code  (StringCharacterIterator& patternIt);
    static CNode* single(StringCharacterIterator& patternIt);

    // The following classes need access to tree building functions
    friend class CNode_List;
    friend class CNode_Command;
	friend class CSettings;
};


}	// namespace Proxydomo



