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


#include "nodes.h"
#include "const.h"
#include "util.h"
//#include "settings.h"
#include "..\Settings.h"
#include "expander.h"
//#include "log.h"
#include "..\Log.h"
//#include "logframe.h"
#include "url.h"
#include "matcher.h"
#include "filter.h"
#include "..\filterowner.h"
//#include <wx/msgdlg.h>
//#include <wx/textdlg.h>
#include <algorithm>
#include <functional>

using namespace std;

/* this macro updates the CNode::reached value
 */
#define UPDATE_REACHED(p) (reached<p?reached=p:p)


/* class CNode_Star
 * Try and match any string. Corresponds to *
 */
CNode_Star::CNode_Star(const char*& reached) : CNode(reached, STAR) {
}

const char* CNode_Star::match(const char* start, const char* stop) {

    //  if desired, try first by consuming everything
    if (maxFirst) {
        UPDATE_REACHED(stop);
        if (!nextNode || nextNode->match(stop, stop)) {
            return (consumed = stop);
        }
        --stop;
    }

    // try positions one by one (use hint if available)
    const char* ret;
    do {
        if (useTab) {
            while (start < stop && !tab[(unsigned char)(*start)]) ++start;
        }
        ret = nextNode->match(start, stop);
        if (ret) {
            consumed = start;
            UPDATE_REACHED(start);
            return ret;
        }
        ++start;
    } while (start <= stop);

    UPDATE_REACHED(stop);
    return NULL;
}

bool CNode_Star::mayMatch(bool* tab) {
    if (tab) {
        for (int i=0; i<256; i++) tab[i] = true;
    }
    return maxFirst;
}

void CNode_Star::setNextNode(CNode* node) {
    nextNode = node;
    maxFirst = !nextNode || nextNode->mayMatch(NULL);
    useTab = false;
    if (!maxFirst) {
        for (int i=0; i<256; i++) tab[i] = false;
        node->mayMatch(tab);
    }
}


/* class CNode_MemStar
 * Try and match any string, and store the match. Corresponds to \0 or \#
 */
CNode_MemStar::CNode_MemStar(const char*& reached,
            CMemory& mem) : CNode(reached, MEMSTAR),
            memory(&mem), stack(NULL) {
}

CNode_MemStar::CNode_MemStar(const char*& reached,
            vector<CMemory>& st) : CNode(reached, MEMSTAR),
            memory(NULL), stack(&st) {
}

const char* CNode_MemStar::match(const char* start, const char* stop) {

    const char* left = start;

    // Backup memory and replace by a new one, or push new one on stack
    if (memory) {
        backup = *memory;
        (*memory)(left, stop);
    } else {
        stack->push_back(CMemory(left, stop));
    }

    //  if desired, try first by consuming everything
    if (maxFirst) {
        UPDATE_REACHED(stop);
        if (!nextNode || nextNode->match(stop, stop)) {
            return (consumed = stop);
        }
        --stop;
    }

    // try positions one by one (use hint if available)
    const char* ret;
    do {
        if (useTab) {
            while (start < stop && !tab[(unsigned char)(*start)]) ++start;
        }
        // Change stored/pushed memory
        if (memory) {
            (*memory)(left, start);
        } else {
            stack->back()(left, start);
        }
        ret = nextNode->match(start, stop);
        if (ret) {
            consumed = start;
            UPDATE_REACHED(start);
            return ret;
        }
        ++start;
    } while (start <= stop);

    // Undo backup.
    if (memory) {
        *memory = backup;
    } else {
        stack->pop_back();
    }
    UPDATE_REACHED(stop);
    return NULL;
}

bool CNode_MemStar::mayMatch(bool* tab) {
    if (tab) {
        for (int i=0; i<256; i++) tab[i] = true;
    }
    return maxFirst;
}

void CNode_MemStar::setNextNode(CNode* node) {
    nextNode = node;
    maxFirst = !nextNode || nextNode->mayMatch(NULL);
    useTab = false;
    if (!maxFirst) {
        for (int i=0; i<256; i++) tab[i] = false;
        node->mayMatch(tab);
    }
}


/* class CNode_Space
 * Matches any spaces.
 */
CNode_Space::CNode_Space(const char*& reached) : CNode(reached, SPACE) {
}

const char* CNode_Space::match(const char* start, const char* stop) {

    // Rule: space
    while (start < stop && *start <= ' ') ++start;

    const char* ret = nextNode ? nextNode->match(start, stop) : start;
    UPDATE_REACHED(start);
    consumed = start;
    return ret;
}

bool CNode_Space::mayMatch(bool* tab) {
    if (tab) {
        tab[' ']  = true;
        tab['\t'] = true;
        tab['\r'] = true;
        tab['\n'] = true;
    }
    return !nextNode || nextNode->mayMatch(tab);
}


/* class CNode_Equal
 * Matches an Equal sign surrounded by spaces.
 */
CNode_Equal::CNode_Equal(const char*& reached) : CNode(reached, EQUAL) {
}

const char* CNode_Equal::match(const char* start, const char* stop) {

    // Rule: =
    while (start < stop && *start <= ' ') ++start;
    if    (start < stop && *start == '=') {
        ++start;
    } else {
        UPDATE_REACHED(start);
        return NULL;
    }
    while (start < stop && *start <= ' ') ++start;

    const char* ret = nextNode ? nextNode->match(start, stop) : start;
    UPDATE_REACHED(start);
    consumed = start;
    return ret;
}

bool CNode_Equal::mayMatch(bool* tab) {
    if (tab) {
        tab['=']  = true;
        tab[' ']  = true;
        tab['\t'] = true;
        tab['\r'] = true;
        tab['\n'] = true;
    }
    return false;
}


/* class CNode_Quote
 * Try and match a " or a '.
 */
CNode_Quote::CNode_Quote(const char*& reached, char q) :
            CNode(reached, QUOTE), quote(q), matched(0), openingQuote(NULL) {
}

void CNode_Quote::setOpeningQuote(CNode_Quote *node) {
    openingQuote = node;
}

const char* CNode_Quote::match(const char* start, const char* stop) {

    if (start >= stop) return NULL;
    matched = *start++;
    // Rule: "
    // Rule: '
    if (!((   matched == '\'' && (   quote == '\"'
                                  || !openingQuote
                                  || openingQuote->matched == '\'' ))
          ||  (matched == '\"' && (   quote == '\"'
                                  || (openingQuote
                                      && openingQuote->matched == '\"'))) )) {
        return NULL;
    }

    const char* ret = nextNode ? nextNode->match(start, stop) : start;
    UPDATE_REACHED(start);
    consumed = start;
    return ret;
}

bool CNode_Quote::mayMatch(bool* tab) {
    if (tab) {
        tab['\"'] = true;
        tab['\''] = true;
    }
    return false;
}


/* class CNode_Char
 * Try and match a single character
 */
CNode_Char::CNode_Char(const char*& reached, char c) :
            CNode(reached, CHAR), byte(c) {
}

char CNode_Char::getChar() {
    return byte;
}

const char* CNode_Char::match(const char* start, const char* stop) {
    // The test is case insensitive
    if (start>=stop || tolower(*start) != byte) return NULL;
    start++;

    const char* ret = nextNode ? nextNode->match(start, stop) : start;
    UPDATE_REACHED(start);
    consumed = start;
    return ret;
}

bool CNode_Char::mayMatch(bool* tab) {
    if (tab) {
        tab[(unsigned char)byte] = true;
        tab[(unsigned char)toupper(byte)] = true;
    }
    return false;
}


/* class CNode_Range
 * Try and match a single character
 */
CNode_Range::CNode_Range(const char*& reached, int min, int max,
        bool allow) : CNode(reached, RANGE), min(min), max(max), allow(allow) {
}

const char* CNode_Range::match(const char* start, const char* stop) {

    // Check if there is a quote
    char quote = 0;
    if (start < stop && (*start == '\'' || *start == '\"')) {
        quote = *start++;
    }
    // Check if there is a minus sign
    int sign = 1;
    if (start < stop && *start == '-') {
        sign = -1;
        start++;
    }
    // Check if there is a digit
    if (start >= stop || !CUtil::digit(*start)) {
        UPDATE_REACHED(start);
        return NULL;
    }
    // Read the number
    int num = 0;
    while (start < stop && CUtil::digit(*start)) {
        num = num*10 + *start++ - '0';
    }
    num *= sign;
    // Check if optional quotes correspond
    if (quote && start < stop && *start == quote) {
        quote = 0;
        start++;
    }
    // Rule: [#]
    // Check if the number is in the range. Optional quote must be closed
    if (quote || (allow ^ (min <= num && num <= max))) return NULL;

    const char* ret = nextNode ? nextNode->match(start, stop) : start;
    UPDATE_REACHED(start);
    consumed = start;
    return ret;
}

bool CNode_Range::mayMatch(bool* tab) {
    if (tab) {
        for (int i='0'; i<='9'; i++) tab[i] = true;
        tab['-'] = true;
        tab['\''] = true;
        tab['\"'] = true;
    }
    return false;
}


/* class CNode_String
 * Try and match a string of characters.
 * Note: s and c must be in lowercase.
 */
CNode_String::CNode_String(const char*& reached, const string& s) :
            CNode(reached, STRING), str(s) {
    size = s.length();
}

void CNode_String::append(char c) {
    str += c;
    size++;
}

const char* CNode_String::match(const char* start, const char* stop) {
    const char* ptr = str.c_str();
    const char* max = (stop < start+size ? stop : start+size);
    while (start < max && *ptr == tolower(*start)) { ptr++; start++; }
    if (ptr < str.c_str() + size) {
        UPDATE_REACHED(start);
        return NULL;
    }

    const char* ret = nextNode ? nextNode->match(start, stop) : start;
    UPDATE_REACHED(start);
    consumed = start;
    return ret;
}

bool CNode_String::mayMatch(bool* tab) {
    if (tab) {
        tab[(unsigned char)str[0]] = true;
        tab[(unsigned char)toupper(str[0])] = true;
    }
    return false;
}


/* class CNode_Chars
 * Try and match a single character.
 */
CNode_Chars::CNode_Chars(const char*& reached, string s, bool allow) :
            CNode(reached, CHARS), allow(allow){
    // For a fast matching, all allowed (or forbidden) characters are
    // tagged in a table of booleans.
    unsigned int i;
    for (i=0; i<256; i++) byte[i] = !allow;
    for (string::iterator it = s.begin(); it != s.end(); it++) {
        byte[(unsigned char)(*it)] = allow;
    }
}

void CNode_Chars::add(unsigned char c) {
    // We can add characters to the list after the construction
    byte[c] = allow;
}

const char* CNode_Chars::match(const char* start, const char* stop) {
    if (start>=stop || !byte[(unsigned char)(*start)]) return NULL;
    start++;

    const char* ret = nextNode ? nextNode->match(start, stop) : start;
    UPDATE_REACHED(start);
    consumed = start;
    return ret;
}

bool CNode_Chars::mayMatch(bool* tab) {
    if (tab) {
        for (int i=0; i<256; i++) if (byte[i]) tab[i] = true;
    }
    return false;
}


/* class CNode_Empty
 * Empty pattern. Always matches (but only once) or never, depending on accept
 */
CNode_Empty::CNode_Empty(const char*& reached, bool accept) :
        CNode(reached, EMPTY), accept(accept) {
}

const char* CNode_Empty::match(const char* start, const char* stop) {
    if (!accept) return NULL;

    const char* ret = nextNode ? nextNode->match(start, stop) : start;
    UPDATE_REACHED(start);
    consumed = start;
    return ret;
}

bool CNode_Empty::mayMatch(bool* tab) {
    return accept && (!nextNode || nextNode->mayMatch(tab));
}


/* class CNode_Any
 * Matches any character. Corresponds to ?
 */
CNode_Any::CNode_Any(const char*& reached) : CNode(reached, ANY) {
}

const char* CNode_Any::match(const char* start, const char* stop) {
    // Rule: ?
    if (start >= stop) return NULL;
    start++;
    
    const char* ret = nextNode ? nextNode->match(start, stop) : start;
    UPDATE_REACHED(start);
    consumed = start;
    return ret;
}

bool CNode_Any::mayMatch(bool* tab) {
    if (tab) {
        for (int i=0; i<256; i++) tab[i] = true;
    }
    return false;
}


/* class CNode_Run
 * Try and match a seauence of nodes.
 */
CNode_Run::CNode_Run(const char*& reached, vector<CNode*> *vect) :
            CNode(reached, RUN) {
    nodes = vect;
    firstNode = *nodes->begin();
}

CNode_Run::~CNode_Run() {
    CUtil::deleteVector<CNode>(*nodes);
    delete nodes;
}

const char* CNode_Run::match(const char* start, const char* stop) {
    const char* ret = firstNode->match(start, stop);
    consumed = nodes->back()->consumed;
    return ret;
}

bool CNode_Run::mayMatch(bool* tab) {
    return firstNode->mayMatch(tab);
}

void CNode_Run::setNextNode(CNode* node) {
    nextNode = NULL;
    for (vector<CNode*>::reverse_iterator it = nodes->rbegin();
                it != nodes->rend(); it++) {
        (*it)->setNextNode(node);
        node = *it;
    }
}


/* class CNode_Or
 * Try and match nodes one after another
 */
CNode_Or::CNode_Or(const char*& reached, vector<CNode*> *vect) :
            CNode(reached, OR) {
    nodes = vect;
}

CNode_Or::~CNode_Or() {
    CUtil::deleteVector<CNode>(*nodes);
    delete nodes;
}

const char* CNode_Or::match(const char* start, const char* stop) {

    for (vector<CNode*>::iterator iter = nodes->begin(); iter != nodes->end(); iter++) {
        const char* ret = (*iter)->match(start, stop);
        if (ret) {
            consumed = (*iter)->consumed;
            return ret;
        }
    }
    return NULL;
}

bool CNode_Or::mayMatch(bool* tab) {
    bool ret = false;
    for (unsigned int i=0; i<nodes->size(); i++)
        if ((*nodes)[i]->mayMatch(tab)) ret = true;
    return ret;
}

void CNode_Or::setNextNode(CNode* node) {
    nextNode = NULL;
    for (vector<CNode*>::iterator it = nodes->begin();
                it != nodes->end(); it++) {
        (*it)->setNextNode(node);
    }
}


/* class CNode_And
 * Try and match left node then right node (returns max length of both).
 * 'force' is for "&&", limiting right to matching exactly what left matched
 */
CNode_And::CNode_And(const char*& reached, CNode *L, CNode *R,
        bool force) : CNode(reached, AND), nodeL(L), nodeR(R), force(force) {
}

CNode_And::~CNode_And() {
    delete nodeL;
    delete nodeR;
}

const char* CNode_And::match(const char* start, const char* stop) {

    // Ask left node for the first match
    const char* posL = nodeL->match(start, stop);
    if (!posL) return NULL;
    consumed = nodeL->consumed;

    // Ask right node for the first match
    const char* posR = nodeR->match(start, (force ? consumed : stop));
    if (!posR || (force && posR != consumed)) return NULL;
    if (consumed < nodeR->consumed) consumed = nodeR->consumed;
    return (posL > posR ? posL : posR);
}

bool CNode_And::mayMatch(bool* tab) {
    bool tabL[256], tabR[256], retL, retR;
    for (int i=0; i<256; i++) tabL[i] = tabR[i] = false;
    retL = nodeL->mayMatch(tabL);
    retR = nodeR->mayMatch(tabR);
    if (tab) {
        if (!retL && !retR) {
            for (int i=0; i<256; i++)
                if (tabL[i] && tabR[i]) tab[i] = true;
        } else {
            for (int i=0; i<256; i++)
                if ((retR && tabL[i]) || (retL && tabR[i])) tab[i] = true;
        }
    }
    return retL && retR;
}

void CNode_And::setNextNode(CNode* node) {
    nextNode = NULL;
    nodeL->setNextNode(node);
    nodeR->setNextNode(force ? NULL : node);
}


/* class CNode_Repeat
 * Try and match a pattern several times
 */
CNode_Repeat::CNode_Repeat(const char*& reached, CNode *node,
            int rmin, int rmax, bool iter) : CNode(reached, REPEAT),
            node(node), rmin(rmin), rmax(rmax), iterate(iter) {
}
CNode_Repeat::~CNode_Repeat() {
    delete node;
}

const char* CNode_Repeat::match(const char* start, const char* stop) {

    int rcount = 0;
    bool checked = false;
    while (rcount < rmax) {
        if (iterate && nextNode && rcount >= rmin) {
            const char* ret = nextNode->match(start, stop);
            if (ret) {
                consumed = start;
                return ret;
            }
            checked = true;
        }
        const char* ret = node->match(start, stop);
        if (!ret) break;
        checked = false;
        if (ret == start) { rcount = rmax; break; } // infinite-loop protection
        start = ret;
        rcount++;
    }
    if (rcount < rmin || checked) return NULL;

    const char* ret = nextNode ? nextNode->match(start, stop) : start;
    consumed = start;
    return ret;
}

bool CNode_Repeat::mayMatch(bool* tab) {
    bool ret = node->mayMatch(tab);
    return ret && rmin>0 ? false : (!nextNode || nextNode->mayMatch(tab));
}

void CNode_Repeat::setNextNode(CNode* node) {
    nextNode = node;
    this->node->setNextNode(NULL);
}


/* class CNode_Memory
 * Try and match something, and if it does, store the position with a CMemory
 */
CNode_Memory::CNode_Memory(const char*& reached, CNode *node, CMemory& mem) :
                        CNode(reached, MEMORY), node(node), memory(&mem), stack(NULL) {

    memorizer = !node ? NULL : new CNode_Memory(reached, NULL, mem);
}

CNode_Memory::CNode_Memory(const char*& reached, CNode *node, vector<CMemory>& st) :
                        CNode(reached, MEMORY), node(node), memory(NULL), stack(&st) {

    memorizer = !node ? NULL : new CNode_Memory(reached, NULL, st);
}

CNode_Memory::~CNode_Memory() {
    if (node) delete node;
    if (memorizer) delete memorizer;
}

const char* CNode_Memory::match(const char* start, const char* stop) {

    if (memorizer) {	// 親Memoryの時に通る。子Memory(memorizer)に現在の位置を記憶しておく
        memorizer->recordPos = start;
        const char* ret = node->match(start, stop);
        consumed = node->consumed;
        return ret;
    } else {	// setNextNodeで次のマッチの前にmemorizerがよばれるようになっている
        // Backup memory and replace by a new one, or push new one on stack
        if (memory) {
            backup = *memory;
            (*memory)(recordPos, start);
        } else {
            stack->push_back(CMemory(recordPos, start));
        }
        // Ask next node
        const char* ret = nextNode ? nextNode->match(start, stop) : start;
        if (ret) {
            consumed = start;
            return ret;
        }
        // Undo backup.
        if (memory) {
            *memory = backup;
        } else {
            stack->pop_back();
        }
        return NULL;
    }
}

bool CNode_Memory::mayMatch(bool* tab) {
    if (memorizer) {
        return node->mayMatch(tab);
    } else {
        return !nextNode || nextNode->mayMatch(tab);
    }
}

void CNode_Memory::setNextNode(CNode* node) {
    if (memorizer) {
        nextNode = NULL;
        memorizer->setNextNode(node);
        this->node->setNextNode(memorizer);
    } else {
        nextNode = node;
    }
}


/* class CNode_Negate
 * Try and match a pattern. If it does, failure (NULL), else success (start)
 */
CNode_Negate::CNode_Negate(const char*& reached, CNode *node) :
            CNode(reached, NEGATE), node(node) {
}

CNode_Negate::~CNode_Negate() {
    delete node;
}

const char* CNode_Negate::match(const char* start, const char* stop) {
    // The negation node does not consume text
    const char* test = node->match(start, stop);
    if (test) return NULL;
    const char* ret = nextNode ? nextNode->match(start, stop) : start;
    consumed = start;
    return ret;
}

bool CNode_Negate::mayMatch(bool* tab) {
    return !nextNode || nextNode->mayMatch(tab);
}

void CNode_Negate::setNextNode(CNode* node) {
    nextNode = node;
    this->node->setNextNode(NULL);
}


/* class CNode_AV
 * Try and match an html parameter
 */
CNode_AV::CNode_AV(const char*& reached, CNode *node, bool isAVQ) :
            CNode(reached, AV), node(node), isAVQ(isAVQ) {
}

CNode_AV::~CNode_AV() {
    delete node;
}

const char* CNode_AV::match(const char* start, const char* stop) {

    // find parameter limits
    bool consumeQuote = false;
    const char *begin, *end;
    begin = end = start;
    if (start<stop) {
        char token = *start;
        if (token == '\'' || token == '\"') {
            // We'll try and match a quoted parameter. Look for closing quote.
            end++;
            if (!isAVQ) begin++; // AV: the matching will start after the quote
            while (end<stop && *end!=token) end++;
            if (end<stop) {
                if (isAVQ)
                    end++; // AVQ: the matching will include the closing quote
                else
                    // AV: if we match the interior, we will
                    // consume the closing quote
                    consumeQuote = true;
            }
        } else {
            // Parameter without quote (single word), look for its end
            while (end<stop && *end > ' ' && *end != '>') end++;
            if (end == begin) return NULL;
        }
    }
    
    // test parameter value
    begin = node->match(begin, end);
    if (begin != end) return NULL;
    start = consumeQuote ? end+1 : end;

    const char* ret = nextNode ? nextNode->match(start, stop) : start;
    UPDATE_REACHED(start);
    consumed = start;
    return ret;
}

bool CNode_AV::mayMatch(bool* tab) {
    if (tab) {
        tab['\''] = true;
        tab['\"'] = true;
    }
    return node->mayMatch(tab);
}

void CNode_AV::setNextNode(CNode* node) {
    nextNode = node;
    this->node->setNextNode(NULL);
}


/* class CNode_Url
 * Try and match a part of URL
 */
CNode_Url::CNode_Url(const char*& reached, const CUrl& url, char token) :
            CNode(reached, URL), url(url), token(token) {
}

const char* CNode_Url::match(const char* start, const char* stop) {
    const char* ptr = NULL;
    switch (token) {
        case 'u': ptr = url.getUrl().c_str();     break;
        case 'h': ptr = url.getHost().c_str();    break;
        case 'p': ptr = url.getPath().c_str();    break;
        case 'q': ptr = url.getQuery().c_str();   break;
        case 'a': ptr = url.getAnchor().c_str();  break;
    }
    if (*ptr == '\0') return NULL;
    while (start < stop && *ptr != '\0' && tolower(*ptr) == tolower(*start)) {
        ptr++; start++;
    }
    if (*ptr != '\0') {
        UPDATE_REACHED(start);
        return NULL;
    }

    const char* ret = nextNode ? nextNode->match(start, stop) : start;
    UPDATE_REACHED(start);
    consumed = start;
    return ret;
}

bool CNode_Url::mayMatch(bool* tab) {
    // At construction time, we don't have the URL address
    // (it will be set/changed later, or when recyling the filter)
    // so we must allow everything.
    if (tab) {
        for (int i=0; i<256; i++) tab[i] = true;
    }
    return false;
}


/* class CNode_List
 * Try and match nodes one after another, in CSettings::lists order.
 * Corresponds to $LST() command.
 */
CNode_List::CNode_List(const char*& reached, const string& name, CMatcher& matcher) :
            CNode(reached, LIST), name(name), matcher(matcher) {

    // we don't want the list to change while we read it
    //std::lock_guard<std::recursive_mutex> lock1(CSettings::ref().listsMutex);
    std::lock_guard<std::recursive_mutex> lock2(objectsMutex);
    // parse all the patterns from the list
    //deque<string>& list = CSettings::ref().lists[name];
	std::lock_guard<std::recursive_mutex> lock(CSettings::s_mutexLists);
	deque<string>& list = CSettings::s_mapLists[name];
    for (deque<string>::iterator it = list.begin(); it != list.end(); it++) {
        pushPattern(*it);
    }
    // register this object for incremental list parsing
    objects.insert(this);
}

CNode_List::~CNode_List() {

    std::lock_guard<std::recursive_mutex> lock1(objectsMutex);
    std::lock_guard<std::recursive_mutex> lock2(hashedMutex);
    // unregister this object
    objects.erase(this);
    // delete built nodes. They are pointed to by the hashed lists.
    for (int i=0; i<256; i++) {
        deque<SListItem>& h = hashed[i];
        for (deque<SListItem>::iterator it = h.begin(); it != h.end(); it++) {
            // unhashed nodes must be deleted only once (for i=0)
            if ((it->flags & 0x2) || i==0) {
                delete it->node;
            }
        }
    }
}

std::recursive_mutex CNode_List::objectsMutex;

set<CNode_List*> CNode_List::objects;

bool CNode_List::isHashable(char c) {
    return (c != '\\' && c != '[' && c != '$'  && c != '('  && c != ')'  &&
            c != '|'  && c != '&' && c != '?'  && c != ' '  && c != '\t' &&
            c != '='  && c != '*' && c != '\'' && c != '\"' );
}

void CNode_List::pushPattern(const string& pattern) {

    std::lock_guard<std::recursive_mutex> lock(hashedMutex);
    // we'll record the built node and its flags in a structure
    SListItem item = { NULL, 0 };
    if (pattern[0] == '~') item.flags |= 0x1;
    int start = (item.flags & 0x1 ? 1 : 0);
    char c = pattern[start];
    if (isHashable(c)) item.flags |= 0x2;
    // parse the pattern
    item.node = matcher.expr(pattern, start, pattern.size());
    item.node->setNextNode(NULL);
    
    if (item.flags & 0x2) {
        // hashable pattern goes in a single hashed bucket (lowercase)
        hashed[hashBucket(c)].push_back(item);
    } else {
        // Expressions that start with a special char cannot be hashed
        // Push them on every hashed list
        for (size_t i = 0; i < sizeof(hashed) / sizeof(hashed[0]); i++)
            if (!isupper((char)i))
                hashed[i].push_back(item);
    }
}

void CNode_List::popPattern(const string& pattern) {

    std::lock_guard<std::recursive_mutex> lock(hashedMutex);
    // since the first pattern in the list is also on the front of relevant
    // hashed buckets, we can remove it by popping nodes from the buckets.
    char c = (pattern[0] == '~' ? pattern[1] : pattern[0]);
    if (isHashable(c)) {
        delete hashed[hashBucket(c)].front().node;  // delete the node first!
        hashed[hashBucket(c)].pop_front();
    } else {
        delete hashed[0].front().node;              // delete the node first!
        for (size_t i = 0; i < sizeof(hashed) / sizeof(hashed[0]); i++)
            if (!isupper((char)i))
                hashed[i].pop_front();
    }
}

void CNode_List::notifyPatternPushBack(const string& listname, const string& pattern) {

    std::lock_guard<std::recursive_mutex> lock(objectsMutex);
    // push pattern on all objects registered on this list
    for (set<CNode_List*>::iterator it = objects.begin(); it != objects.end(); it++) {
        if (listname == (*it)->name) (*it)->pushPattern(pattern);
    }
}

void CNode_List::notifyPatternPopFront(const string& listname, const string& pattern) {

    std::lock_guard<std::recursive_mutex> lock(objectsMutex);
    // pop pattern from all objects registered on this list
    for (set<CNode_List*>::iterator it = objects.begin(); it != objects.end(); it++) {
        if (listname == (*it)->name) (*it)->popPattern(pattern);
    }
}

const char* CNode_List::match(const char* start, const char* stop) {

    if (start < stop) {
        // Check the hashed list corresponding to the first char
        std::lock_guard<std::recursive_mutex> lock(hashedMutex);
        deque<SListItem>& h = hashed[hashBucket(*start)];
        for (deque<SListItem>::iterator it = h.begin(); it != h.end(); it++) {
            const char* ptr = it->node->match(start, stop);
            if (ptr) {
                if (it->flags & 0x1) return NULL;   // the pattern is a ~...
                start = ptr;
                const char* ret = nextNode ? nextNode->match(start, stop) : start;
                consumed = start;
                return ret;
            }
        }
    }
    return NULL;
}

bool CNode_List::mayMatch(bool* tab) {
    if (tab) {
        // We cannot do incremental updates of tab (because it sometimes is
        // a temporary array) so we must allow all characters to get through.
        for (int i=0; i<256; i++) tab[i] = true; 
    }
    return !nextNode || nextNode->mayMatch(NULL);
}

void CNode_List::setNextNode(CNode* node) {
    nextNode = node;
}


/* class CNode_Command
 * Executes a command that does not consume anything.
 */
CNode_Command::CNode_Command(const char*& reached, CMD_ID num,
        string name, string content, CFilter& filter) :
        CNode(reached, COMMAND), num(num), name(name), content(content),
        filter(filter), owner(filter.owner) {
    matcher = NULL;
    // For some commands we'll build a CMatcher (content is a pattern)
    if (num >= 100) {
        matcher = new CMatcher(content, filter);
    }
}

CNode_Command::~CNode_Command() {
    if (matcher) delete matcher;
}

const char* CNode_Command::match(const char* start, const char* stop) {
    int tmp;
    const char *tStart, *tStop, *tEnd, *tReached;

    switch (num) {

    case CMD_TSTSHARP:
        if (filter.memoryStack.empty()) return NULL;
        toMatch = filter.memoryStack.back().getValue();
        tStart = toMatch.c_str();
        tStop = tStart + toMatch.size();
        if (toMatch.empty()
               || !matcher->match(tStart, tStop, tEnd, tReached)
               || tEnd != tStop ) {
            return NULL;
        }
        break;

    case CMD_TSTEXPAND:
        toMatch = CExpander::expand(name, filter);
        tStart = toMatch.c_str();
        tStop = tStart + toMatch.size();
        if (toMatch.empty()
               || !matcher->match(tStart, tStop, tEnd, tReached)
               || tEnd != tStop ) {
            return NULL;
        }
        break;

    case CMD_TSTDIGIT:
        toMatch = filter.memoryTable[name[0]-'0'].getValue();
        tStart = toMatch.c_str();
        tStop = tStart + toMatch.size();
        if (toMatch.empty()
               || !matcher->match(tStart, tStop, tEnd, tReached)
               || tEnd != tStop ) {
            return NULL;
        }
        break;

    case CMD_TSTVAR:
        toMatch = owner.variables[name];
        tStart = toMatch.c_str();
        tStop = tStart + toMatch.size();
        if (toMatch.empty()
               || !matcher->match(tStart, tStop, tEnd, tReached)
               || tEnd != tStop ) {
            return NULL;
        }
        break;

    case CMD_URL:
        toMatch = owner.url.getUrl();
        tStart = toMatch.c_str();
        tStop = tStart + toMatch.size();
        if (!matcher->match(tStart, tStop, tEnd, tReached)) return NULL;
        break;

    case CMD_IHDR:
        toMatch = CFilterOwner::GetHeader(owner.inHeaders, name);
        tStart = toMatch.c_str();
        tStop = tStart + toMatch.size();
        if (!matcher->match(tStart, tStop, tEnd, tReached)) return NULL;
        break;

    case CMD_OHDR:
        toMatch = CFilterOwner::GetHeader(owner.outHeaders, name);
        tStart = toMatch.c_str();
        tStop = tStart + toMatch.size();
        if (!matcher->match(tStart, tStop, tEnd, tReached)) return NULL;
        break;

    case CMD_RESP:
        toMatch = owner.responseCode;
        tStart = toMatch.c_str();
        tStop = tStart + toMatch.size();
        if (!matcher->match(tStart, tStop, tEnd, tReached)) return NULL;
        break;

    case CMD_SETSHARP:
        filter.memoryStack.push_back(CMemory(content));
        break;

    case CMD_SETDIGIT:
        filter.memoryTable[name[0]-'0'] = CMemory(content);
        break;

    case CMD_SETVAR:
        owner.variables[name] = CExpander::expand(content, filter);
        break;

    case CMD_KEYCHK:
        if (!CUtil::keyCheck(content)) return NULL;
        break;

    case CMD_KILL:
        // \k acts as a command (it changes variables and does not consume)
        // so it is processed by CNode_Command.
        owner.killed = true;
        break;

    case CMD_ADDLST:
        //CSettings::ref().addListLine(name, CExpander::expand(content, filter));
        break;
        
    case CMD_ADDLSTBOX:
        {
            string title = APP_NAME;
            string value = content;
            size_t comma = CUtil::findUnescaped(value, ',');
            if (comma != string::npos) {
                title = value.substr(0, comma);
                value = value.substr(comma + 1);
            }
            title = CExpander::expand(title, filter);
            value = CExpander::expand(value, filter);
            //string message = CSettings::ref().getMessage(
            //                        "ADDLSTBOX_MESSAGE", name);
            //wxTextEntryDialog dlg(NULL, S2W(message), S2W(title), S2W(value));
            //if (dlg.ShowModal() == wxID_OK)
            //    CSettings::ref().addListLine(name, W2S(dlg.GetValue()));
        }
        break;
    
    case CMD_ALERT:
        //wxMessageBox(S2W(CExpander::expand(content, filter)), wxT(APP_NAME));
        break;

    case CMD_CONFIRM:
        //tmp = wxMessageBox(S2W(CExpander::expand(content, filter)),
        //                   wxT(APP_NAME), wxYES_NO);
        //if (tmp != wxYES) return NULL;
        break;

    case CMD_TYPE:
        if (content != owner.fileType) return NULL;
        break;

    case CMD_STOP:
        filter.bypassed = true;
        break;

    case CMD_USEPROXY:
        owner.useSettingsProxy = (content[0]=='t');
        break;

    case CMD_SETPROXY:
        //for (set<string>::iterator it = CSettings::ref().proxies.begin();
        //            it != CSettings::ref().proxies.end(); it++) {
        //    if (CUtil::noCaseBeginsWith(content, *it)) {
        //        owner.contactHost = *it;
        //        owner.useSettingsProxy = false;
        //        break;
        //    }
        //}
        break;

    case CMD_LOG:
        {
            string log = CExpander::expand(content, filter);
			CLog::FilterEvent(kLogFilterLogCommand, owner.requestNumber, filter.title, log);
        }
        break;

    case CMD_JUMP:
        owner.rdirToHost = CExpander::expand(content, filter);
        CUtil::trim(owner.rdirToHost);
        owner.rdirMode = 0;
		CLog::FilterEvent(kLogFilterJump, owner.requestNumber, filter.title, owner.rdirToHost);
        break;

    case CMD_RDIR:
        owner.rdirToHost = CExpander::expand(content, filter);
        CUtil::trim(owner.rdirToHost);
        owner.rdirMode = 1;
		CLog::FilterEvent(kLogFilterRdir, owner.requestNumber, filter.title, owner.rdirToHost);
        break;

    case CMD_FILTER:
        owner.bypassBody = (content[0]!='t');
        owner.bypassBodyForced = true;
        break;

    case CMD_LOCK:
        if (!filter.locked) {
            //CLog::ref().filterLock.Lock();
            filter.locked = true;
        }
        break;

    case CMD_UNLOCK:
        if (filter.locked) {
            //CLog::ref().filterLock.Unlock();
            filter.locked = false;
        }
        break;
    
    } // end of switch

    const char* ret = nextNode ? nextNode->match(start, stop) : start;
    consumed = start;
    return ret;
}

bool CNode_Command::mayMatch(bool* tab) {
    return !nextNode || nextNode->mayMatch(tab);
}


/* class CNode_Cnx
 * Matches depending on connection number.
 */
CNode_Cnx::CNode_Cnx(const char*& reached, int x, int y, int z, int& num) :
        CNode(reached, CNX), x(x), y(y), z(z), num(num) {
}

const char* CNode_Cnx::match(const char* start, const char* stop) {
    if ((((num-1)/z)%y) != x-1) return NULL;

    const char* ret = nextNode ? nextNode->match(start, stop) : start;
    consumed = start;
    return ret;
}

bool CNode_Cnx::mayMatch(bool* tab) {
    return !nextNode || nextNode->mayMatch(tab);
}


/* class CNode_Nest
 * Matches nested tags, with optional content.
 */
CNode_Nest::CNode_Nest(const char*& reached, CNode* left,
        CNode* middle, CNode* right, bool inest) : CNode(reached, NEST),
        left(left), middle(middle), right(right), inest(inest) {
    int i;
    for (i=0; i<256; i++) tabL[i] = false;
    if (left->mayMatch(tabL)) for (i=0; i<256; i++) tabL[i] = true;
    for (i=0; i<256; i++) tabR[i] = false;
    if (right->mayMatch(tabR)) for (i=0; i<256; i++) tabR[i] = true;
}

CNode_Nest::~CNode_Nest() {
    if (left)   delete left;
    if (middle) delete middle;
    if (right)  delete right;
}

const char* CNode_Nest::match(const char* start, const char* stop) {

    int level;
    const char *endL = start, *startR, *pos;
    if (!inest) {
        if (!tabL[(unsigned char)(*start)]) return NULL;
        endL = left->match(start, stop);
        if (!endL) return NULL;
    }
    startR = pos = endL;
    level = 1;
    while (pos < stop && level > 0) {
        if (tabR[(unsigned char)(*pos)]) {
            const char* end = right->match(pos, stop);
            if (end > pos) {
                level--;
                startR = pos;
                pos = end;
                continue;
            }
        }
        if (tabL[(unsigned char)(*pos)]) {
            const char* end = left->match(pos, stop);
            if (end > pos) {
                level++;
                pos = end;
                continue;
            }
        }
        pos++;
    }
    if (level > 0) return NULL;
    if (middle) {
        const char* end = middle->match(endL, startR);
        if (end != startR) return NULL;
    }
    start = inest ? startR : pos;
    
    const char* ret = nextNode ? nextNode->match(start, stop) : start;
    consumed = start;
    return ret;
}

bool CNode_Nest::mayMatch(bool* tab) {
    bool ret = true;
    if (ret && !inest)   ret = left->mayMatch(tab);
    if (ret && middle)   ret = middle->mayMatch(tab);
    if (ret && !inest)   ret = right->mayMatch(tab);
    if (ret && nextNode) ret = nextNode->mayMatch(tab);
    return ret;
}

void CNode_Nest::setNextNode(CNode* node) {
    nextNode = node;
    left->setNextNode(NULL);
    right->setNextNode(NULL);
    if (middle) middle->setNextNode(NULL);
}


/* class CNode_Test
 * Try and match a string of characters.
 */
CNode_Test::CNode_Test(const char*& reached, string name,
        CFilter& filter) : CNode(reached, TEST), name(name), filter(filter) {
}

const char* CNode_Test::match(const char* start, const char* stop) {
    string str;     // variable content, that will be compared to text
    if (name == "#") {
        if (!filter.memoryStack.empty())
            str = filter.memoryStack.back().getValue();
    } else if (name.size() == 1 && CUtil::digit(name[0])) {
        str = filter.memoryTable[name[0]-'0'].getValue();
    } else if (name[0] == '(' && name[name.size()-1] == ')') {
        str = CExpander::expand(name.substr(1, name.size()-2), filter);
    } else {
        str = filter.owner.variables[name];
    }
    int size = str.size();
    if (!size) return NULL;

    const char* ptr = str.c_str();
    const char* max = (stop < start+size ? stop : start+size);
    while (start < max && *ptr == tolower(*start)) { ptr++; start++; }
    if (ptr < str.c_str() + size) {
        UPDATE_REACHED(start);
        return NULL;
    }

    const char* ret = nextNode ? nextNode->match(start, stop) : start;
    UPDATE_REACHED(start);
    consumed = start;
    return ret;
}

bool CNode_Test::mayMatch(bool* tab) {
    // We don't know at construction time what the string to match is.
    // So we must allow everything.
    if (tab) {
        for (int i=0; i<256; i++) tab[i] = true;
    }
    return !nextNode || nextNode->mayMatch(NULL);
}


/* class CNode_Ask
 * Automates the insertion of an item in a list based on user choice.
 */
CNode_Ask::CNode_Ask(const char*& reached, CFilter& filter,
        string allowName, string denyName, string question, string item,
        string pattern) : CNode(reached, ASK), filter(filter),
        allowName(allowName), denyName(denyName), question(question),
        item(item), pattern(pattern) {

    allowMatcher = new CMatcher("$LST(" + allowName + ")", filter);
    try {
        denyMatcher = new CMatcher("$LST(" + denyName + ")", filter);
    } catch (parsing_exception e) {
        delete allowMatcher;
        throw e;
    }
}

CNode_Ask::~CNode_Ask() {
    delete allowMatcher;
    delete denyMatcher;
}

const char* CNode_Ask::match(const char* start, const char* stop) {

    // We lock so that 2 documents doing the same test at the same time
    // won't ask the question twice. No need unlocking afterwards.
    if (!filter.locked) {
        //CLog::ref().filterLock.Lock();
        filter.locked = true;
    }
    const char *tStart, *tStop, *tEnd, *tReached;
    toMatch = CExpander::expand(pattern, filter);
    tStart = toMatch.c_str();
    tStop = tStart + toMatch.size();
    // If the pattern is found in Allow list, we return a non-match
    // (so that the filter does not operate)
    if (allowMatcher->match(tStart, tStop, tEnd, tReached)) return NULL;
    // On the contrary, if found in Deny list, we return a match
    // (to continue filtering)
    if (!denyMatcher->match(tStart, tStop, tEnd, tReached)) {
        // Now we'll ask the user what they want to do with it
#if 0
        int tmp = wxMessageBox(S2W(CExpander::expand(question, filter)),
                               wxT(APP_NAME), wxYES_NO);
        // Then add the item to one list.
        if (tmp == wxYES) {
            CSettings::ref().addListLine(allowName, CExpander::expand(item, filter));
            return NULL;
        } else {
            CSettings::ref().addListLine(denyName, CExpander::expand(item, filter));
        }
#endif
    }

    const char* ret = nextNode ? nextNode->match(start, stop) : start;
    consumed = start;
    return ret;
}

bool CNode_Ask::mayMatch(bool* tab) {
    return !nextNode || nextNode->mayMatch(NULL);
}
// vi:ts=4:sw=4:et
