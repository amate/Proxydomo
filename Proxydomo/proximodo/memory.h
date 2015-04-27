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
#include <memory>

/* class CMemory
 * Represents a matched string, by its position in original string or by value
 * Used to retain \1 or \#
 */
class CMemory
{
public:
    // Default constructor
    inline CMemory();
    
    // Constructor (by value)
    inline CMemory(const std::wstring& value);

    // Constructor (by markers)
    inline CMemory(const wchar_t* posStart, const wchar_t* posEnd, bool bSave);

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
    inline CMemory& operator()(const wchar_t* posStart, const wchar_t* posEnd, bool bSave);

    // Clear content
    inline void clear();

    // Assignment operator
    inline CMemory& operator=(const CMemory& mem);

private:
	// Data members
	const wchar_t*	m_posStart;
	const wchar_t*	m_posEnd;
	std::unique_ptr<std::wstring> m_pText;
    std::unique_ptr<std::wstring> m_pContent;  // (pointer so that we don't construct string if not used)
};


/* Default constructor
 */
CMemory::CMemory() : m_posStart(nullptr), m_posEnd(nullptr) {
}


/* Constructor (by value)
 */
CMemory::CMemory(const std::wstring& value) : m_posStart(nullptr), m_posEnd(nullptr),
	m_pContent(new std::wstring(value))
{
}


/* Constructor (by markers)
 */
CMemory::CMemory(const wchar_t* posStart, const wchar_t* posEnd, bool bSave) : 
	m_posStart(posStart), m_posEnd(posEnd)
{
	if (bSave)
		m_pText.reset(new std::wstring(posStart, posEnd));
}


/* Copy constructor
 */
CMemory::CMemory(const CMemory& mem) {
	*this = mem;
}


/* Destructor
 */
CMemory::~CMemory() {
}


/* created with a value? (then it must be expanded)
 */
bool CMemory::isByValue() {
    return (m_pContent != nullptr);
}


/* Get the retained value
 */
std::wstring CMemory::getValue() {
	if (m_pContent) {
		return *m_pContent;

	} else if (m_pText) {
		return *m_pText;

	} else if (m_posStart) {
		return std::wstring(m_posStart, m_posEnd);

	} else {
		return L"";
	}
}


/* Assignment operator
 */
CMemory& CMemory::operator=(const CMemory& mem) {
	clear();

    if (mem.m_pContent) {
		m_pContent.reset(new std::wstring(*mem.m_pContent));

    } else {
		if (mem.m_pText) {
			m_pText.reset(new std::wstring(*mem.m_pText));
		} else {
			m_posStart = mem.m_posStart;
			m_posEnd = mem.m_posEnd;
		}
    }
    return *this;
}


/* Change operators
 */
CMemory& CMemory::operator()(const std::wstring& value) {
    if (m_pContent) {
        *m_pContent = value;
    } else {
        m_pContent.reset(new std::wstring(value));
    }
    return *this;
}

CMemory& CMemory::operator()(const wchar_t* posStart, const wchar_t* posEnd, bool bSave) {
    if (m_pContent) 
		m_pContent.reset();

	if (bSave) {
		m_pText.reset(new std::wstring(posStart, posEnd));

		m_posStart = nullptr;
		m_posEnd = nullptr;

	} else {
		m_posStart = posStart;
		m_posEnd = posEnd;

		m_pText.reset();
	}
    return *this;
}


/* Clear content
 */
void CMemory::clear() {
	m_pContent.reset();
	m_posStart = nullptr;
	m_posEnd = nullptr;
	m_pText.reset();
}

#endif
