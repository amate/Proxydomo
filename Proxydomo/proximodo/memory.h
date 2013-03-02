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


#ifndef __memory__
#define __memory__

#include <string>

/* class CMemory
 * Represents a matched string, by its position in original string or by value
 * Used to retain \1 or \#
 */
class CMemory {

private:
    const wchar_t* posStart;   // start position in text buffer
    const wchar_t* posEnd;     // end position in text
    std::wstring* content;    // (pointer so that we don't construct string if not used)

public:
    // Default constructor
    inline CMemory();
    
    // Constructor (by value)
    inline CMemory(const std::wstring& value);

    // Constructor (by markers)
    inline CMemory(const wchar_t* posStart, const wchar_t* posEnd);

    // Copy constructor
    inline CMemory(const CMemory& mem);

    // Destructor
    inline ~CMemory();

    // created with a value? (then it must be expanded)
    inline bool isByValue();

    // Get the retained value
    inline std::wstring getValue();

    // Change operators
    inline CMemory& operator()(const std::wstring& value);
    inline CMemory& operator()(const wchar_t* posStart, const wchar_t* posEnd);

    // Clear content
    inline void clear();

    // Assignment operator
    inline CMemory& operator=(const CMemory& mem);
};


/* Default constructor
 */
CMemory::CMemory() : posStart(NULL), posEnd(NULL), content(NULL) {
}


/* Constructor (by value)
 */
CMemory::CMemory(const std::wstring& value) : content(new std::wstring(value))
{
}


/* Constructor (by markers)
 */
CMemory::CMemory(const wchar_t* posStart, const wchar_t* posEnd) :
        posStart(posStart), posEnd(posEnd), content(NULL)
{
}


/* Copy constructor
 */
CMemory::CMemory(const CMemory& mem) {
    if (mem.content) {
        content = new std::wstring(*mem.content);
    } else {
        content  = NULL;
        posStart = mem.posStart;
        posEnd   = mem.posEnd;
    }
}


/* Destructor
 */
CMemory::~CMemory() {
    if (content) delete content;
}


/* created with a value? (then it must be expanded)
 */
bool CMemory::isByValue() {
    return (content != NULL);
}


/* Get the retained value
 */
std::wstring CMemory::getValue() {
    return (content ? *content : posStart < posEnd ?
            std::wstring(posStart, posEnd - posStart) : std::wstring(L"") );
}


/* Assignment operator
 */
CMemory& CMemory::operator=(const CMemory& mem) {
    if (mem.content) {
        if (content) {
            *content = *mem.content;
        } else {
            content = new std::wstring(*mem.content);
        }
    } else {
        if (content) delete content;
        content  = NULL;
        posStart = mem.posStart;
        posEnd   = mem.posEnd;
    }
    return *this;
}


/* Change operators
 */
CMemory& CMemory::operator()(const std::wstring& value) {
    if (content) {
        *content = value;
    } else {
        content = new std::wstring(value);
    }
    return *this;
}

CMemory& CMemory::operator()(const wchar_t* posStart, const wchar_t* posEnd) {
    if (content) { delete content; content = NULL; }
    this->posStart = posStart;
    this->posEnd   = posEnd;
    return *this;
}


/* Clear content
 */
void CMemory::clear() {
    if (content) { delete content; content = NULL; }
    posStart = posEnd = NULL;
}

#endif
