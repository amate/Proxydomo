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
#include "CodeConvert.h"
#include <atlsync.h>
#include <regex>

using namespace CodeConvert;
using namespace icu;

////////////////////////////////////////////////////////////////////////
// CTextBuffer

namespace {

UCharsetDetector*	g_dectator;
CCriticalSection	g_csMapConverter;
std::map<std::string, UConverter*>	g_mapConverter;

}

CTextBuffer::CTextBuffer(CFilterOwner& owner, IDataReceptor* output) : 
	m_owner(owner), m_output(output), m_bCharaCodeDectated(false), m_pConverter(nullptr)
{
	if (g_dectator == nullptr) {
		UErrorCode err = UErrorCode::U_ZERO_ERROR;
		g_dectator = ucsdet_open(&err);
		ATLASSERT( U_SUCCESS(err) );
	}

	CSettings::EnumActiveFilter([&, this](CFilterDescriptor* filter) {
		if (filter->filterType == filter->kFilterText)
			m_vecpTextfilters.emplace_back(new CTextFilter(owner, *filter));
	});
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

	m_tailBuffer.clear();
	m_unicodeBuffer.remove();

	m_bCharaCodeDectated = false;
	m_pConverter = nullptr;
	m_output->DataReset();	
}

/// EUC-JP, Shift-JIS, UTF-8 の末尾を調べる
static int findEndPoint(const char* start, const char*& end)
{
	enum { kMaxDecriment = 512 };
	--end;
	for (int decrimentCount = 0; 
			start < end && decrimentCount < kMaxDecriment; ++decrimentCount, --end)
	{
		unsigned char code = *end;
		if (/*0x20*/0 < code && code <= 0x7E) {	// acii	// Shift-JISだと2byte目の可能性もあるが大丈夫
			++end;
			return decrimentCount;
		}
	}
	return -1;
}

inline int CharCount(const wchar_t* end, const wchar_t* begin)
{
#ifdef _DEBUG
	int count = (end - begin);
	return count;
#else
	return (end - begin);
#endif
}


static std::string GetCharaCode(const std::string& data)
{
	UErrorCode err = UErrorCode::U_ZERO_ERROR;
	ucsdet_setText(g_dectator, data.c_str(), data.length(), &err);
	ATLASSERT( U_SUCCESS( err ) );
	err = UErrorCode::U_ZERO_ERROR;
	const UCharsetMatch* charaCodeMatch = ucsdet_detect(g_dectator, &err);
	ATLASSERT( charaCodeMatch );
	ATLASSERT( U_SUCCESS( err ) );
	err = UErrorCode::U_ZERO_ERROR;
	std::string charaCode = ucsdet_getName(charaCodeMatch, &err);
	return charaCode;
}

void CTextBuffer::DataFeed(const std::string& data)
{
	if (m_owner.killed)
		return ;

	/// 文字コードを判別する
	if (m_bCharaCodeDectated == false) {
		
		m_buffer += data;
		enum { kMaxBufferForCharaCodeSearch = 5000 };
		if (m_buffer.size() < kMaxBufferForCharaCodeSearch) {
			if (data.size() > 0)
				return ;	// バッファがたまるまで待つ
		}
		std::string charaCode;

		std::string contentType = m_owner.GetInHeader("Content-Type");
		if (contentType.size() > 0) {
			CUtil::lower(contentType);
			int nPos = contentType.find("charset=");
			if (nPos != std::string::npos) {
				charaCode = contentType.substr(nPos + 8);
				CUtil::upper(charaCode);
			}
		}
		if (charaCode.empty()) {
			std::regex rx("<meta [^>]*http-equiv=\"?Content-Type\"? [^>]*charset=\"?([^\"' >]+)\"?[^>]*>", std::regex_constants::icase);
			std::smatch result;
			if (std::regex_search(m_buffer.cbegin(), m_buffer.cend(), result, rx)) {
				charaCode = result.str(1);
				CUtil::upper(charaCode);
			}
		}
		if (charaCode.empty())
			charaCode = GetCharaCode(m_buffer);

		CCritSecLock lock(g_csMapConverter);
		auto& pConverter = g_mapConverter[charaCode];
		if (pConverter == nullptr) {
			UErrorCode err = UErrorCode::U_ZERO_ERROR;
			pConverter = ucnv_open(charaCode.c_str(), &err);
			ATLASSERT( pConverter );			
		}
		m_pConverter = pConverter;
		m_bCharaCodeDectated = true;
	} else {
		m_buffer += m_tailBuffer;
		m_tailBuffer.clear();
		m_buffer += data;
	}
	std::stringstream out;

	const char* endPoint = m_buffer.c_str() + m_buffer.size();
	int decrimentCount = findEndPoint(m_buffer.c_str(), endPoint);
	if (decrimentCount == -1) {
		if (data.size() > 0)
			return ;	// 末尾が見つからなかったら危ないので帰る
		decrimentCount = 0;	// for data dump
	}
	// あまりを保存しておく
	m_tailBuffer.assign(endPoint, decrimentCount);

	int validBufferSize = endPoint - m_buffer.c_str();
	if (validBufferSize > 0) {
		UErrorCode	err = U_ZERO_ERROR;
		UnicodeString	appendBuff(m_buffer.c_str(), validBufferSize, m_pConverter, err);
		ATLASSERT( U_SUCCESS(err) );
		m_buffer.clear();
		m_unicodeBuffer.append(appendBuff);
	}
	const UChar* bufStart = m_unicodeBuffer.getBuffer();
	int len = m_unicodeBuffer.length();
	if (len == 0)
		return ;
	const UChar* bufEnd   = bufStart + len;
    const UChar* index    = bufStart;
    const UChar* done     = bufStart;
    
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

                if (m_owner.killed) {	// '\k'	?

                    // filter would like more data, but killed the stream anyway
                    escapeOutput(out, done, CharCount(index, done));
					m_unicodeBuffer.remove();
                    m_output->DataFeed(out.str());
                    m_output->DataDump();
                    return;         // no more processing
                    
                } else {

                    // filter does not have enough data to provide an accurate result
                    escapeOutput(out, done, CharCount(index, done));
					m_unicodeBuffer.remove(0, CharCount(index, bufStart));
                    m_output->DataFeed(out.str());
                    return;         // when more data arrives, same filter, same position
                }

            } else if (ret > 0) {

                std::wstring replaceText = (*m_currentFilter)->getReplaceText();

                // log match events                
				// フィルタがマッチした
				CLog::FilterEvent(kLogFilterTextMatch, m_owner.requestNumber, 
					UTF8fromUTF16((*m_currentFilter)->title), ""/*occurrence*/);
				//CLog::FilterEvent(kLogFilterTextReplace, m_owner.requestNumber, (*m_currentFilter)->title, replaceText);

                escapeOutput(out, done, CharCount(index, done));
                if (m_owner.url.getDebug()) {
					std::wstring occurrence(index, 
											CharCount((*m_currentFilter)->endOfMatched, index));
                    string buf = "<div class=\"match\">\n"
                        "<div class=\"filter\">Match: ";
                    CUtil::htmlEscape(buf, ConvertFromUTF16((*m_currentFilter)->title, m_pConverter));
                    buf += "</div>\n<div class=\"in\">";
                    CUtil::htmlEscape(buf, ConvertFromUTF16(occurrence, m_pConverter));
                    buf += "</div>\n<div class=\"repl\">";
                    CUtil::htmlEscape(buf, ConvertFromUTF16(replaceText, m_pConverter));
                    buf += "</div>\n</div>\n";
                    out << buf;
                }
                if (m_owner.killed) {

                    // filter matched and killed the stream
                    if (!m_owner.url.getDebug())
                        out << ConvertFromUTF16(replaceText, m_pConverter);

					m_unicodeBuffer.remove();
                    m_output->DataFeed(out.str());
                    m_output->DataDump();
                    return;         // no more processing

                } else if ((*m_currentFilter)->multipleMatches) {

                    // filter matched, insert replace text in buffer
                    //m_buffer.replace(0, (size_t)((*m_currentFilter)->endOfMatched - bufStart),
                    //               replaceText);
                    //bufStart = m_buffer.data();
					m_unicodeBuffer.replace(0, 
						CharCount((*m_currentFilter)->endOfMatched, bufStart), 
						replaceText.c_str(), replaceText.length());
					bufStart = m_unicodeBuffer.getBuffer();
					bufEnd   = bufStart + m_unicodeBuffer.length();
                    index    = bufStart;
                    done     = bufStart;
                    continue;       // next filter, same position
                    
                } else {

                    // filter matched, output replace text.
                    if (m_owner.url.getDebug() == false)
                        out << ConvertFromUTF16(replaceText, m_pConverter);
                    done = (*m_currentFilter)->endOfMatched;
                    // If the occurrence is length 0, we'll try next filters
                    // then move 1 byte, to avoid infinite loop on the filter.
                    if (done == index) 
						continue;
                    index = done - 1;
                    break;          // first filter, after the occurrence
                }

            } else if (m_owner.killed) {

                // filter did not match but killed the stream
				escapeOutput(out, done, CharCount(index, done));
                m_output->DataFeed(out.str());
                m_output->DataDump();
                return;             // no more processing
            }
        }
    }
    
    // we processed all the available data.
    // Add unmatching data left and send everything.
    escapeOutput(out, done, CharCount(bufEnd, done));
	m_unicodeBuffer.remove();
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


void CTextBuffer::escapeOutput(std::stringstream& out, const UChar *data, size_t len)
{
	std::string webcode = ConvertFromUTF16(data, len, m_pConverter);
    if (m_owner.url.getDebug()) {
        std::string buf;
        CUtil::htmlEscape(buf, webcode);
        out << buf;
    } else {
        out << webcode;
    }
}
