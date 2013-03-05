/**
*	@file	TextBuffer.h
*	@brief	データを受け取ってテキストフィルターで処理してRequestManagerに返すクラス
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

#include <vector>
#include <memory>
#include <map>
#include <sstream>
#include "DataReceptor.h"
#include "proximodo\textfilter.h"
#include <unicode\ustring.h>
#include <unicode\ucnv.h>
#include <unicode\ucsdet.h>

class CFilterOwner;
class CTextFilter;

class CTextBuffer : public IDataReceptor
{
public:
	CTextBuffer(CFilterOwner& owner, IDataReceptor* output);
	~CTextBuffer();

	// IDataReceptor
    void DataReset();
    void DataFeed(const std::string& data);
    void DataDump();

private:
	// Data members

    // the object which contains header values and variables
    CFilterOwner&	m_owner;

    // where to send processed data
    IDataReceptor*	m_output;

    // the filters
    std::vector<std::unique_ptr<CTextFilter>> m_vecpTextfilters;
    
    // the next filter to try (if a filter needs more data,
    // we'll start with it on next data arrival)
    std::vector<std::unique_ptr<CTextFilter>>::iterator	m_currentFilter;

    // pass string to output, escaping HTML chars as needed
    void escapeOutput(std::stringstream& out, const UChar *data, size_t len);
    
    // the actual buffer
    std::string	m_buffer;
	std::string m_tailBuffer;

	bool	m_bCharaCodeDectated;
	icu::UnicodeString	m_unicodeBuffer;
	UConverter*	m_pConverter;
};

