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
#include "proximodo\util.h"
#include "Matcher.h"
#include "proximodo\expander.h"
#include "FilterOwner.h"
#include "Log.h"
#include "Settings.h"
#include "CodeConvert.h"
#include <unicode\uchar.h>

using namespace CodeConvert;


namespace Proxydomo {

inline void UpdateReached(const UChar* p, MatchData* pMatch)
{
	if (pMatch->reached < p)
		pMatch->reached = p;
}

/* class CNode_Star
 * Try and match any string. Corresponds to *
 */

const UChar* CNode_Star::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
    //  if desired, try first by consuming everything
    if (m_maxFirst) {
        UpdateReached(stop, pMatch);
        if (m_nextNode == nullptr || m_nextNode->match(stop, stop, pMatch)) {
			pMatch->consumed = stop;
            return pMatch->consumed;
        }
        --stop;
    }

    // try positions one by one (use hint if available)
    do {
        if (m_useTab) {
            while (start < stop && m_tab[(unsigned char)(*start)] == false) ++start;
        }
        const UChar* ret = m_nextNode->match(start, stop, pMatch);
        if (ret) {
            pMatch->consumed = start;
            UpdateReached(start, pMatch);
            return ret;
        }
        ++start;
    } while (start <= stop);

    UpdateReached(stop, pMatch);
    return nullptr;
}


bool CNode_Star::mayMatch(bool* tab)
{
    if (tab) {
        for (int i=0; i<256; i++) tab[i] = true;
    }
    return m_maxFirst;
}

void CNode_Star::setNextNode(CNode* node)
{
    m_nextNode = node;
    m_maxFirst = m_nextNode == nullptr || m_nextNode->mayMatch(nullptr);
    m_useTab = false;
    if (m_maxFirst == false) {
        for (int i=0; i<256; i++) m_tab[i] = false;
        node->mayMatch(m_tab);
    }
}



/* class CNode_MemStar
 * Try and match any string, and store the match. Corresponds to \0 or \#
 */
const UChar* CNode_MemStar::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
    const UChar* left = start;
	CMemory backup;
    // Backup memory and replace by a new one, or push new one on stack
    if (m_memoryPos != -1) {		
        backup = pMatch->pFilter->memoryTable[m_memoryPos];
        pMatch->pFilter->memoryTable[m_memoryPos](left, stop);
    } else {
		pMatch->pFilter->memoryStack.push_back(CMemory(left, stop));
    }

    //  if desired, try first by consuming everything
    if (m_maxFirst) {
		UpdateReached(stop, pMatch);
        if (m_nextNode == nullptr || m_nextNode->match(stop, stop, pMatch)) {
			pMatch->consumed = stop;
            return pMatch->consumed;
        }
        --stop;
    }

    // try positions one by one (use hint if available)
    do {
        if (m_useTab) {
            while (start < stop && m_tab[(unsigned char)(*start)] == false) ++start;
        }
        // Change stored/pushed memory
        if (m_memoryPos != -1) {
            pMatch->pFilter->memoryTable[m_memoryPos](left, start);
        } else {
            pMatch->pFilter->memoryStack.back()(left, start);
        }
        const UChar* ret = m_nextNode->match(start, stop, pMatch);
        if (ret) {
            pMatch->consumed = start;
			UpdateReached(start, pMatch);
            return ret;
        }
        ++start;
    } while (start <= stop);

    // Undo backup.
    if (m_memoryPos != -1) {
        pMatch->pFilter->memoryTable[m_memoryPos] = backup;
    } else {
		pMatch->pFilter->memoryStack.pop_back();
    }
	UpdateReached(stop, pMatch);
    return nullptr;
}

bool CNode_MemStar::mayMatch(bool* tab)
{
    if (tab) {
        for (int i=0; i<256; i++) tab[i] = true;
    }
    return m_maxFirst;
}

void CNode_MemStar::setNextNode(CNode* node)
{
    m_nextNode = node;
    m_maxFirst = m_nextNode == nullptr || m_nextNode->mayMatch(NULL);
    m_useTab = false;
    if (m_maxFirst == false) {
        for (int i=0; i<256; i++) m_tab[i] = false;
        node->mayMatch(m_tab);
    }
}



/* class CNode_Space
 * Matches any spaces.
 */

const UChar* CNode_Space::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
    // Rule: space
    while (start < stop && *start <= ' ') ++start;

    const UChar* ret = m_nextNode ? m_nextNode->match(start, stop, pMatch) : start;
	UpdateReached(start, pMatch);
    pMatch->consumed = start;
    return ret;
}

bool CNode_Space::mayMatch(bool* tab)
{
    if (tab) {
        tab[' ']  = true;
        tab['\t'] = true;
        tab['\r'] = true;
        tab['\n'] = true;
    }
    return m_nextNode == nullptr || m_nextNode->mayMatch(tab);
}



/* class CNode_Equal
 * Matches an Equal sign surrounded by spaces.
 */
/// '=' 前後のスペースを含めて消費する
const UChar* CNode_Equal::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
    // Rule: =
    while (start < stop && *start <= L' ') ++start;
    if    (start < stop && *start == L'=') {
        ++start;
    } else {
        UpdateReached(start, pMatch);
        return NULL;
    }
    while (start < stop && *start <= L' ') ++start;

    const UChar* ret = m_nextNode ? m_nextNode->match(start, stop, pMatch) : start;
    UpdateReached(start, pMatch);
    pMatch->consumed = start;
    return ret;
}

bool CNode_Equal::mayMatch(bool* tab)
{
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

const UChar* CNode_Quote::match(const UChar* start, const UChar* stop, MatchData* pMatch) {

    if (start >= stop) 
		return NULL;

    m_matched = *start;
	++start;
    // Rule: "
    // Rule: '
    if (!((   m_matched == L'\'' && (   m_quote == L'\"'
                                  || m_openingQuote == nullptr
                                  || m_openingQuote->m_matched == '\'' ))
          ||  (m_matched == L'\"' && (  m_quote == L'\"'
                                  || (m_openingQuote
                                      && m_openingQuote->m_matched == L'\"'))) )) {
        return nullptr;
    }

    const UChar* ret = m_nextNode ? m_nextNode->match(start, stop, pMatch) : start;
	UpdateReached(start, pMatch);
    pMatch->consumed = start;
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

const UChar* CNode_Char::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
    // The test is case insensitive
    if (start>=stop || towlower(*start) != m_byte) 
		return nullptr;		// マッチしなかった

    start++;

    const UChar* ret = m_nextNode ? m_nextNode->match(start, stop, pMatch) : start;
    UpdateReached(start, pMatch);
    pMatch->consumed = start;
    return ret;
}

bool CNode_Char::mayMatch(bool* tab)
{
    if (tab) {
        tab[(unsigned char)m_byte] = true;
        tab[(unsigned char)towupper(m_byte)] = true;
    }
    return false;
}



/* class CNode_Range
 * Try and match a single character
 */

const UChar* CNode_Range::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
    // Check if there is a quote
    UChar quote = 0;
    if (start < stop && (*start == L'\'' || *start == L'\"')) {
        quote = *start++;
    }
    // Check if there is a minus sign
    int sign = 1;
    if (start < stop && *start == L'-') {
        sign = -1;
        start++;
    }
    // Check if there is a digit
    if (start >= stop || u_isdigit(*start) == false) {
		UpdateReached(start, pMatch);
        return nullptr;
    }
    // Read the number
    int num = 0;
    while (start < stop && u_isdigit(*start)) {
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
    if (quote || (m_allow ^ (m_min <= num && num <= m_max))) return nullptr;

    const UChar* ret = m_nextNode ? m_nextNode->match(start, stop, pMatch) : start;
	UpdateReached(start, pMatch);
    pMatch->consumed = start;
    return ret;
}

bool CNode_Range::mayMatch(bool* tab)
{
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
const UChar* CNode_String::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
    const UChar* ptr = m_str.c_str();
    const UChar* max = (stop < start + m_str.length()) ? stop : start + m_str.length();

    while (start < max && *ptr == towlower(*start)) { ptr++; start++; }

	// 全部消費されなかった
    if (ptr < m_str.c_str() + m_str.length()) {
		UpdateReached(start, pMatch);
        return nullptr;
    }

    const UChar* ret = m_nextNode ? m_nextNode->match(start, stop, pMatch) : start;
    UpdateReached(start, pMatch);
    pMatch->consumed = start;
    return ret;
}

bool CNode_String::mayMatch(bool* tab)
{
    if (tab) {
        tab[(unsigned char)m_str[0]] = true;
        tab[(unsigned char)towupper(m_str[0])] = true;
    }
    return false;
}


/* class CNode_Chars
 * Try and match a single character.
 */
CNode_Chars::CNode_Chars(const std::wstring& s, bool allow /*= true*/) : CNode(CHARS), m_allow(allow)
{
    // For a fast matching, all allowed (or forbidden) characters are
    // tagged in a table of booleans.
    for (unsigned int i=0; i<256; i++) m_byte[i] = !allow;
	for (UChar c : s)
		m_byte[(unsigned char)c] = allow;
}


const UChar* CNode_Chars::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
    if (start >= stop || m_byte[(unsigned char)(*start)] == false)
		return nullptr;

    start++;

    const UChar* ret = m_nextNode ? m_nextNode->match(start, stop, pMatch) : start;
    UpdateReached(start, pMatch);
    pMatch->consumed = start;
    return ret;
}

bool CNode_Chars::mayMatch(bool* tab)
{
    if (tab) {
        for (int i=0; i<256; i++) if (m_byte[i]) tab[i] = true;
    }
    return false;
}



/* class CNode_Empty
 * Empty pattern. Always matches (but only once) or never, depending on accept
 */
const UChar* CNode_Empty::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
    if (m_accept == false) 
		return nullptr;

    const UChar* ret = m_nextNode ? m_nextNode->match(start, stop, pMatch) : start;
    UpdateReached(start, pMatch);
    pMatch->consumed = start;
    return ret;
}

bool CNode_Empty::mayMatch(bool* tab)
{
    return m_accept && (m_nextNode == nullptr || m_nextNode->mayMatch(tab));
}



/* class CNode_Any
 * Matches any character. Corresponds to ?
 */
const UChar* CNode_Any::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
    // Rule: ?
    if (start >= stop) 
		return nullptr;

    start++;
    
    const UChar* ret = m_nextNode ? m_nextNode->match(start, stop, pMatch) : start;
    UpdateReached(start, pMatch);
    pMatch->consumed = start;
    return ret;
}

bool CNode_Any::mayMatch(bool* tab)
{
    if (tab) {
        for (int i=0; i<256; i++) tab[i] = true;
    }
    return false;
}



/* class CNode_Run
 * Try and match a seauence of nodes.
 */
CNode_Run::~CNode_Run() {
    CUtil::deleteVector<CNode>(*m_nodes);
    delete m_nodes;
}

const UChar* CNode_Run::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
    const UChar* ret = m_firstNode->match(start, stop, pMatch);
    //m_consumed = m_nodes->back()->m_consumed;
    return ret;
}

bool CNode_Run::mayMatch(bool* tab)
{
    return m_firstNode->mayMatch(tab);
}

void CNode_Run::setNextNode(CNode* node)
{
    m_nextNode = nullptr;
    for (auto it = m_nodes->rbegin(); it != m_nodes->rend(); ++it) {
        (*it)->setNextNode(node);
        node = *it;
    }
}



/* class CNode_Or
 * Try and match nodes one after another
 */
CNode_Or::~CNode_Or() {
    CUtil::deleteVector<CNode>(*m_nodes);
    delete m_nodes;
}

const UChar* CNode_Or::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
	for (CNode* node : *m_nodes) {
		const UChar* ret = node->match(start, stop, pMatch);
        if (ret) {
            //m_consumed = node->m_consumed;
            return ret;
        }
	}
    return nullptr;
}

bool CNode_Or::mayMatch(bool* tab)
{
    bool ret = false;
    for (unsigned int i=0; i< m_nodes->size(); i++)
        if ((*m_nodes)[i]->mayMatch(tab)) ret = true;
    return ret;
}

void CNode_Or::setNextNode(CNode* node)
{
    m_nextNode = nullptr;
    for (auto it = m_nodes->begin(); it != m_nodes->end(); it++) {
        (*it)->setNextNode(node);
    }
}



/* class CNode_And
 * Try and match left node then right node (returns max length of both).
 * 'force' is for "&&", limiting right to matching exactly what left matched
 */

CNode_And::~CNode_And()
{
    delete m_nodeL;
    delete m_nodeR;
}

const UChar* CNode_And::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
    // Ask left node for the first match
    const UChar* posL = m_nodeL->match(start, stop, pMatch);
    if (posL == nullptr) 
		return nullptr;

	const UChar* consumedL = pMatch->consumed;
    //m_consumed = m_nodeL->m_consumed;

    // Ask right node for the first match
    const UChar* posR = m_nodeR->match(start, (m_force ? consumedL : stop), pMatch);
    if (posR == nullptr || (m_force && posR != consumedL)) 
		return nullptr;

	const UChar* consumedR = pMatch->consumed;
    if (consumedL < consumedR) 
		pMatch->consumed = consumedR;
    return (posL > posR ? posL : posR);
}

bool CNode_And::mayMatch(bool* tab)
{
    bool tabL[256], tabR[256];
    for (int i=0; i<256; i++) tabL[i] = tabR[i] = false;

    bool retL = m_nodeL->mayMatch(tabL);
    bool retR = m_nodeR->mayMatch(tabR);
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

void CNode_And::setNextNode(CNode* node)
{
    m_nextNode = nullptr;
    m_nodeL->setNextNode(node);
    m_nodeR->setNextNode(m_force ? nullptr : node);
}




/* class CNode_Repeat
 * Try and match a pattern several times
 */
CNode_Repeat::~CNode_Repeat() 
{
    delete m_node;
}

const UChar* CNode_Repeat::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
    int rcount = 0;
    bool checked = false;
    while (rcount < m_rmax) {
        if (m_iterate && m_nextNode && rcount >= m_rmin) {
            const UChar* ret = m_nextNode->match(start, stop, pMatch);
            if (ret) {
                pMatch->consumed = start;
                return ret;
            }
            checked = true;
        }
        const UChar* ret = m_node->match(start, stop, pMatch);
        if (ret == nullptr) 
			break;

        checked = false;
        if (ret == start) {
			rcount = m_rmax;
			break;	// infinite-loop protection
		} 
        start = ret;
        rcount++;
    }
    if (rcount < m_rmin || checked) 
		return nullptr;

    const UChar* ret = m_nextNode ? m_nextNode->match(start, stop, pMatch) : start;
    pMatch->consumed = start;
    return ret;
}

bool CNode_Repeat::mayMatch(bool* tab)
{
    bool ret = m_node->mayMatch(tab);
    return ret && m_rmin > 0 ? false : (m_nextNode == nullptr || m_nextNode->mayMatch(tab));
}

void CNode_Repeat::setNextNode(CNode* node)
{
    m_nextNode = node;
    m_node->setNextNode(nullptr);
}



/* class CNode_Memory
 * Try and match something, and if it does, store the position with a CMemory
 */
CNode_Memory::CNode_Memory(CNode *node, int memoryPos) :
	CNode(MEMORY), m_node(node), m_memoryPos(memoryPos), m_memorizer(nullptr)
{
	if (m_node)
		m_memorizer = new CNode_Memory(nullptr, m_memoryPos);
}

CNode_Memory::~CNode_Memory()
{
    if (m_node) delete m_node;
    if (m_memorizer) delete m_memorizer;
}

const UChar* CNode_Memory::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
    if (m_memorizer) {
        //m_memorizer->m_recordPos = start;
		pMatch->vecRecordPos.push_back(start);
        const UChar* ret = m_node->match(start, stop, pMatch);
		pMatch->vecRecordPos.pop_back();
        //m_consumed = m_node->m_consumed;
        return ret;
    } else {
		ATLASSERT( pMatch->vecRecordPos.size() > 0 );

		CMemory backup;
        // Backup memory and replace by a new one, or push new one on stack
        if (m_memoryPos != -1) {
			backup = pMatch->pFilter->memoryTable[m_memoryPos];
			pMatch->pFilter->memoryTable[m_memoryPos](pMatch->vecRecordPos.back(), start);
        } else {
			pMatch->pFilter->memoryStack.push_back(CMemory(pMatch->vecRecordPos.back(), start));
        }
        // Ask next node
        const UChar* ret = m_nextNode ? m_nextNode->match(start, stop, pMatch) : start;
        if (ret) {
            pMatch->consumed = start;
            return ret;
        }
        // Undo backup.
        if (m_memoryPos != -1) {
            pMatch->pFilter->memoryTable[m_memoryPos] = backup;
        } else {
            pMatch->pFilter->memoryStack.pop_back();
        }
        return nullptr;
    }
}

bool CNode_Memory::mayMatch(bool* tab)
{
    if (m_memorizer) {
        return m_node->mayMatch(tab);
    } else {
        return m_nextNode == nullptr || m_nextNode->mayMatch(tab);
    }
}

void CNode_Memory::setNextNode(CNode* node)
{
    if (m_memorizer) {
        m_nextNode = nullptr;
        m_memorizer->setNextNode(node);
        m_node->setNextNode(m_memorizer);
    } else {
        m_nextNode = node;
    }
}



/* class CNode_Negate
 * Try and match a pattern. If it does, failure (NULL), else success (start)
 */
const UChar* CNode_Negate::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
    // The negation node does not consume text
    const UChar* test = m_node->match(start, stop, pMatch);
    if (test) 
		return nullptr;
    const UChar* ret = m_nextNode ? m_nextNode->match(start, stop, pMatch) : start;
    pMatch->consumed = start;
    return ret;
}

bool CNode_Negate::mayMatch(bool* tab)
{
    return m_nextNode == nullptr || m_nextNode->mayMatch(tab);
}

void CNode_Negate::setNextNode(CNode* node)
{
    m_nextNode = node;
    m_node->setNextNode(nullptr);
}




/* class CNode_AV
 * Try and match an html parameter
 */
const UChar* CNode_AV::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
    // find parameter limits
    bool consumeQuote = false;
    const UChar *begin, *end;
    begin = end = start;
    if (start < stop) {
        UChar token = *start;
        if (token == L'\'' || token == L'\"') {
            // We'll try and match a quoted parameter. Look for closing quote.
            end++;
            if (m_isAVQ == false) begin++; // AV: the matching will start after the quote
            while (end < stop && *end != token) end++;
            if (end < stop) {
                if (m_isAVQ) {
                    end++; // AVQ: the matching will include the closing quote
				} else {
                    // AV: if we match the interior, we will
                    // consume the closing quote
                    consumeQuote = true;
				}
            }
        } else {
            // Parameter without quote (single word), look for its end
            while (end < stop && *end > L' ' && *end != L'>') end++;
            if (end == begin) return NULL;
        }
    }
    
    // test parameter value
    begin = m_node->match(begin, end, pMatch);
    if (begin != end) 
		return nullptr;
    start = consumeQuote ? end + 1 : end;

    const UChar* ret = m_nextNode ? m_nextNode->match(start, stop, pMatch) : start;
	UpdateReached(start, pMatch);
    pMatch->consumed = start;
    return ret;
}

bool CNode_AV::mayMatch(bool* tab) {
    if (tab) {
        tab['\''] = true;
        tab['\"'] = true;
    }
    return m_node->mayMatch(tab);
}

void CNode_AV::setNextNode(CNode* node)
{
    m_nextNode = node;
    m_node->setNextNode(nullptr);
}


/* class CNode_Url
 * Try and match a part of URL
 */
const UChar* CNode_Url::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
	const CUrl& url = pMatch->pFilter->owner.url;
    const UChar* ptr = nullptr;
	std::wstring ptrBuffer;
    switch (m_token) {
        case L'u': ptrBuffer = UTF16fromUTF8(url.getUrl());     break;
        case L'h': ptrBuffer = UTF16fromUTF8(url.getHost());    break;
        case L'p': ptrBuffer = UTF16fromUTF8(url.getPath());    break;
        case L'q': ptrBuffer = UTF16fromUTF8(url.getQuery());   break;
        case L'a': ptrBuffer = UTF16fromUTF8(url.getAnchor());  break;
    }
	ptr = ptrBuffer.data();
    if (*ptr == L'\0') 
		return nullptr;
	
    while (start < stop && *ptr != '\0' && towlower(*ptr) == towlower(*start)) {
        ptr++; start++;
    }
	// 全部消費しなかった
    if (*ptr != L'\0') {
		UpdateReached(start, pMatch);
        return nullptr;
    }

    const UChar* ret = m_nextNode ? m_nextNode->match(start, stop, pMatch) : start;
	UpdateReached(start, pMatch);
    pMatch->consumed = start;
    return ret;
}

bool CNode_Url::mayMatch(bool* tab)
{
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
CNode_List::CNode_List(const std::string& name/*, CMatcher& matcher*/) :
            CNode(LIST), m_name(name)/*, matcher(matcher)*/
{
	std::lock_guard<std::recursive_mutex> lock(CSettings::s_mutexHashedLists);
	auto& hashedLists = CSettings::s_mapHashedLists[m_name];
	if (hashedLists == nullptr)	// リストがないので適当に生成しとく
		hashedLists.reset(new HashedListCollection);

	m_phashedCollection = hashedLists.get();	// ここで取得したポインタはプログラム終了まで有効
}

CNode_List::~CNode_List()
{
}

const UChar* CNode_List::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
	if (m_phashedCollection == nullptr)
		return nullptr;

    if (start < stop) {
		const UChar* startOrigin = start;
        // Check the hashed list corresponding to the first char
		boost::shared_lock<boost::shared_mutex>	lock(m_phashedCollection->mutex);
		// 固定プレフィックス
		auto& preHashWordList = m_phashedCollection->hashWordList;
		std::unordered_map<wchar_t, std::unique_ptr<HashedListCollection::PreHashWord>>* pmapPreHashWord = &preHashWordList.mapChildPreHashWord;
		while (start < stop) {
			auto itfound = pmapPreHashWord->find(towlower(*start));
			if (itfound == pmapPreHashWord->end())
				break;

			++start;
			for (auto& node : itfound->second->vecNode) {
				const UChar* ptr = node->match(start, stop, pMatch);
				if (ptr) {
					start = ptr;
					const UChar* ret = m_nextNode ? m_nextNode->match(start, stop, pMatch) : start;
					pMatch->consumed = start;
					return ret;
				}
			}
			pmapPreHashWord = &itfound->second->mapChildPreHashWord;
		}

		start = startOrigin;
		// NormalList
		for (auto& node : m_phashedCollection->deqNormalNode) {
			const UChar* ptr = node->match(start, stop, pMatch);
			if (ptr) {
				start = ptr;
				const UChar* ret = m_nextNode ? m_nextNode->match(start, stop, pMatch) : start;
				pMatch->consumed = start;
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
    return m_nextNode == nullptr || m_nextNode->mayMatch(NULL);
}




/* class CNode_Command
 * Executes a command that does not consume anything.
 */
CNode_Command::CNode_Command(CMD_ID num, const std::wstring& name, const std::wstring& content) :
        CNode(COMMAND), m_num(num), m_name(name), m_content(content), m_matcher(nullptr)
		//, filter(filter), owner(filter.owner)
{
    // For some commands we'll build a CMatcher (content is a pattern)
    if (num >= CMD_TSTSHARP) {
        m_matcher = new CMatcher(content);
    }
}

CNode_Command::~CNode_Command()
{
    if (m_matcher) delete m_matcher;
}

const UChar* CNode_Command::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
    const UChar *tStart = nullptr;
	const UChar *tStop = nullptr;
	const UChar *tEnd = nullptr;
	std::wstring toMatch;

	CFilter& filter = *pMatch->pFilter;
	CFilterOwner& owner = filter.owner;

    switch (m_num) {

    case CMD_TSTSHARP:
        if (filter.memoryStack.empty()) return NULL;
        toMatch = filter.memoryStack.back().getValue();
        tStart = toMatch.c_str();
        tStop = tStart + toMatch.size();
        if (toMatch.empty()
               || !m_matcher->match(tStart, tStop, tEnd, pMatch)
               || tEnd != tStop ) {
            return NULL;
        }
        break;

    case CMD_TSTEXPAND:
        toMatch = CExpander::expand(m_name, filter);
        tStart = toMatch.c_str();
        tStop = tStart + toMatch.size();
        if (toMatch.empty()
               || !m_matcher->match(tStart, tStop, tEnd, pMatch)
               || tEnd != tStop ) {
            return NULL;
        }
        break;

    case CMD_TSTDIGIT:
        toMatch = filter.memoryTable[m_name[0] - L'0'].getValue();
        tStart = toMatch.c_str();
        tStop = tStart + toMatch.size();
        if (toMatch.empty()
               || !m_matcher->match(tStart, tStop, tEnd, pMatch)
               || tEnd != tStop ) {
            return NULL;
        }
        break;

    case CMD_TSTVAR:
        toMatch = owner.variables[m_name];
        tStart = toMatch.c_str();
        tStop = tStart + toMatch.size();
        if (toMatch.empty()
               || !m_matcher->match(tStart, tStop, tEnd, pMatch)
               || tEnd != tStop ) {
            return NULL;
        }
        break;

    case CMD_URL:
        toMatch = UTF16fromUTF8(owner.url.getUrl());
        tStart = toMatch.c_str();
        tStop = tStart + toMatch.size();
        if (!m_matcher->match(tStart, tStop, tEnd, pMatch)) return NULL;
        break;

    case CMD_IHDR:
        toMatch = UTF16fromUTF8(CFilterOwner::GetHeader(owner.inHeaders, UTF8fromUTF16(m_name)));
        tStart = toMatch.c_str();
        tStop = tStart + toMatch.size();
        if (!m_matcher->match(tStart, tStop, tEnd, pMatch)) return NULL;
        break;

    case CMD_OHDR:
        toMatch = UTF16fromUTF8(CFilterOwner::GetHeader(owner.outHeaders, UTF8fromUTF16(m_name)));
        tStart = toMatch.c_str();
        tStop = tStart + toMatch.size();
        if (!m_matcher->match(tStart, tStop, tEnd, pMatch)) return NULL;
        break;

    case CMD_RESP:
        toMatch = UTF16fromUTF8(owner.responseCode);
        tStart = toMatch.c_str();
        tStop = tStart + toMatch.size();
        if (!m_matcher->match(tStart, tStop, tEnd, pMatch)) return NULL;
        break;

    case CMD_SETSHARP:
        filter.memoryStack.push_back(CMemory(m_content));
        break;

    case CMD_SETDIGIT:
        filter.memoryTable[m_name[0] - L'0'] = CMemory(m_content);
        break;

    case CMD_SETVAR:
        owner.variables[m_name] = CExpander::expand(m_content, filter);
        break;

    case CMD_KEYCHK:
        if (!CUtil::keyCheck(m_content)) return NULL;
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
            //string title = APP_NAME;
            //string value = m_content;
            //size_t comma = CUtil::findUnescaped(value, ',');
            //if (comma != string::npos) {
            //    title = value.substr(0, comma);
            //    value = value.substr(comma + 1);
            //}
            //title = CExpander::expand(title, filter);
            //value = CExpander::expand(value, filter);
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
        if (UTF8fromUTF16(m_content) != owner.fileType) return NULL;
        break;

    case CMD_STOP:
        filter.bypassed = true;
        break;

    case CMD_USEPROXY:
        owner.useSettingsProxy = (m_content[0] == L't');
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
            std::wstring log = CExpander::expand(m_content, filter);
			CLog::FilterEvent(kLogFilterLogCommand, owner.requestNumber, UTF8fromUTF16(filter.title), UTF8fromUTF16(log));
        }
        break;

    case CMD_JUMP:
        owner.rdirToHost = UTF8fromUTF16(CExpander::expand(m_content, filter));
        CUtil::trim(owner.rdirToHost);
        owner.rdirMode = 0;
		CLog::FilterEvent(kLogFilterJump, owner.requestNumber, UTF8fromUTF16(filter.title), owner.rdirToHost);
        break;

    case CMD_RDIR:
        owner.rdirToHost = UTF8fromUTF16(CExpander::expand(m_content, filter));
        CUtil::trim(owner.rdirToHost);
        owner.rdirMode = 1;
		CLog::FilterEvent(kLogFilterRdir, owner.requestNumber, UTF8fromUTF16(filter.title), owner.rdirToHost);
        break;

    case CMD_FILTER:
        owner.bypassBody = (m_content[0] != L't');
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

    const UChar* ret = m_nextNode ? m_nextNode->match(start, stop, pMatch) : start;
    pMatch->consumed = start;
    return ret;
}

bool CNode_Command::mayMatch(bool* tab)
{
    return m_nextNode == nullptr || m_nextNode->mayMatch(tab);
}


/* class CNode_Cnx
 * Matches depending on connection number.
 */
const UChar* CNode_Cnx::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
    if ((((m_num - 1) / m_z) % m_y) != m_x - 1)
		return nullptr;

    const UChar* ret = m_nextNode ? m_nextNode->match(start, stop, pMatch) : start;
    pMatch->consumed = start;
    return ret;
}

bool CNode_Cnx::mayMatch(bool* tab)
{
    return m_nextNode == nullptr || m_nextNode->mayMatch(tab);
}



/* class CNode_Nest
 * Matches nested tags, with optional content.
 */
CNode_Nest::CNode_Nest(CNode* left, CNode* middle, CNode* right, bool inest) : 
	CNode(NEST), m_left(left), m_middle(middle), m_right(right), m_inest(inest)
{
    for (int i=0; i<256; i++) m_tabL[i] = false;
    if (m_left->mayMatch(m_tabL)) 
		for (int i=0; i<256; i++) m_tabL[i] = true;

    for (int i=0; i<256; i++) m_tabR[i] = false;
    if (m_right->mayMatch(m_tabR)) 
		for (int i=0; i<256; i++) m_tabR[i] = true;
}

CNode_Nest::~CNode_Nest() {
    if (m_left)   delete m_left;
    if (m_middle) delete m_middle;
    if (m_right)  delete m_right;
}

const UChar* CNode_Nest::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
    int level;
    const UChar *endL = start, *startR, *pos;
    if (m_inest == false) {
        if (m_tabL[(unsigned char)(*start)] == false)
			return nullptr;
        endL = m_left->match(start, stop, pMatch);
        if (!endL) return NULL;
    }
    startR = pos = endL;
    level = 1;
    while (pos < stop && level > 0) {
        if (m_tabR[(unsigned char)(*pos)]) {
            const UChar* end = m_right->match(pos, stop, pMatch);
            if (end > pos) {
                level--;
                startR = pos;
                pos = end;
                continue;
            }
        }
        if (m_tabL[(unsigned char)(*pos)]) {
            const UChar* end = m_left->match(pos, stop, pMatch);
            if (end > pos) {
                level++;
                pos = end;
                continue;
            }
        }
        pos++;
    }
    if (level > 0) {
		UpdateReached(pos, pMatch);	// 消費したので更新しとかないとまずい？
		pMatch->consumed = pos;
		return NULL;	// Nestが収束しないのでマッチしない
	}

    if (m_middle) {
        const UChar* end = m_middle->match(endL, startR, pMatch);
        if (end != startR) return NULL;
    }
    start = m_inest ? startR : pos;
    
    const UChar* ret = m_nextNode ? m_nextNode->match(start, stop, pMatch) : start;
    pMatch->consumed = start;
    return ret;
}

bool CNode_Nest::mayMatch(bool* tab)
{
    bool ret = true;
    if (ret && !m_inest)   ret = m_left->mayMatch(tab);
    if (ret && m_middle)   ret = m_middle->mayMatch(tab);
    if (ret && !m_inest)   ret = m_right->mayMatch(tab);
    if (ret && m_nextNode) ret = m_nextNode->mayMatch(tab);
    return ret;
}

void CNode_Nest::setNextNode(CNode* node)
{
    m_nextNode = node;
    m_left->setNextNode(NULL);
    m_right->setNextNode(NULL);
    if (m_middle) m_middle->setNextNode(NULL);
}


/* class CNode_Test
 * Try and match a string of characters.
 */
const UChar* CNode_Test::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
	CFilter& filter = *pMatch->pFilter;
    std::wstring str;     // variable content, that will be compared to text
    if (m_name == L"#") {
        if (!filter.memoryStack.empty())
            str = filter.memoryStack.back().getValue();
	} else if (m_name.size() == 1 && iswdigit(m_name[0])) {
        str = filter.memoryTable[m_name[0] - L'0'].getValue();
    } else if (m_name[0] == L'(' && m_name[m_name.size()-1] == L')') {
        str = CExpander::expand(m_name.substr(1, m_name.size()-2), filter);
    } else {
        str = filter.owner.variables[m_name];
    }
    int size = str.size();
    if (!size) return NULL;

    const UChar* ptr = str.c_str();
    const UChar* max = (stop < start+size ? stop : start+size);
    while (start < max && *ptr == towlower(*start)) { ptr++; start++; }
    if (ptr < str.c_str() + size) {
		UpdateReached(start, pMatch);
        return NULL;
    }

    const UChar* ret = m_nextNode ? m_nextNode->match(start, stop, pMatch) : start;
    UpdateReached(start, pMatch);
    pMatch->consumed = start;
    return ret;
}

bool CNode_Test::mayMatch(bool* tab)
{
    // We don't know at construction time what the string to match is.
    // So we must allow everything.
    if (tab) {
        for (int i=0; i<256; i++) tab[i] = true;
    }
    return m_nextNode == nullptr || m_nextNode->mayMatch(NULL);
}




/* class CNode_Ask
 * Automates the insertion of an item in a list based on user choice.
 */
CNode_Ask::CNode_Ask(/*CFilter& filter,*/
        std::wstring allowName, std::wstring denyName, std::wstring question, std::wstring item,
        std::wstring pattern) : CNode(ASK), //filter(filter),
        allowName(allowName), denyName(denyName), question(question),
        item(item), pattern(pattern)
{

    m_allowMatcher.reset(new CMatcher(L"$LST(" + allowName + L")"));
    try {
        m_denyMatcher.reset(new CMatcher(L"$LST(" + denyName + L")"));
    } catch (parsing_exception e) {
        throw e;
    }
}

CNode_Ask::~CNode_Ask()
{
}

const UChar* CNode_Ask::match(const UChar* start, const UChar* stop, MatchData* pMatch)
{
	CFilter& filter = *pMatch->pFilter;
    // We lock so that 2 documents doing the same test at the same time
    // won't ask the question twice. No need unlocking afterwards.
    if (!filter.locked) {
        //CLog::ref().filterLock.Lock();
        filter.locked = true;
    }
    const UChar *tStart, *tStop, *tEnd;
    std::wstring toMatch = CExpander::expand(pattern, filter);
    tStart = toMatch.c_str();
    tStop = tStart + toMatch.size();
    // If the pattern is found in Allow list, we return a non-match
    // (so that the filter does not operate)
    if (m_allowMatcher->match(tStart, tStop, tEnd, pMatch)) return NULL;
    // On the contrary, if found in Deny list, we return a match
    // (to continue filtering)
    if (!m_denyMatcher->match(tStart, tStop, tEnd, pMatch)) {
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

    const UChar* ret = m_nextNode ? m_nextNode->match(start, stop, pMatch) : start;
    pMatch->consumed = start;
    return ret;
}

bool CNode_Ask::mayMatch(bool* tab) {
    return m_nextNode == nullptr || m_nextNode->mayMatch(NULL);
}

}	// namespace Proxydomo

