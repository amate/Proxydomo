/**
*	@file	TextBuffer.cpp
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
#include "stdafx.h"
#include "TextBuffer.h"
#include <sstream>
#include "FilterOwner.h"
#include "Settings.h"
//#include "proximodo\matcher.h"
#include "Matcher.h"
#include "proximodo\util.h"
#include "Log.h"

CTextBuffer::CTextBuffer(CFilterOwner& owner, IDataReceptor* output) : m_owner(owner), m_output(output)
{
	CCritSecLock	lock(CSettings::s_csFilters);
	for (auto& filter : CSettings::s_vecpFilters) {
		if (filter->errorMsg.empty() && filter->Active && filter->filterType == filter->kFilterText) 
		{
			m_vecpTextfilters.emplace_back(new CTextFilter(owner, *filter));
		}
	}

	m_currentFilter = m_vecpTextfilters.begin();
}


CTextBuffer::~CTextBuffer(void)
{
}


// IDataReceptor

void CTextBuffer::DataReset()
{
	m_buffer.clear();
	m_currentFilter = m_vecpTextfilters.begin();
	for (auto& filter : m_vecpTextfilters)
		filter->reset();

	m_output->DataReset();
}

void CTextBuffer::DataFeed(const std::string& data)
{
	if (m_owner.killed)
		return ;
	m_buffer += data;
	std::stringstream out;
  
    const char* bufStart = m_buffer.data();
    const char* bufEnd   = bufStart + m_buffer.size();
    const char* index    = bufStart;
    const char* done     = bufStart;
    
    // test every filter at every position
    for (; index <= bufEnd; m_currentFilter = m_vecpTextfilters.begin(), ++index) {
        for (; m_currentFilter != m_vecpTextfilters.end(); ++m_currentFilter) {

            // don't need to test the filter if character is not a good candidate
            if ((*m_currentFilter)->bypassed ||
                (index < bufEnd && !(*m_currentFilter)->okayChars[(unsigned char)(*index)]))
                continue;

            // try and match
            int ret = (*m_currentFilter)->match(index, bufEnd);

            if (ret < 0) {

                if (m_owner.killed) {

                    // filter would like more data, but killed the stream anyway
                    escapeOutput(out, done, (size_t)(index - done));
                    m_buffer.clear();
                    m_output->DataFeed(out.str());
                    m_output->DataDump();
                    return;         // no more processing
                    
                } else {

                    // filter does not have enough data to provide an accurate result
                    escapeOutput(out, done, (size_t)(index - done));
                    m_buffer.erase(0, (size_t)(index - bufStart));
                    m_output->DataFeed(out.str());
                    return;         // when more data arrives, same filter, same position
                }

            } else if (ret > 0) {

                string replaceText = (*m_currentFilter)->getReplaceText();

                // log match events
                string occurrence(index, (size_t)((*m_currentFilter)->endOfMatched - index));
				CLog::FilterEvent(kLogFilterTextMatch, m_owner.requestNumber, (*m_currentFilter)->title, occurrence);
				CLog::FilterEvent(kLogFilterTextReplace, m_owner.requestNumber, (*m_currentFilter)->title, replaceText);

                escapeOutput(out, done, (size_t)(index - done));
                if (m_owner.url.getDebug()) {
                    string buf = "<div class=\"match\">\n"
                        "<div class=\"filter\">Match: ";
                    CUtil::htmlEscape(buf, (*m_currentFilter)->title);
                    buf += "</div>\n<div class=\"in\">";
                    CUtil::htmlEscape(buf, occurrence);
                    buf += "</div>\n<div class=\"repl\">";
                    CUtil::htmlEscape(buf, replaceText);
                    buf += "</div>\n</div>\n";
                    out << buf;
                }
                if (m_owner.killed) {

                    // filter matched and killed the stream
                    if (!m_owner.url.getDebug())
                        out << replaceText;
                    m_buffer.clear();
                    m_output->DataFeed(out.str());
                    m_output->DataDump();
                    return;         // no more processing

                } else if ((*m_currentFilter)->multipleMatches) {

                    // filter matched, insert replace text in buffer
                    m_buffer.replace(0, (size_t)((*m_currentFilter)->endOfMatched - bufStart),
                                   replaceText);
                    bufStart = m_buffer.data();
                    bufEnd   = bufStart + m_buffer.size();
                    index    = bufStart;
                    done     = bufStart;
                    continue;       // next filter, same position
                    
                } else {

                    // filter matched, output replace text.
                    if (!m_owner.url.getDebug())
                        out << replaceText;
                    done = (*m_currentFilter)->endOfMatched;
                    // If the occurrence is length 0, we'll try next filters
                    // then move 1 byte, to avoid infinite loop on the filter.
                    if (done == index) continue;
                    index = done - 1;
                    break;          // first filter, after the occurrence
                }

            } else if (m_owner.killed) {

                // filter did not match but killed the stream
                escapeOutput(out, done, (size_t)(index - done));
                m_buffer.clear();
                m_output->DataFeed(out.str());
                m_output->DataDump();
                return;             // no more processing
            }
        }
    }
    
    // we processed all the available data.
    // Add unmatching data left and send everything.
    escapeOutput(out, done, (size_t)(bufEnd - done));
    m_buffer.clear();
    m_output->DataFeed(out.str());

}

void CTextBuffer::DataDump()
{
	if (m_owner.killed)
		return ;

	for (auto& filter : m_vecpTextfilters)
		filter->endReached();

	DataFeed("");
	m_output->DataDump();
	m_owner.killed = true;
}


void CTextBuffer::escapeOutput(std::stringstream& out, const char *data,
                               size_t len) {
    if (m_owner.url.getDebug()) {
        std::string buf;
        CUtil::htmlEscape(buf, std::string(data, len));
        out << buf;
    } else {
        out << std::string(data, len);
    }
}
