/**
*	@file	Matcher.cpp
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

#include "Matcher.h"
#include "Nodes.h"
#include "proximodo\util.h"
#include "proximodo\const.h"	// for BIGNUMBER
#include <unicode\uchar.h>
#include "CodeConvert.h"
#include "Settings.h"

using namespace CodeConvert;

namespace Proxydomo {

/* Searches a comma outside parentheses, within a command
 */
size_t findParamEnd(const std::wstring& str, UChar c) {
    int level = 0;
    size_t size = str.size();
    for (size_t pos = 0; pos < size; pos++) {
        switch (str[pos]) {
            case L'\\': pos++;   break;
            case L'(': level++; break;
            case L')': level--; break;
            case L',': if (level<1) return pos; break;
        }
    }
    return string::npos;
}

/* Decodes a number in the pattern
 */
int readNumber(StringCharacterIterator& patternIt, int &num) {

    int sign = 1;
    // There can be a minus sign
    if (patternIt.current() != patternIt.DONE && patternIt.current() == L'-') {
        sign = -1;
        patternIt.next();
    }
    num = 0;
    // At least one digit is needed
	if (patternIt.current() == patternIt.DONE || iswdigit(patternIt.current()) == false )
		throw parsing_exception("NOT_A_NUMBER", patternIt.getIndex());

    // Read all digits available
	for (UChar code = patternIt.current(); code != patternIt.DONE && iswdigit(code); code = patternIt.next()) {
        num = num*10 + code - L'0';
    }
    // Return the corresponding number
    num *= sign;
    return num;
}


/* Decodes a range, in the form of spaces + (number or *) + spaces
 * possibly followed by the separation symbol + spaces + (number or *) + spaces
 */
void readMinMax(StringCharacterIterator& patternIt, UChar sep, int& min, int& max)
{
    // consume spaces
	while (patternIt.current() != patternIt.DONE && 
		(patternIt.current() == L' ' || patternIt.current() == L'\t')) patternIt.next();
    
    // read lower limit
	if (patternIt.current() != patternIt.DONE && patternIt.current() == '*') {
		patternIt.next();
        min = -BIG_NUMBER;
        max = BIG_NUMBER;   // in case there is no upper limit
    } else if (patternIt.current() != patternIt.DONE && patternIt.current() == sep) {
        min = -BIG_NUMBER;
        max = BIG_NUMBER;   // in case there is no upper limit
    } else {
        max = min = 0;
        readNumber(patternIt, min);
        max = min;          // in case there is no upper limit
    }
    
    // consume spaces
	while (patternIt.current() != patternIt.DONE && 
		(patternIt.current() == L' ' || patternIt.current() == L'\t')) patternIt.next();

    // check if there is an upper limit
    if (patternIt.current() != patternIt.DONE && 
		(patternIt.current() == sep || patternIt.current() == L'-')) {
        patternIt.next();

        // consume spaces
		while (patternIt.current() != patternIt.DONE && 
			(patternIt.current() == L' ' || patternIt.current() == L'\t')) patternIt.next();

        // read upper limit
        if (patternIt.current() != patternIt.DONE && patternIt.current() == L'*') {
		  patternIt.next();
            max = BIG_NUMBER;
		} else if (patternIt.current() != patternIt.DONE && patternIt.current() != L'-' && 
			iswdigit(patternIt.current()) == false ) {
            max = BIG_NUMBER;
        } else {
            readNumber(patternIt, max);
            if (max < min) { int i = max; max = min; min = i; }
        }

        // consume spaces
		while (patternIt.current() != patternIt.DONE && 
			(patternIt.current() == L' ' || patternIt.current() == L'\t')) patternIt.next();
    }
}

/* Decodes a command name and content starting at position pos
 * (which should be pointing to the first letter of the command name).
 * Returns -1 if the function name is not known or () are not
 * corresponding. Else returns the position after the ).
 * command and content will contain the name and content.
 */
/// $command(content)
int decodeCommand(StringCharacterIterator& patternIt,
							std::wstring& command, std::wstring& content)
{
	int index = patternIt.getIndex();
    // Read command name
	while (patternIt.current() != patternIt.DONE && iswupper(patternIt.current())) {
			command += patternIt.nextPostInc();
	}
    // It should exist and be followed by (
	if (index == patternIt.getIndex() || patternIt.hasNext() == false || patternIt.current() != L'(')
		return -1;

    // Search for the closing )
    UChar prevCode = 0;
    int level = 1;  // Level of parentheses
	for (UChar code = patternIt.next(); code != patternIt.DONE && level > 0; prevCode = code, code = patternIt.next()) {
        // escaped () don't count
        if (code == L'(' && prevCode != L'\\') {
            level++;
        } else if (code == L')' && prevCode != L'\\') {
            level--;
        }
		content += code;
    }
    // Did we reach end of string before closing all parentheses?
	// '(' ')'が閉じていない
    if (level > 0) 
		return -1;
	content.erase(content.end() - 1);
	return patternIt.getIndex();
}



///////////////////////////////////////////////////////////////////
// CMatcher

/* Constructor
 * It stores the requested url and transforms the pattern into
 * a search tree. An exception is returned if the pattern is
 * malformed.
 */
/// pattern を検索木に変換します。 patternが無効なら例外が飛びます。
CMatcher::CMatcher(const std::wstring& pattern)
{
	UnicodeString pat(pattern.c_str(), pattern.length());
	_CreatePattern(pat);
}

void	CMatcher::_CreatePattern(const UnicodeString& pattern)
{
	StringCharacterIterator patternIt(pattern);

	if (pattern.length() > 0 && patternIt.current() == L'^') {
        // special case: inverted pattern
		patternIt.next();
		m_root.reset(new CNode_Negate(expr(patternIt)));
    } else {
        // (this call may throw an exception)
        m_root.reset(expr(patternIt)); 
    }
    m_root->setNextNode();

    // another reason to throw : the pattern was not completely
    // consumed, i.e there was an unpaired )
	/// パターンが完全に消費されていない。※例: ()がペアになっていない
	if (patternIt.hasNext()) {
		throw parsing_exception("PARSING_INCOMPLETE", patternIt.getIndex());
    }
}

/* Destructor
 */
CMatcher::~CMatcher()
{
}


std::shared_ptr<CMatcher>	CMatcher::CreateMatcher(const std::wstring& pattern)
{
	try {
		return std::shared_ptr<CMatcher>(new CMatcher(pattern));
	} catch (parsing_exception& e) {
		//ATLTRACE("CreateMatcher parsererro pattern(%s) : err %s [%d]", pattern.c_str(), e.message.c_str(), e.position);
	} catch (...) {

	}
	return nullptr;
}

std::shared_ptr<CMatcher>	CMatcher::CreateMatcher(const std::wstring& pattern, std::string& errmsg)
{
	try {
		return std::shared_ptr<CMatcher>(new CMatcher(pattern));
	} catch (parsing_exception& e) {
		errmsg = e.message;
		char strpos[24];
		::_itoa_s(e.position, strpos, 10);
		errmsg += "\nposition: ";
		errmsg += strpos;
		errmsg += "\n";
	}
	return nullptr;
}


/* Invokes the root node of the tree, to try and match the text.
 * start: position in html from which to scan
 * stop:  position in html where to stop scanning
 * end:   end position of the match
 * reached: farthest character scanned (e.g =stop for pattern "*")
 * Returns the end position (end) of the match, if text[start,end] matches
 * the pattern. Otherwise returns -1
 * Memory stack and map will also contain strings matched by \0-9 or \#
 *
 * Note: The filter using match() MUST unlock the mutex if it is still
 * locked. It cannot be done from inside CMatcher (nor CExpander) because
 * of possible recursive calls.
 */
bool CMatcher::match(const UChar* start, const UChar* stop, const UChar*& end, MatchData* pMatch)
{
    if (m_root == nullptr)
		return false;
    //this->reached = start;
	pMatch->reached = start;
    end = m_root->match(start, stop, pMatch);
    return end != nullptr;
}

/// simple
bool CMatcher::match(const std::string& text, CFilter* filter)
{
	UnicodeString str(text.c_str(), text.length());
	MatchData	mdata(filter);
	const UChar* end = nullptr;
	return match(str.getBuffer(), str.getBuffer() + str.length(), end, &mdata);
}


/* This static version of the matching function builds a search tree
 * at each call. It is mainly for use in replacement functions, that
 * are not supposed to be called too often (only after a match has
 * been found with an instantiated CMatcher).
 */
bool CMatcher::match(const std::wstring& pattern, CFilter& filter, 
					 const UChar* start, const UChar* stop, const UChar*& end, const UChar*& reached) {
    try {
        CMatcher matcher(pattern);
		MatchData matchData(&filter);
		bool match = matcher.match(start, stop, end, &matchData);
		reached = matchData.reached;
        return match;
    } catch (parsing_exception e) {
        return false;
    }
}

/* mayMatch() creates and returns a table of expected characters
 * at the beginning of the text to match. It can be called after
 * the construction of the CMatcher, to provide the search engine
 * with a fast way to know if a call to CMatcher::match() is of any
 * use. If !tab[text[start]] the engine knows there cannot be any
 * match at start, _even_ if the buffer is smaller than the window
 * size.
 */
void CMatcher::mayMatch(bool* tab)
{
    for (int i=0; i<256; i++) tab[i] = false;
    if (m_root->mayMatch(tab))
        // The whole pattern can match without consuming anything,
        // we'll have to try it systematically.
        for (int i=0; i<256; i++) tab[i] = true;
}


/* Tells if pattern is * or \0-9
 */
bool CMatcher::isStar() 
{
    return (m_root && (m_root->m_id == CNode::STAR || m_root->m_id == CNode::MEMSTAR));
}


/* Builds a tree node for the expression beginning at position pos.
 * We split and interpret the pattern where there is && (level 0)
 * & (level 1) or | (level 2). This allows operator precedence:
 * "|" > "&" > "&&"
 */
CNode* CMatcher::expr(StringCharacterIterator& patternIt, int level)
{

    if (level == 2) {

        CNode *node = run(patternIt);

        if (patternIt.current() != patternIt.DONE && patternIt.current() == L'|') {
            // Rule: |
            // We need an alternative of several runs. We will place them
            // all in a vector, then build a CNode_Or with the vector content.
            std::vector<CNode*> *vect = new std::vector<CNode*>();
            vect->push_back(node);
            
            // We continue getting runs as long as there are |
            while (patternIt.current() != patternIt.DONE && patternIt.current() == L'|') {
                patternIt.next();
                try {
                    node = run(patternIt);
                } catch (parsing_exception e) {
                    // Clean before throwing exception
                    CUtil::deleteVector<CNode>(*vect);
                    delete vect;
                    throw e;
                }
                vect->push_back(node);
            }
            
            // Build the CNode_Or will all the alternatives.
            node = new CNode_Or(vect);
        }
        return node;
    }

    CNode *node = expr(patternIt, level + 1);
    
    while (patternIt.current() != patternIt.DONE && patternIt.current() == L'&') {
		UChar code = patternIt.next();
		if (  (level == 0 && patternIt.getIndex() + 1 < patternIt.endIndex() && code == L'&') ||
              (level == 1 && (patternIt.getIndex() + 1 == patternIt.endIndex() || code != '&')) ) {

			// Rules: & &&
			// & is a binary operator, so we read the following run and
			// make a CNode_And with left and right runs.
			if (level == 0)
				patternIt.next();
			try {
				CNode *node2 = expr(patternIt, level + 1); // &'s right run
				// Build the CNode_And with left and right runs.
				node = new CNode_And(node, node2, (level == 0));
			} catch (parsing_exception e) {
				delete node;
				throw e;
			}
		} else {
			patternIt.previous();
			break;
		}
    }
    // Node contains the tree for all that we decoded from old pos to new pos
    return node;
}


/* Builds a node for the run beginning at position pos
 * A 'run' is a list of codes (escaped codes, characters,
 * expressions in (), ...) that should be matched one after another.
 */
CNode* CMatcher::run(StringCharacterIterator& patternIt)
{
    CNode* node = code(patternIt);
    std::vector<CNode*>* v = new std::vector<CNode*>();     // the list of codes

    while (node) {
        // Rule: +
        // If the code is followed by +, we will embed the code in a
        // repetition node (Node_Repeat). But first we check what follows
        // to see if parameters should be provided to the CNode_Repeat.
		if (patternIt.current() != patternIt.DONE && patternIt.current() == L'+') {
            patternIt.next();
            // Rule: ++
            // is it a ++?
            bool iter = false;
            int rmin = 0, rmax = BIG_NUMBER;
            if (patternIt.current() != patternIt.DONE && patternIt.current() == L'+') {
                patternIt.next();
                iter = true;
            }
            // Rule: {}
            // Is a range provided?
            if (patternIt.current() != patternIt.DONE && patternIt.current() == L'{') {
                patternIt.next();
                try {
                    // Read the values
                    readMinMax(patternIt, L',', rmin, rmax);
                    if (rmin < 0) rmin = 0;
                    if (rmax < rmin) rmax = rmin;

                    // check that the curly braces are closed
                    if (patternIt.current() != patternIt.DONE && patternIt.current() == L'}') {
                        patternIt.next();
                    } else {
                        throw parsing_exception("MISSING_CURLY", patternIt.getIndex());
                    }
                } catch (parsing_exception e) {
                    CUtil::deleteVector<CNode>(*v);
                    delete v;
                    delete node;
                    throw e;
                }
            }
            // Embed node in a repetition node
            node = new CNode_Repeat(node, rmin, rmax, iter);
        }

        // Let's do now an obvious optimization: consecutive
        // chars will not be matched one by one (too many function
        // calls) but as single string.
        if (node->m_id == CNode::CHAR && !v->empty()) {
            CNode_Char *nodeChar = (CNode_Char*)node;
            UChar newChar = nodeChar->getChar();

            CNode *prevNode = v->back();
            if (prevNode->m_id == CNode::CHAR) {
                CNode_Char *prevNodeChar = (CNode_Char*)prevNode;

                // consecutive CNode_Char's become one CNode_String
                std::wstring str;
                str  = prevNodeChar->getChar();
                str += newChar;
                delete prevNode;
                v->pop_back();
                delete node;
                node = new CNode_String(str);

            } else if (prevNode->m_id == CNode::STRING) {
                CNode_String *prevNodeString = (CNode_String *)prevNode;

                // append CNode_Char to previous CNode_String
                prevNodeString->append(newChar);
                v->pop_back();
                delete node;
                node = prevNodeString;
            }
        }

        v->push_back(node);
        try {
            node = code(patternIt);
        } catch (parsing_exception e) {
            CUtil::deleteVector<CNode>(*v);
            delete v;
            throw;
        }
    }
    // (end of the run)
    
    if (v->size() == 0) {

        // Empty run, just record that as an empty node
        delete v;
        return new CNode_Empty();
        
    } else if (v->size() == 1) {

        // Only 1 code, we don't need to embed it in a CNode_Run
        node = (*v)[0];
        delete v;
        return node;
        
    } else {

        // Now scan the whole list of codes to associate quotes by pairs.
        // We don't care about the quotes being ' or ". We just have to
        // associate first with second, third with fourth, and so on.
        // The association is materialized by second (resp. fourth) being
        // given a reference to first (resp. third).
        CNode_Quote* quote = NULL; // this points to the previous unpaired quote
        for (auto it = v->begin(); it != v->end(); ++it) {
            if ((*it)->m_id == CNode::QUOTE) {
                if (quote) {
                    ((CNode_Quote*)(*it))->setOpeningQuote(quote);
                    quote = NULL;
                } else {
                    quote = (CNode_Quote*)(*it);
                }
            }
        }
        
        // Returns a CNode_Run embedding the vector of Nodes
        return new CNode_Run(v);
    }
}


/* Builds a node for a single code beginning at position pos. A code can be:
 * - an escaped code \x
 * - an alternative of characters [ ] or [^ ]
 * - a numerical range [# ] or [^# ]
 * - a command $NAME( )
 * - an expression ( )
 * - an expression to memorize ( )\n
 * - a single character x
 */
CNode* CMatcher::code(StringCharacterIterator& patternIt)
{
    // CR and LF are only for user convenience, they have no meaning whatsoever
	// CR や LF はユーザーにとって便利なだけで、構文的には何の意味も持たないので飛ばす
	while ((patternIt.current() != patternIt.DONE) && 
		(patternIt.current() == L'\r' || patternIt.current() == L'\n')) patternIt.next();

    // This NULL will be interpreted by run() as end of expression
    if (patternIt.current() == patternIt.DONE) 
		return nullptr;

    // We look at the first caracter
    UChar token = patternIt.current();
    
    // Escape code
    if (token == L'\\') {

		token = patternIt.next();
        // it should not be at the end of the pattern
		// '\'でパターンが終わっていればおかしいので例外を飛ばす
		if (token == patternIt.DONE) 
			throw parsing_exception("ESCAPE_AT_END", patternIt.getIndex());

        patternIt.next();

        switch (token) {
            case L't':
                // Rule: \t
                return new CNode_Char(L'\t');
            case L'r':
                // Rule: \r
                return new CNode_Char(L'\r');
            case L'n':
                // Rule: \n
                return new CNode_Char(L'\n');
            case L's':
                // Rule: \s
                return new CNode_Repeat(new CNode_Chars(L" \t\r\n"), 1, BIG_NUMBER);
            case L'w':
                // Rule: \w
                return new CNode_Repeat(new CNode_Chars(L" \t\r\n>", false), 0, BIG_NUMBER, true);
            case L'#':
                // Rule: \#
                return new CNode_MemStar();
            case L'k':
                // Rule: \k
                return new CNode_Command(CMD_KILL, L"", L"");
			case L'd':
				// Rule: \d
				return new CNode_Repeat(new CNode_Chars(L"0123456789"), 1, BIG_NUMBER);

            default:
                // Rule: \0-9
				if (iswdigit(token))
                    return new CNode_MemStar(token - L'0');

                // Rules: \u \h \p \a \q
                if (std::wstring(L"uhpaq").find(token, 0) != string::npos)
                    return new CNode_Url(token);

                // Rule: backslash
                return new CNode_Char(token);
        }
        
    // Range or list of characters
    } else if (token == L'[') {

        patternIt.next();
        // Rule: [^
        // If the [] is negated by ^, keep that information
        bool allow = true;
		if (patternIt.current() != patternIt.DONE && patternIt.current() == L'^') {
            allow = false;
            patternIt.next();
        }
        
        // Range
        if (patternIt.current() != patternIt.DONE && patternIt.current() == L'#') {

            patternIt.next();
            // read the range. pos is updated to the closing ]
            int rmin, rmax;
            readMinMax(patternIt, L':', rmin, rmax);
			if (patternIt.hasNext() == false || patternIt.current() != L']')
				throw parsing_exception("MISSING_CROCHET", patternIt.getIndex());
            patternIt.next();
            // Rule: \n
            return new CNode_Range(rmin, rmax, allow);

        // Alternative of characters
        } else {

            // Rule: []
            // We create a clear CNode_Chars then provide it with
            // characters one by one.
            CNode_Chars *node = new CNode_Chars(L"", allow);
            
            UChar prev = 0;
            bool prevCase = false, codeCase, inRange = false;
			for (UChar code = patternIt.current(); code != patternIt.DONE && code != L']'; code = patternIt.next()) {
				
				UChar c1 = patternIt.next();
				UChar c2 = patternIt.next();
				if (c1 != patternIt.DONE) {
					patternIt.previous();
					patternIt.previous();
				}

				if (code == L'%' && patternIt.getIndex() + 2 < patternIt.endIndex()
                        && CUtil::hexa(c1)
                        && CUtil::hexa(c2) ) {

                    // %xx notation (case-sensitive)
					UChar c = towupper(c1);
                    code = (c <= L'9' ? c - L'0' : c - L'A' + 10) * 16;
                    c = towupper(c2);
                    code += (c <= L'9' ? c - L'0' : c - L'A' + 10);
                    codeCase = true;
                    
                } else if (code == L'\\' && c1 != patternIt.DONE) {

                    // escaped characters must be decoded (case-sensitive)
                    code = patternIt.next();
					c1 = patternIt.next();
					c2 = patternIt.next();
					patternIt.previous();
					patternIt.previous();

                    switch (code) {
                        case L't' : code = L'\t'; break;
                        case L'r' : code = L'\r'; break;
                        case L'n' : code = L'\n'; break;
                        // Only those three are not to be taken as is
                    }
                    codeCase = true;

                } else {
                    // normal character
                    codeCase = false;
                }
                
                if (inRange) {

                    // We decoded both ends of a range
                    if (prevCase || codeCase) {
                        // Add characters as is
                        for (; prev <= code; prev++) {
                            node->add(prev);
                        }
                    } else {
                        // Add both uppercase and lowercase characters
                        prev = towlower(prev);
                        wint_t cmax = towlower(code);
                        for (; prev <= cmax; prev++) {
                            node->add(prev);
                            node->add(towupper(prev));
                        }
                    }
                    inRange = false;
                    
                } else if (patternIt.getIndex() + 2 < patternIt.endIndex()
                        && c1 == L'-'
                        && c2 != L']') {

                    // We just decoded the start of a range, record it but don't add it
                    inRange = true;
                    prev = code;
                    prevCase = codeCase;
					patternIt.next();
                    
                } else {

                    // The decoded character is not part of a range
                    if (codeCase) {
                        node->add(code);
                    } else {
                        node->add(towlower(code));
                        node->add(towupper(code));
                    }
                }
            }

            // Check if the [] is closed
			// ']'が来る前にパターンが終わったので例外を飛ばす
			if (patternIt.getIndex() == patternIt.endIndex()) {
                delete node;
                throw parsing_exception("[]が閉じていません。", patternIt.getIndex());
            }
			patternIt.next();
            return node;
        }
        
    // Command
    } else if (token == L'$') {

        // Rule: $COMMAND(,,)
        // Decode command name and content
        // Note: unknown commands are ignored (just as a $DONOTHING(someignoredtext))
		patternIt.next();
		std::wstring command, content;
        int endContent = decodeCommand(patternIt, command, content); // after )
        int contentSize = content.size();
        int startContent = endContent - 1 - contentSize; // for exception info
        
        if (endContent < 0) {
            // Do as if it was not a command, the token is a normal char
            return new CNode_Char(L'$');
        } else {
			//patternIt.previous();
		}

        // The pattern will continue after the closing )
        //pos = endContent;
		UnicodeString	contentPattern(content.c_str(), content.length());
		StringCharacterIterator	contentIt(contentPattern);

        try {
            if (command == L"AV") {

                // Command "match a tag parameter"
                // Transform content of () in a tree and embed it in
                // a fast quote searcher
                return new CNode_AV(expr(contentIt), false);

            } else if (command == L"AVQ") {

                // Command "match a tag parameter, including optional quotes"
                // Transform content of () in a tree and embed it in a fast
                // quote searcher
                return new CNode_AV(expr(contentIt), true);

            } else if (command == L"LST") {

                // Command to try a list of patterns.
                // The patterns are found in CSettings, and nodes are created
                // at need, and kept until the matcher is destroyed.
                return new CNode_List(UTF8fromUTF16(content));

            } else if (command == L"SET") {

                // Command to set a variable
                size_t eq = content.find(L'=');
                if (eq == string::npos)
                    throw parsing_exception("MISSING_EQUAL", 0);
                std::wstring name = content.substr(0, eq);
                std::wstring value = content.substr(eq + 1);
                CUtil::trim(name);
                CUtil::lower(name);
                if (name[0] == L'\\') name.erase(0,1);
                if (name == L"#") {	// #=value
                    return new CNode_Command(CMD_SETSHARP, L"", value);

				} else if (name.length() == 1 && iswdigit(name[0])) {	// \0-9=value
                    return new CNode_Command(CMD_SETDIGIT, name, value);

                } else {	// globalName=value
                    return new CNode_Command(CMD_SETVAR, name, value);
                }

            } else if (command == L"TST") {

                // Command to try and match a variable
                int eq, level;
                for (eq = 0, level = 0; eq < contentSize; eq++) {
                    // Left parameter can be some text containing ()
                    if (content[eq] == L'(' && (!eq || content[eq-1] != L'\\'))
                        level++;
                    else if (content[eq] == L')' && (!eq || content[eq-1] != L'\\'))
                        level--;
                    else if (content[eq] == L'=' && !level)
                        break;
                }
                
                if (eq == contentSize) {
                    CUtil::trim(content);
                    if (content[0] != L'(') CUtil::lower(content);
                    if (content[0] == L'\\') content.erase(0,1);
                    return new CNode_Test(content/*, filter*/);
                }
                startContent += eq + 1;
                std::wstring name = content.substr(0, eq);
                std::wstring value = content.substr(eq+1);
                CUtil::trim(name);
                if (name[0] != L'(') CUtil::lower(name);
                if (name[0] == L'\\') name.erase(0,1);
                if (name == L"#") {
                    return new CNode_Command(CMD_TSTSHARP, L"", value);

				} else if (name.length() == 1 && iswdigit(name[0])) {
                    return new CNode_Command(CMD_TSTDIGIT, name, value);

                } else if (name[0] == L'(' && name[name.size()-1] == L')') {
                    name = name.substr(1, name.size()-2);
                    return new CNode_Command(CMD_TSTEXPAND, name, value);

                } else {
                    return new CNode_Command(CMD_TSTVAR, name, value);
                }

            } else if (command == L"ADDLST") {

                // Command to add a line to a list
                size_t comma = content.find(L',');
                if (comma == string::npos)
                    throw parsing_exception("MISSING_COMMA", 0);
                std::wstring name = content.substr(0, comma);
                std::wstring value = content.substr(comma + 1);
                CUtil::trim(name);
                return new CNode_Command(CMD_ADDLST, name, value/*, filter*/);

            } else if (command == L"ADDLSTBOX") {

                // Command to add a line to a list after user confirm & edit
                size_t comma = content.find(L',');
                if (comma == string::npos)
                    throw parsing_exception("MISSING_COMMA", 0);
                std::wstring value = content.substr(comma + 1);
                std::wstring name = content.substr(0, comma);
                CUtil::trim(name);
                return new CNode_Command(CMD_ADDLSTBOX, name, value/*, filter*/);

            } else if (command == L"URL") {

                // Command to try and match the document URL
                return new CNode_Command(CMD_URL, L"", content/*, filter*/);

            } else if (command == L"TYPE") {

                // Command to check the downloaded file type
                CUtil::trim(content);
                CUtil::lower(content);
				return new CNode_Command_Type(content);

            } else if (command == L"IHDR") {

                // Command to try and match an incoming header
                size_t colon = content.find(':');
                if (colon == string::npos)
                    throw parsing_exception("MISSING_COLON", 0);
                std::wstring name = content.substr(0, colon);
                std::wstring value = content.substr(colon+1);
                CUtil::trim(name);
                CUtil::lower(name);
                return new CNode_Command(CMD_IHDR, name, value/*, filter*/);

            } else if (command == L"OHDR") {

                // Command to try and match an outgoing header
                size_t colon = content.find(L':');
                if (colon == string::npos)
                    throw parsing_exception("MISSING_COLON", 0);
                std::wstring name = content.substr(0, colon);
                std::wstring value = content.substr(colon+1);
                CUtil::trim(name);
                CUtil::lower(name);
                return new CNode_Command(CMD_OHDR, name, value/*, filter*/);

            } else if (command == L"RESP") {

                // Command to try and match the response line
                return new CNode_Command(CMD_RESP, L"", content/*, filter*/);

            } else if (command == L"ALERT") {

                // Command to display a message box
                return new CNode_Command(CMD_ALERT, L"", content/*, filter*/);

            } else if (command == L"CONFIRM") {

                // Command to ask the user for a choice (Y/N)
                return new CNode_Command(CMD_CONFIRM, L"", content/*, filter*/);

            } else if (command == L"STOP") {

                // Command to stop the filter from filtering
                return new CNode_Command(CMD_STOP, L"", L""/*, filter*/);

            } else if (command == L"USEPROXY") {

                // Command to set if the proxy settings will be used
                CUtil::trim(content);
                CUtil::lower(content);
                if (content != L"true" && content != L"false")
                    throw parsing_exception("NEED_TRUE_FALSE", 0);
                return new CNode_Command(CMD_USEPROXY, L"", content/*, filter*/);

            } else if (command == L"SETPROXY") {

                // Command to force the use of a proxy. The command
                // will have no effect if, when matching, there is no corresponding
                // proxy is the settings.
                CUtil::trim(content);
                if (content.empty())
                    throw parsing_exception("NEED_TEXT", 0);
                return new CNode_Command(CMD_SETPROXY, L"", content/*, filter*/);
                
            } else if (command == L"CON") {
#if 0
                // Command to count requests. Matches if (reqNum-1)/z % y = x-1
                int x, y, z;
                try {
                    x = y = z = 0;
                    size_t comma = content.find(',');
                    if (comma == string::npos) throw (int)1;
                    stringstream ss;
                    ss << content.substr(0,comma);
                    ss >> x;
                    if (x<1) throw (int)2;
                    content = content.substr(comma+1);
                    comma = content.find(',');
                    if (comma != string::npos) {
                        ss.clear();
                        ss.str(content.substr(comma+1));
                        ss >> z;
                        if (z<1) throw (int)3;
                        content = content.substr(0, comma);
                    } else {
                        z = 1;
                    }
                    ss.clear();
                    ss.str(content);
                    ss >> y;
                    if (y<1) throw (int)4;
                } catch (int err) {
                    throw parsing_exception("TWO_THREE_INTEGERS", 0);
                }
                return new CNode_Cnx(reached, x, y, z,
                                     filter.owner.cnxNumber);
#endif
				
            } else if (command == L"LOG") {

                // Command to send an log event to the log system
                return new CNode_Command(CMD_LOG, L"", content/*, filter*/);

            } else if (command == L"JUMP") {

                // Command to tell the browser to go to another URL
                CUtil::trim(content);
                return new CNode_Command(CMD_JUMP, L"", content/*, filter*/);

            } else if (command == L"RDIR") {

                // Command to transparently redirect to another URL
                CUtil::trim(content);
                return new CNode_Command(CMD_RDIR, L"", content/*, filter*/);

            } else if (command == L"FILTER") {

                // Command to force body filtering regardless of its type
                CUtil::trim(content);
                CUtil::lower(content);
                if (content != L"true" && content != L"false")
                    throw parsing_exception("NEED_TRUE_FALSE", 0);
                return new CNode_Command(CMD_FILTER, L"", content/*, filter*/);

            } else if (command == L"LOCK") {

                // Command to lock the filter mutex
                return new CNode_Command(CMD_LOCK,L"", L""/*, filter*/);

            } else if (command == L"UNLOCK") {

                // Command to lock the filter mutex
                return new CNode_Command(CMD_UNLOCK, L"", L""/*, filter*/);

            } else if (command == L"KEYCHK") {

                // Command to lock the filter mutex
                CUtil::upper(content);
                return new CNode_Command(CMD_KEYCHK, L"", content/*, filter*/);

            } else if (command == L"NEST" ||
                       command == L"INEST") {

                // Command to match nested tags (with optional content)
                size_t colon = findParamEnd(content, L',');
                if (colon == string::npos)
                    throw parsing_exception("MISSING_COMMA", 0);
                bool hasMiddle = false;
                std::wstring text1 = content.substr(0, colon);
                std::wstring text2;
                std::wstring text3 = content.substr(colon + 1);
                colon = findParamEnd(text3, L',');
                if (colon != string::npos) {
                    text2 = text3.substr(0, colon);
                    text3 = text3.substr(colon + 1);
                    hasMiddle = true;
                }
                CNode *left = NULL, *middle = NULL, *right = NULL;
                try {
					UnicodeString	patNest1(text1.c_str(), text1.length());
					StringCharacterIterator	patNestIt1(patNest1);
                    left = expr(patNestIt1);
                    if (hasMiddle) {
						UnicodeString	patNest2(text2.c_str(), text2.length());
						StringCharacterIterator	patNestIt2(patNest2);
						middle = expr(patNestIt2);
					}
					UnicodeString	patNest3(text3.c_str(), text3.length());
					StringCharacterIterator	patNestIt3(patNest3);
                    right = expr(patNestIt3);
                } catch (parsing_exception e) {
                    if (left)   delete left;
                    if (middle) delete middle;
                    if (right)  delete right;
                    throw e;
                }
                return new CNode_Nest(left, middle, right, command == L"INEST");

            } else if (command == L"ASK") {

                // Command to automate the insertion of an item in one of 2 lists
                size_t colon = content.find(L',');
                if (colon == string::npos)
                    throw parsing_exception("MISSING_COMMA", 0);
                std::wstring allowName = content.substr(0, colon);
                content.erase(0, colon + 1);
                colon = content.find(L',');
                if (colon == string::npos)
                    throw parsing_exception("MISSING_COMMA", 0);
                std::wstring denyName = content.substr(0, colon);
                content.erase(0, colon + 1);
                colon = findParamEnd(content, L',');
                if (colon == string::npos)
                    throw parsing_exception("MISSING_COMMA", 0);
                std::wstring question = content.substr(0, colon);
                std::wstring item = content.substr(colon + 1);
                std::wstring pattern = L"\\h\\p\\q\\a";
                colon = findParamEnd(item, L',');
                if (colon != string::npos) {
                    pattern = item.substr(colon + 1);
                    item = item.substr(0, colon);
                }
                CUtil::trim(allowName);
                CUtil::trim(denyName);
                CUtil::trim(question);
                CUtil::trim(item);
                CUtil::trim(pattern);
                return new CNode_Ask(/*filter, */allowName, denyName, question, item, pattern);

            } else {

                // Unknown command
                throw parsing_exception("UNKNOWN_COMMAND", -1);
            }
        } catch (parsing_exception e) {
            // Error: adjust errorPos and forward error
            e.position += startContent;
            throw e;
        }
    // Expression
    } else if (token == L'(') {

        // Rule: ()
        patternIt.next();
        // Detect a negation symbol
        bool negate = false;
		if (patternIt.current() != patternIt.DONE && patternIt.current() == L'^') {
			patternIt.next();
            negate = true;
        }
            
        // We read the enclosed expression
        CNode *node = expr(patternIt);
        
        // It must be followed by the closing )
		// ')'がこない、もしくは 次のトークンに')'が来ないと例外を飛ばす
		if (patternIt.hasNext() == false || patternIt.current() != L')') {
            delete node;
			throw parsing_exception("MISSING_PARENTHESE", patternIt.getIndex());
        }
        patternIt.next();

        // Rule: (^ )
        // If the expression must be negated, embed it in a CNode_Negate
        if (negate)
            node = new CNode_Negate(node);

		if (patternIt.hasNext() && patternIt.current() == L'\\') {
			UChar dig = patternIt.next();
			if (iswdigit(dig)) {
                // Rule: ()\0-9
                // The code will be embedded in a memory node
                int num = dig - L'0';
                node = new CNode_Memory(node, num);

                patternIt.next();
            } else if (dig == L'#') {
                // Rule: ()\#
                // The code will be embedded in a stacked-memory node
                node = new CNode_Memory(node, -1);
                patternIt.next();
            } else {
				patternIt.previous();
			}
        }

        return node;

    // End of expression
    } else if (token == L')' || token == L'|' || token == L'&') {

        return NULL;    // This NULL will be interpreted by run()
        
    }

    // Single character (which can have a special meaning)
    return single(patternIt);
}


/* Builds a node for a single symbol at position pos. It can be
 * a character with a special meaning (space, =, ?, *, ', ") or
 * a ascii character to be matched (case insensitive).
 */
CNode* CMatcher::single(StringCharacterIterator& patternIt) {

    // No need to test for pos<stop here, the test is done in code()
	UChar token = patternIt.current();
	patternIt.next();

    if (token == L'?') {

        // Rule: ?
        return new CNode_Any();
        
    } else if (token == L' ' || token == L'\t') {

        // Only one CNode_Space for consecutive spaces
		while (patternIt.current() != patternIt.DONE && patternIt.current() <= L' ') patternIt.next();
        
        // No need creating a CNode_Space for spaces before = in the pattern
        if (patternIt.current() != patternIt.DONE && patternIt.current() == L'=')
            return single(patternIt);
            
        // Rule: space
        return new CNode_Space();
		
    } else if (token == L'=') {

        // Rule: =
        while (patternIt.current() != patternIt.DONE && patternIt.current() <= L' ') patternIt.next();
        return new CNode_Equal();

    } else if (token == L'*') {

        // Rule: *
        return new CNode_Star();

    } else if (token == L'\'' || token == L'\"') {

        // Rule: "
        // Rule: '
        return new CNode_Quote(token);

    } else {

        // ascii character
		return new CNode_Char(towlower(token));
    }
}



}	// namespace Proxydomo
















