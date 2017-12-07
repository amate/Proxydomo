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

#pragma once

#include <vector>
#include <deque>
#include <mutex>
#include <memory>
#include <unordered_set>
#include "Node.h"
#include "proximodo\memory.h"
#include "proximodo\url.h"
#include "proximodo\filter.h"

struct HashedListCollection;	// for CNode_List

namespace Proxydomo {

class CMatcher;

/* class CNode_Star
 * Try and match any string
 */
class CNode_Star : public CNode
{
public:
	CNode_Star() : CNode(STAR) { }
    ~CNode_Star() { }

	// CNode
    bool mayMatch(bool* tab) override;
    void setNextNode(CNode* node) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;
	
private:
    bool m_maxFirst;
    bool m_tab[256];
    bool m_useTab;
};


/* class CNode_MemStar
 * Try and match any string and store the match
 */
class CNode_MemStar : public CNode
{
public:
	CNode_MemStar(int memoryPos = -1) : CNode(MEMSTAR), m_memoryPos(memoryPos) { }
    ~CNode_MemStar() { }

	// CNode
    bool mayMatch(bool* tab) override;
    void setNextNode(CNode* node) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;

private:
    bool m_maxFirst;
    bool m_tab[256];
    bool m_useTab;

	int	m_memoryPos;	// '0'-'9' or '-1'(stack)
};



/* class CNode_Space
 * Matches any spaces
 */
class CNode_Space : public CNode
{
public:
	CNode_Space() : CNode(SPACE) { }
    ~CNode_Space() { }

	// CNode
    bool mayMatch(bool* tab) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;
};


/* class CNode_Equal
 * Matches an Equal sign surrounded by spaces
 */
class CNode_Equal : public CNode
{
public:
	CNode_Equal() : CNode(EQUAL) { }
    ~CNode_Equal() { }

	// CNode
    bool mayMatch(bool* tab) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;
};


/* class CNode_Quote
 * Try and match a " or a '
 */
class CNode_Quote : public CNode
{
public:
	CNode_Quote(UChar q) : CNode(QUOTE), m_quote(q), m_openingQuote(nullptr) { }
    ~CNode_Quote() { }

	void setOpeningQuote(CNode_Quote* node) { m_openingQuote = node; }

	// CNode
    bool mayMatch(bool* tab) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;

private:
    UChar m_quote;                 // can be ' or "
    CNode_Quote* m_openingQuote;  // points to next ' in run, if any
};


/* class CNode_Char
 * Try and match a single character
 */
class CNode_Char : public CNode
{
public:
	CNode_Char(UChar c) : CNode(CHAR), m_char(c) { }
    ~CNode_Char() { }

	UChar getChar() { return m_char; }

	// CNode
    bool mayMatch(bool* tab) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;

private:
    UChar m_char;
};



/* class CNode_Range
 * Try and match a single character
 */
class CNode_Range : public CNode
{
public:
	CNode_Range(int min, int max, bool allow = true) : 
		CNode(RANGE), m_min(min), m_max(max), m_allow(allow) { }
    ~CNode_Range() { }

	// CNode
    bool mayMatch(bool* tab) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;

private:
    int m_min;
	int m_max;
    bool m_allow;
};


/* class CNode_String
 * Try and match a string of characters
 */
class CNode_String : public CNode
{
public:
	CNode_String(const std::wstring& s) : CNode(STRING), m_str(s) { }
    ~CNode_String() { }

	void append(UChar c) { m_str += c; }

	// CNode
    bool mayMatch(bool* tab) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;

private:
    std::wstring m_str;
};


/* class CNode_Chars
 * Try and match a single character
 */
class CNode_Chars : public CNode
{
public:
    CNode_Chars(const std::wstring& c, bool allow = true);
    ~CNode_Chars() { }

	void add(UChar c);

    bool mayMatch(bool* tab) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;

private:
	std::unordered_set<UChar>	m_setChars;
	bool m_byte[256];
    bool m_allow;
};


/* class CNode_Empty
 * Empty pattern
 */
class CNode_Empty : public CNode
{
public:
	CNode_Empty() : CNode(EMPTY) { }
    ~CNode_Empty() { }

	// CNode
    bool mayMatch(bool* tab) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;
};


/* class CNode_Any
 * Matches any character
 */
class CNode_Any : public CNode
{
public:
	CNode_Any() : CNode(ANY) { }
    ~CNode_Any() { }

	// CNode
    bool mayMatch(bool* tab) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;
};


/* class CNode_Run
 * Try and match a concatenation
 */
class CNode_Run : public CNode
{
public:
	CNode_Run(std::vector<CNode*>* vect) : CNode(RUN), m_nodes(vect), m_firstNode(vect->front()) { }
    ~CNode_Run();

	// CNode
    bool mayMatch(bool* tab) override;
    void setNextNode(CNode* node) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;

private:
    std::vector<CNode*>* m_nodes;
    CNode* m_firstNode;
};


/* class CNode_Or
 * Try and match nodes one after another
 */
class CNode_Or : public CNode
{
public:
	CNode_Or(std::vector<CNode*>* vect) : CNode(OR), m_nodes(vect) { }
    ~CNode_Or();

	// CNode
    bool mayMatch(bool* tab) override;
    void setNextNode(CNode* node) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;

private:
    std::vector<CNode*>* m_nodes;
};


/* class CNode_And
 * Try and match left node then right node (returns max length of both)
 * Stop is for "&&", limiting right to matching exactly what left matched
 */
class CNode_And : public CNode
{
public:
	CNode_And(CNode* L, CNode* R) : 
		CNode(AND), m_nodeL(L), m_nodeR(R) { }
    ~CNode_And();

	// CNode
    bool mayMatch(bool* tab) override;
    void setNextNode(CNode* node) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;

private:
    CNode* m_nodeL;
	CNode* m_nodeR;
};

/* class CNode_AndAnd
* Try and match left node then right node (returns max length of both)
* Stop is for "&&", limiting right to matching exactly what left matched
*/

class CNode_AndAnd : public CNode
{
public:
	CNode_AndAnd(CNode* L, CNode* R);
	~CNode_AndAnd();

	// CNode
	bool mayMatch(bool* tab) override;
	void setNextNode(CNode* node) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;

private:
	CNode* m_nodeL;
	CNode* m_nodeR;
	class CNode_nodeLReachedRecoder;
	CNode_nodeLReachedRecoder*	m_recorder;
};

/* class CNode_Repeat
 * Try and match a pattern several times
 */
class CNode_Repeat : public CNode
{
public:
	CNode_Repeat(CNode* node, int rmin, int rmax, bool iter = false) : 
		CNode(REPEAT), m_node(node), m_rmin(rmin), m_rmax(rmax), m_iterate(iter) { }
    ~CNode_Repeat();

	// CNode
    bool mayMatch(bool* tab) override;
    void setNextNode(CNode* node) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;

private:
    CNode* m_node;
    int m_rmin;
	int m_rmax;
    bool m_iterate;

};


/* class CNode_Memory
 * Try and match something, store the match if success (in table or on stack)
 */
class CNode_Memory : public CNode
{
public:
    CNode_Memory(CNode* node, int memoryPos);
    ~CNode_Memory();

	// CNode
    bool mayMatch(bool* tab) override;
    void setNextNode(CNode* node) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;

private:
    CNode* m_node;
	int	m_memoryPos;	// 	'0'-'9' or '-1'(stack)

    CNode_Memory* m_memorizer;
	bool	m_contentNodeIsAnd;
    //const UChar* m_recordPos;
};


/* class CNode_Negate
 * Try and match something, if it does it returns -1, else start
 */
class CNode_Negate : public CNode
{
public:
	CNode_Negate(CNode* node) : CNode(NEGATE), m_node(node) { }
	~CNode_Negate() { delete m_node; }

	// CNode
    bool mayMatch(bool* tab) override;
    void setNextNode(CNode* node) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;

private:
    CNode* m_node;
};


/* class CNode_AV
 * Try and match an html parameter
 */
class CNode_AV : public CNode
{
public:
	CNode_AV(CNode* node, bool isAVQ) : CNode(AV), m_node(node), m_isAVQ(isAVQ) { }
	~CNode_AV() { delete m_node; }

	// CNode
    bool mayMatch(bool* tab) override;
    void setNextNode(CNode* node) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;

private:
    CNode*	m_node;
    bool	m_isAVQ;
};


/* class CNode_Url
 * Try and match a part of URL
 */
class CNode_Url : public CNode
{
public:
	CNode_Url(const UChar token) : CNode(URL), m_token(token) { }
    ~CNode_Url() { }

	// CNode
    bool mayMatch(bool* tab) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;

private:
    const UChar	m_token;
};



/* class CNode_List
 * Try and match nodes one after another. Nodes are looked for in the
 * order they appear in CSettings::lists . They are built in the
 * constructor, to reflect the list of patterns. New nodes can be built
 * and added to the object by other threads, running the $ADDLST command.
 */
class CNode_List : public CNode
{
public:
	CNode_List(const std::string& name, bool top = true);
    ~CNode_List();

	// CNode
    bool mayMatch(bool* tab) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;

private:
	// Data members
    const std::string		m_name;            // name of the list
	HashedListCollection*	m_phashedCollection;
	std::unique_ptr<CNode_List>	m_whiteList;
};



/* class CNode_Command
 * Execute a command. Commands that do not consume anything (whether they
 * always match or not) don't have a dedicated CNode class. They are
 * gathered here.
 */
enum CMD_ID {
    CMD_SETSHARP = 1,
    CMD_SETDIGIT,  CMD_SETVAR,   CMD_ALERT,  CMD_CONFIRM, CMD_STOP,
    CMD_USEPROXY,  CMD_SETPROXY, CMD_LOG,    CMD_JUMP,    CMD_RDIR,
    CMD_FILTER,    CMD_LOCK,     CMD_UNLOCK, CMD_KEYCHK,  CMD_ADDLST,
    CMD_ADDLSTBOX, CMD_TYPE,     CMD_KILL,
    
    CMD_TSTSHARP = 100, // From this point, commands need a CMatcher for content
    CMD_TSTDIGIT, CMD_TSTVAR, CMD_URL,
    CMD_IHDR,     CMD_OHDR,   CMD_RESP,
    CMD_TSTEXPAND
};

class CNode_Command : public CNode
{
public:
    CNode_Command(CMD_ID num, const std::wstring& name, const std::wstring& content);
    ~CNode_Command();

	// CNode
    bool mayMatch(bool* tab) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;

private:
    CMD_ID	m_num;
    std::wstring m_name;
    std::wstring m_content;

    CMatcher* m_matcher;
};

/* class CNode_Command_Type
 * $TYPE()
 */
class CNode_Command_Type : public CNode
{
public:
	CNode_Command_Type(const std::wstring& type);

	// CNode
	bool mayMatch(bool* tab) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;

private:
	std::string m_type;
};


/* class CNode_Cnx
 * Matches depending on connection number.
 */
class CNode_Cnx : public CNode
{
public:
	CNode_Cnx(int x, int y, int z, int& num) : CNode(CNX), m_x(x), m_y(y), m_z(z), m_num(num) { }
    ~CNode_Cnx() { }

	// CNode
    bool mayMatch(bool* tab) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;

private:
    int	m_x;
	int m_y;
	int m_z;
    int& m_num;
};


/* class CNode_Nest
 * Matches nested tags, with optional content.
 */
class CNode_Nest : public CNode
{
public:
    CNode_Nest(CNode* left, CNode* middle, CNode* right, bool inest);
    ~CNode_Nest();

	// CNode
    bool mayMatch(bool* tab) override;
    void setNextNode(CNode* node) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;

private:
    CNode*	m_left;
	CNode*	m_middle;
	CNode*	m_right;
    bool m_tabL[256];
	bool m_tabR[256];
    bool m_inest;
};


/* class CNode_Test
 * Try and match a variable content. Commands $TST(var) and $TST(\?)
 */
class CNode_Test : public CNode
{
public:
	CNode_Test(const std::wstring& name/*, CFilter& filter*/) : CNode(TEST), m_name(name) { }
    ~CNode_Test() { }

	// CNode
    bool mayMatch(bool* tab) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;

private:
    std::wstring m_name;
    //CFilter& filter;
};


/* class CNode_Ask
 * Automates the insertion of an item in a list based on user choice.
 */
class CNode_Ask : public CNode
{
public:
    CNode_Ask(/*CFilter& filter,*/
                std::wstring allowName, std::wstring denyName, std::wstring question,
                std::wstring item, std::wstring pattern);
    ~CNode_Ask();

	// CNode
    bool mayMatch(bool* tab) override;
	const UChar* match(const UChar* start, const UChar* stop, MatchData* pMatch) override;

private:
    //CFilter& filter;
    const std::wstring allowName, denyName, question, item, pattern;
    std::unique_ptr<CMatcher>	m_allowMatcher;
	std::unique_ptr<CMatcher>	m_denyMatcher;
};

}	// namespace Proxydomo

