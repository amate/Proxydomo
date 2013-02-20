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


#ifndef __nodes__
#define __nodes__

#include "node.h"
#include "memory.h"
//#include <wx/thread.h>
#include <string>
#include <vector>
#include <set>
#include <deque>
#include <mutex>
class CUrl;
class CMatcher;
class CFilter;
class CFilterOwner;

using namespace std;

/* class CNode_Star
 * Try and match any string
 */
class CNode_Star : public CNode {

private:
    int pos;
    bool maxFirst;
    bool tab[256];
    bool useTab;

public:
    CNode_Star(const char*& reached);
    ~CNode_Star() { }
    bool mayMatch(bool* tab);
    void setNextNode(CNode* node);
    NODE_MATCH()
};


/* class CNode_MemStar
 * Try and match any string and store the match
 */
class CNode_MemStar : public CNode {

private:
    int pos;
    bool maxFirst;
    bool tab[256];
    bool useTab;
    CMemory* memory;
    vector<CMemory>* stack;
    CMemory backup;

public:
    CNode_MemStar(const char*& reached, CMemory& mem);
    CNode_MemStar(const char*& reached, vector<CMemory>& st);
    ~CNode_MemStar() { }
    bool mayMatch(bool* tab);
    void setNextNode(CNode* node);
    NODE_MATCH()
};


/* class CNode_Space
 * Matches any spaces
 */
class CNode_Space : public CNode {

public:
    CNode_Space(const char*& reached);
    ~CNode_Space() { }
    bool mayMatch(bool* tab);
    NODE_SETNEXTNODE()
    NODE_MATCH()
};


/* class CNode_Equal
 * Matches an Equal sign surrounded by spaces
 */
class CNode_Equal : public CNode {

public:
    CNode_Equal(const char*& reached);
    ~CNode_Equal() { }
    bool mayMatch(bool* tab);
    NODE_SETNEXTNODE()
    NODE_MATCH()
};


/* class CNode_Quote
 * Try and match a " or a '
 */
class CNode_Quote : public CNode {

private:
    char quote;                 // can be ' or "
    char matched;
    CNode_Quote *openingQuote;  // points to next ' in run, if any

public:
    CNode_Quote(const char*& reached, char q);
    ~CNode_Quote() { }
    void setOpeningQuote(CNode_Quote *node);
    bool mayMatch(bool* tab);
    NODE_SETNEXTNODE()
    NODE_MATCH()
};


/* class CNode_Char
 * Try and match a single character
 */
class CNode_Char : public CNode {

private:
    char byte;

public:
    CNode_Char(const char*& reached, char c);
    ~CNode_Char() { }
    char getChar();
    bool mayMatch(bool* tab);
    NODE_SETNEXTNODE()
    NODE_MATCH()
};


/* class CNode_Range
 * Try and match a single character
 */
class CNode_Range : public CNode {

private:
    int min, max;
    bool allow;

public:
    CNode_Range(const char*& reached,
                int min, int max, bool allow=true);
    ~CNode_Range() { }
    bool mayMatch(bool* tab);
    NODE_SETNEXTNODE()
    NODE_MATCH()
};


/* class CNode_String
 * Try and match a string of characters
 */
class CNode_String : public CNode {

private:
    string str;
    int size;

public:
    CNode_String(const char*& reached, const string& s);
    ~CNode_String() { }
    void append(char c);
    bool mayMatch(bool* tab);
    NODE_SETNEXTNODE()
    NODE_MATCH()
};


/* class CNode_Chars
 * Try and match a single character
 */
class CNode_Chars : public CNode {

private:
    bool byte[256];
    bool allow;

public:
    CNode_Chars(const char*& reached, string c, bool allow=true);
    ~CNode_Chars() { }
    void add(unsigned char c);
    bool mayMatch(bool* tab);
    NODE_SETNEXTNODE()
    NODE_MATCH()
};


/* class CNode_Empty
 * Empty pattern
 */
class CNode_Empty : public CNode {

private:
    bool accept;

public:
    CNode_Empty(const char*& reached, bool ret=true);
    ~CNode_Empty() { }
    bool mayMatch(bool* tab);
    NODE_SETNEXTNODE()
    NODE_MATCH()
};


/* class CNode_Any
 * Matches any character
 */
class CNode_Any : public CNode {

public:
    CNode_Any(const char*& reached);
    ~CNode_Any() { }
    bool mayMatch(bool* tab);
    NODE_SETNEXTNODE()
    NODE_MATCH()
};


/* class CNode_Run
 * Try and match a concatenation
 */
class CNode_Run : public CNode {

private:
    vector<CNode*> *nodes;
    CNode* firstNode;

public:
    CNode_Run(const char*& reached, vector<CNode*> *vect);
    ~CNode_Run();
    bool mayMatch(bool* tab);
    void setNextNode(CNode* node);
    NODE_MATCH()
};


/* class CNode_Or
 * Try and match nodes one after another
 */
class CNode_Or : public CNode {

private:
    vector<CNode*> *nodes;

public:
    CNode_Or(const char*& reached, vector<CNode*> *vect);
    ~CNode_Or();
    bool mayMatch(bool* tab);
    void setNextNode(CNode* node);
    NODE_MATCH()
};


/* class CNode_And
 * Try and match left node then right node (returns max length of both)
 * Stop is for "&&", limiting right to matching exactly what left matched
 */
class CNode_And : public CNode {

private:
    CNode *nodeL, *nodeR;
    bool force;

public:
    CNode_And(const char*& reached,
                CNode *L, CNode *R, bool force=false);
    ~CNode_And();
    bool mayMatch(bool* tab);
    void setNextNode(CNode* node);
    NODE_MATCH()
};


/* class CNode_Repeat
 * Try and match a pattern several times
 */
class CNode_Repeat : public CNode {

private:
    CNode *node;
    int rmin, rmax;
    bool iterate;

public:
    CNode_Repeat(const char*& reached, CNode *node,
                int rmin, int rmax, bool iter=false);
    ~CNode_Repeat();
    bool mayMatch(bool* tab);
    void setNextNode(CNode* node);
    NODE_MATCH()
};


/* class CNode_Memory
 * Try and match something, store the match if success (in table or on stack)
 */
class CNode_Memory : public CNode {

private:
    CNode* node;
    CMemory* memory;
    vector<CMemory>* stack;
    CMemory backup;
    CNode_Memory* memorizer;
    const char* recordPos;

public:
    CNode_Memory(const char*& reached, CNode *node, CMemory& mem);
    CNode_Memory(const char*& reached, CNode *node, vector<CMemory>& st);
    ~CNode_Memory();
    bool mayMatch(bool* tab);
    void setNextNode(CNode* node);
    NODE_MATCH()
};


/* class CNode_Negate
 * Try and match something, if it does it returns -1, else start
 */
class CNode_Negate : public CNode {

private:
    CNode* node;

public:
    CNode_Negate(const char*& reached, CNode *node);
    ~CNode_Negate();
    bool mayMatch(bool* tab);
    void setNextNode(CNode* node);
    NODE_MATCH()
};


/* class CNode_AV
 * Try and match an html parameter
 */
class CNode_AV : public CNode {

private:
    CNode* node;
    bool isAVQ;

public:
    CNode_AV(const char*& reached, CNode *node, bool isAVQ);
    ~CNode_AV();
    bool mayMatch(bool* tab);
    void setNextNode(CNode* node);
    NODE_MATCH()
};


/* class CNode_Url
 * Try and match a part of URL
 */
class CNode_Url : public CNode {

private:
    const CUrl& url;
    char token;

public:
    CNode_Url(const char*& reached, const CUrl& url, char token);
    ~CNode_Url() { }
    bool mayMatch(bool* tab);
    NODE_SETNEXTNODE()
    NODE_MATCH()
};


/* class CNode_List
 * Try and match nodes one after another. Nodes are looked for in the
 * order they appear in CSettings::lists . They are built in the
 * constructor, to reflect the list of patterns. New nodes can be built
 * and added to the object by other threads, running the $ADDLST command.
 */
class CNode_List : public CNode {

private:
    string name;            // name of the list
    CMatcher& matcher;      // matcher that will build nodes

    typedef struct {
        CNode* node;        // points to the built node tree
        int    flags;       // 0x1: is a tilde pattern,  0x2: is hashable
    } SListItem;
    
	std::recursive_mutex hashedMutex;            // to secure access to 'hashed' and 'lastTab'
    deque<SListItem> hashed[256];   // sublist of nodes that start with a
                                    // constant char; indexed by first char
                                    // unhashable entries go on every list

    static std::recursive_mutex objectsMutex;            // to secure access to 'objects'
    static set<CNode_List*> objects;        // all CNode_List instances.

    void pushPattern(const string& pattern);// add a pattern to 'hashed'
    void popPattern(const string& pattern); // removes a pattern from 'hashed'

    static bool isHashable(char c);

    static inline int hashBucket(char c)
        { return tolower(c) & 0xff; }

public:
    CNode_List(const char*& reached, const string& name, CMatcher& matcher);
    ~CNode_List();
    bool mayMatch(bool* tab);
    void setNextNode(CNode* node);
    NODE_MATCH()
    static void notifyPatternPushBack(const string& listname, const string& pattern);
    static void notifyPatternPopFront(const string& listname, const string& pattern);
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

class CNode_Command : public CNode {

private:
    CMD_ID num;
    string name;
    string content;
    CFilter& filter;
    CFilterOwner& owner;
    string toMatch;
    CMatcher* matcher;

public:
    CNode_Command(const char*& reached,
                  CMD_ID num, string name, string content, CFilter& filter);
    ~CNode_Command();
    bool mayMatch(bool* tab);
    NODE_SETNEXTNODE()
    NODE_MATCH()
};


/* class CNode_Cnx
 * Matches depending on connection number.
 */
class CNode_Cnx : public CNode {

private:
    int x, y, z;
    int& num;

public:
    CNode_Cnx(const char*& reached, int x, int y, int z, int& num);
    ~CNode_Cnx() { }
    bool mayMatch(bool* tab);
    NODE_SETNEXTNODE()
    NODE_MATCH()
};


/* class CNode_Nest
 * Matches nested tags, with optional content.
 */
class CNode_Nest : public CNode {

private:
    CNode *left, *middle, *right;
    bool tabL[256], tabR[256];
    bool inest;

public:
    CNode_Nest(const char*& reached,
                CNode* left, CNode* middle, CNode* right, bool inest);
    ~CNode_Nest();
    bool mayMatch(bool* tab);
    void setNextNode(CNode* node);
    NODE_MATCH()
};


/* class CNode_Test
 * Try and match a variable content. Commands $TST(var) and $TST(\?)
 */
class CNode_Test : public CNode {

private:
    string name;
    CFilter& filter;

public:
    CNode_Test(const char*& reached, string name, CFilter& filter);
    ~CNode_Test() { }
    bool mayMatch(bool* tab);
    NODE_SETNEXTNODE()
    NODE_MATCH()
};


/* class CNode_Ask
 * Automates the insertion of an item in a list based on user choice.
 */
class CNode_Ask : public CNode {

private:
    CFilter& filter;
    const string allowName, denyName, question, item, pattern;
    CMatcher *allowMatcher, *denyMatcher;
    string toMatch; // for matchers

public:
    CNode_Ask(const char*& reached, CFilter& filter,
                string allowName, string denyName, string question,
                string item, string pattern);
    ~CNode_Ask();
    bool mayMatch(bool* tab);
    NODE_SETNEXTNODE()
    NODE_MATCH()
};

#endif
// vi:ts=4:sw=4:et
