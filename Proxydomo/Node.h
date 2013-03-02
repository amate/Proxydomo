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

#pragma once

#include "proximodo\url.h"
#include <unicode\schriter.h>

class CFilter;

namespace Proxydomo {

struct MatchData
{
	const UChar* reached;
	CFilter*	pFilter;
	CUrl		url;

	MatchData(CFilter* filter) : reached(nullptr), pFilter(filter) { }
};


/* class CNode
 * Generic node in the search tree
 *
 * The tree is composed of leaf nodes, that actually read and test the data,
 * and branch nodes that organize them. Examples of leaf nodes: test a
 * character, test a quote, test an arbitrary number of characters. Examples of
 * branch nodes: 'or', 'and', 'sequentially', 'a number of times'.
 *
 * The tree is generated at the construction of a CMatcher, with respect to a
 * given pattern. When CMatcher is used to test data, it asks the root node
 * for the end position of the whole matched text.
 *
 * Each leaf node is linked to the node that is supposed to match the text on its
 * right. When a leaf node matches, it asks the next node for a match, and
 * returns the end position provided by the next node.
 *
 * Branch node may pass match request to their children, which may be linked
 * to the branch's next node or not. If they are not, the branch node must
 * ask its next node after its children matched.
 */
class CNode
{
public:
    // Virtual destructor.
    // The constructor is protected, as only subclasses must be instanciated.
    virtual ~CNode() { }

    // Immediately after constructing the whole node tree, one must call
    // this function (with default parameters) on the root node. The nodes
    // then link together horizontally, so that each node knows which node
    // will scan the text after itself. This is needed for correct processing
    // of "&" and "&&", which try right pattern on the first left match that
    // _do_ let remaining pattern match.
	virtual void setNextNode(CNode* node = nullptr) { m_nextNode = node; }

    // The mayMatch() function updates a table of expected characters
    // at the beginning of the text to match. It is called just after
    // the construction of the whole tree, to provide the search engine
    // with a fast way to know if a call to CMatcher::match() is of any
    // use. If !tab[text[start]] the engine knows there cannot be any
    // match at start, _even_ if the buffer is smaller than the window
    // size.
    // The function returns true if the node may consume nothing yet match.
    virtual bool mayMatch(bool* tab) =0;

    // Match function.
    // Returns the end position of text matched by the whole chain of nodes
    // (wrt. nextNode) up to the last one (the one having nextNode==NULL).
    // The implementation must set 'consumed' to the end position match by
    // the pattern represented by the node itself.
    virtual const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) =0;

    // If match() returned a value != NULL, this variable contains the
    // position corresponding to the end of what just the node itself matched.
    const UChar* m_consumed;

    // Enumeration for id()
    enum type { 
		AND,      ANY,      ASK,      AV,       CHAR,     CHARS,
		CNX,      COMMAND,  EMPTY,    EQUAL,    LIST,     MEMORY,
		MEMSTAR,  NEGATE,   NEST,     OR,       QUOTE,    RANGE,
		REPEAT,   RUN,      SPACE,    STAR,     STRING,   TEST,
		URL 
	};

    // Node type (for upcasting)
    type m_id;

protected:
    //const char*& reached;
    CNode*	m_nextNode;

    // Protected construtor
    CNode(type id) : m_id(id), m_consumed(nullptr), m_nextNode(nullptr) { }
};

}	// namespace Proxydomo
