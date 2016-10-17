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
#include "Logger.h"

using namespace CodeConvert;
using namespace icu;

////////////////////////////////////////////////////////////////////////
// CTextBuffer

namespace {

CCriticalSection	g_csMapConverter;
std::map<std::string, UConverter*>	g_mapConverter;

}

CTextBuffer::CTextBuffer(CFilterOwner& owner, IDataReceptor* output) : 
	m_owner(owner), m_output(output), 
	m_bCharaCodeDectated(false), m_pConverter(nullptr), m_bDataDump(false), m_bJISCode(false), m_bBinary(false)
{
#if 0
	CSettings::EnumActiveFilter([&, this](CFilterDescriptor* filter) {
		if (filter->filterType == filter->kFilterText)
			m_vecpTextfilters.emplace_back(new CTextFilter(owner, *filter));
	});
	m_currentFilter = m_vecpTextfilters.begin();
#endif
}


CTextBuffer::~CTextBuffer(void)
{
}

void	CTextBuffer::_ReloadFilters()
{
	if (m_lastFilterEnumTime != CSettings::s_lastFiltersSaveTime) {
		m_vecpTextfilters.clear();
		m_lastFilterEnumTime = CSettings::EnumActiveFilter([this](CFilterDescriptor* filter) {
			if (filter->filterType == filter->kFilterText)
				m_vecpTextfilters.emplace_back(new CTextFilter(m_owner, *filter));
		});
	}
}

// IDataReceptor

void CTextBuffer::DataReset()
{
	m_buffer.clear();
	_ReloadFilters();
	m_currentFilter = m_vecpTextfilters.begin();
	for (auto& filter : m_vecpTextfilters)
		filter->reset();

	m_tailBuffer.clear();
	m_unicodeBuffer.remove();

	m_bCharaCodeDectated = false;
	m_pConverter = nullptr;
	m_output->DataReset();	
	m_bDataDump = false;
	m_bJISCode = false;
	m_bBinary = false;
}

/// EUC-JP, Shift-JIS, UTF-8, JIS の末尾を調べる
int CTextBuffer::_findEndPoint(const char* start, const char*& end)
{
	enum { kMaxDecriment = 512 };
	--end;
	if (m_bJISCode) {
		for (int decrimentCount = 0;
			start < end && decrimentCount < kMaxDecriment; 
			++decrimentCount, --end )
		{
			unsigned char code = *end;
			if (code == 0x1B) {	// ESC
				if (2 <= decrimentCount) {
					unsigned char esc1 = *std::next(end);
					unsigned char esc2 = *std::next(end, 2);
					if (esc1 == 0x28 && esc2 == 0x42) {	// (B
						std::advance(end, 3);
						decrimentCount -= 2;
						return decrimentCount;
					}
				}
			}
		}
	} else {
		for (int decrimentCount = 0;
			start < end && decrimentCount < kMaxDecriment; 
			++decrimentCount, --end )
		{
			unsigned char code = *end;
			if (/*0x20*/0 < code && code <= 0x7E) {	// acii	// Shift-JISだと2byte目の可能性もあるが大丈夫
				++end;
				return decrimentCount;
			}
		}
	}
	return -1;
}

inline int CharCount(const wchar_t* end, const wchar_t* begin)
{
#ifdef _DEBUG
	size_t count = (end - begin);
	return (int32_t)count;
#else
	return static_cast<int>(end - begin);
#endif
}


static std::pair<int32_t, std::string> GetCharaCode(const std::string& data)
{
	UCharsetDetector* pDectator = nullptr;
	try {
		UErrorCode err = UErrorCode::U_ZERO_ERROR;
		pDectator = ucsdet_open(&err);
		ATLASSERT( U_SUCCESS(err) );
		if (err != UErrorCode::U_ZERO_ERROR)
			throw err;

		ucsdet_setText(pDectator, data.c_str(), (int32_t)data.length(), &err);
		ATLASSERT( U_SUCCESS( err ) );
		if (err != UErrorCode::U_ZERO_ERROR)
			throw err;
		err = UErrorCode::U_ZERO_ERROR;

#if 0
		const UCharsetMatch* charaCodeMatch = ucsdet_detect(pDectator, &err);
		ATLASSERT( charaCodeMatch );
		ATLASSERT( U_SUCCESS( err ) );
		if (err != UErrorCode::U_ZERO_ERROR)
			throw err;

		err = UErrorCode::U_ZERO_ERROR;
		std::string charaCode = ucsdet_getName(charaCodeMatch, &err);
		if (err != UErrorCode::U_ZERO_ERROR)
			throw err;
#endif

#if 0
		int32_t founded = 0;
		int32_t confidence = 0;
		std::string charaCode;
		const UCharsetMatch** matchers = ucsdet_detectAll(pDectator, &founded, &err);
		if (founded > 0) {
			const int32_t kThresold = 10;
			for (int i = 0; i < founded; ++i) {
				confidence = ucsdet_getConfidence(matchers[i], &err);
				if (U_FAILURE(err)) {
					err = U_ZERO_ERROR;
					continue;
				}
				if (confidence < kThresold)
					break;
				const char* match_encoding = ucsdet_getName(matchers[i], &err);
				if (U_FAILURE(err)) {
					err = U_ZERO_ERROR;
					continue;
				}
				const char* match_lang = ucsdet_getLanguage(matchers[i], &err);
				if (U_FAILURE(err)) {
					err = U_ZERO_ERROR;
					continue;
				}
				INFO_LOG << "confidence: " << confidence << L" name: " << (LPWSTR)CA2W(match_encoding);
				//std::cout << "     neme:" << match_encoding
				//	<< " language:" << match_lang
				//	<< std::endl;
			}
		}
#endif

		int32_t confidence = 0;
		std::string charaCode;

		int32_t founded = 0;
		const UCharsetMatch** matchers = ucsdet_detectAll(pDectator, &founded, &err);
		if (founded > 0) {
			confidence = ucsdet_getConfidence(matchers[0], &err);
			if (U_FAILURE(err)) {
				confidence = 0;
				throw err;
			}
			charaCode = ucsdet_getName(matchers[0], &err);
			if (U_FAILURE(err))
				throw err;
		}

		ucsdet_close(pDectator);
		return std::pair<int32_t, std::string>(confidence, charaCode);

	} catch (UErrorCode err) {
		err;
		ATLASSERT( FALSE );
		if (pDectator)
			ucsdet_close(pDectator);
		return std::pair<int32_t, std::string>(0, "");
	}
}

void	CTextBuffer::_decideCharset()
{
	std::string charaCode;

	auto funcPageCharset = [this]() -> std::string {
		std::string charaCode;
		if (m_owner.fileType == "htm") {
			static std::regex rxMeta("(?:<meta ([^>]+)>|</head>)", std::regex_constants::icase);
			auto begin = m_buffer.cbegin();
			std::smatch result;
			while (std::regex_search(begin, m_buffer.cend(), result, rxMeta)) {
				std::string strAttribute = result.str(1);
				if (strAttribute.length() == 0)	// </head>
					break;
				std::smatch attrResult;
				static std::regex rx1("http-equiv=(?:\"|')?Content-Type(?:\"|')? [^>]*charset=(?:\"|')?([^\"' >]+)(?:\"|')?", std::regex_constants::icase);
				static std::regex rx2("charset=(?:\"|')?([^\"' >]+)(?:\"|')? [^>]*http-equiv=(?:\"|')?Content-Type(?:\"|')?", std::regex_constants::icase);
				static std::regex rx3("charset=(?:\"|')?([^\"' >]+)(?:\"|')?", std::regex_constants::icase);
				if (std::regex_search(strAttribute, attrResult, rx1) ||
					std::regex_search(strAttribute, attrResult, rx2) ||
					std::regex_search(strAttribute, attrResult, rx3))
				{
					charaCode = attrResult.str(1);
					CUtil::upper(charaCode);
					if (charaCode == "NONE" || charaCode == "UNICODE")
						charaCode.clear();
					break;
				}
				begin = result[0].second;
			}
		}
		return charaCode;
	};

	auto funcInHeaderCharset = [this]() -> std::string {
		std::string charaCode;
		std::wstring contentType = m_owner.GetInHeader(L"Content-Type");
		if (contentType.size() > 0) {
			CUtil::lower(contentType);
			size_t nPos = contentType.find(L"charset=");
			if (nPos != std::wstring::npos) {
				charaCode = UTF8fromUTF16(contentType.substr(nPos + 8));
				CUtil::upper(charaCode);
				if (charaCode == "NONE" || charaCode == "UNICODE")
					charaCode.clear();
			}
		}
		return charaCode;
	};

	// 1:ページから文字コードを推定する
	enum { kThreshold = 70 };
	auto supposeCharaCode = GetCharaCode(m_buffer);
	if (kThreshold <= supposeCharaCode.first) {	// 推定が閾値を超えていれば判定を優先する
		charaCode = supposeCharaCode.second;
	}

	enum PriorityCharset {
		kSupposeCharset = 1,
		kPageCharset = 2,
		kInHeaderCharset = 3,
	};

	CFilter filter(m_owner);
	std::wstring urlFromHost = m_owner.url.getFromHost();
	if (CSettings::s_pPriorityCharsetMatcher->match(urlFromHost, &filter)) {
		try {
			PriorityCharset priority = static_cast<PriorityCharset>(std::stoi(filter.memoryTable[0].getValue()));
			switch (priority) {
			case kSupposeCharset:
				charaCode = supposeCharaCode.second;
				break;

			case kPageCharset:
				charaCode = funcPageCharset();
				break;

			case kInHeaderCharset:
				charaCode = funcInHeaderCharset();
				break;

			default:
				ATLASSERT(FALSE);
			}
		}
		catch (...) {
			ATLASSERT(FALSE);
		}
	}

	// 2:ページ内の文字コード指定
	if (charaCode.empty()) {
		charaCode = funcPageCharset();
	}

	// 3:レスポンスヘッダのContent-Typeに書いてあるcharset
	if (charaCode.empty()) {
		charaCode = funcInHeaderCharset();
	}

	if (charaCode.empty()) {
		charaCode = supposeCharaCode.second;	// ページ推定のcharaset
		if (charaCode.empty()) {
			charaCode = "UTF-8";	// ここまで何もなかったらUTF-8として解釈する
		}
	}

	ATLASSERT(charaCode.length());

	CCritSecLock lock(g_csMapConverter);
	auto& pConverter = g_mapConverter[charaCode];
	if (pConverter == nullptr) {
		UErrorCode err = UErrorCode::U_ZERO_ERROR;
		pConverter = ucnv_open(charaCode.c_str(), &err);

		// Converterが取れなかったので supposeCharaCode.second から取得する
		if (pConverter == nullptr && supposeCharaCode.second.length()) {
			charaCode = supposeCharaCode.second;
			auto& pConverter2 = g_mapConverter[charaCode];
			if (pConverter2 == nullptr) {
				err = UErrorCode::U_ZERO_ERROR;
				pConverter2 = ucnv_open(charaCode.c_str(), &err);
				ATLASSERT(pConverter2);
			}
			m_pConverter = pConverter2;
			goto convert2;
		}
		ATLASSERT(pConverter);
	}
	m_pConverter = pConverter;
convert2:
	ATLASSERT(m_pConverter);

	if (charaCode == "ISO-2022-JP" || charaCode == "JIS")
		m_bJISCode = true;

	if (m_owner.url.getDebug())
		_firstDebugOutput(charaCode);
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
			if (m_bDataDump == false)
				return ;	// バッファがたまるまで待つ
		}
		// WebFontか判定
		if (m_buffer.size() > 4) {	
			// 'wOFF'
			if (m_buffer[0] == 'w' && m_buffer[1] == 'O' && m_buffer[2] == 'F' && m_buffer[3] == 'F') {
				m_bBinary = true;
				m_bCharaCodeDectated = true;
			}
		}

		if (m_bBinary == false) {
			_decideCharset();
			m_bCharaCodeDectated = true;
		}
	} else {
		m_buffer += m_tailBuffer;
		m_tailBuffer.clear();
		m_buffer += data;
	}

	if (m_bBinary) {
		m_output->DataFeed(m_buffer);
		m_buffer.clear();
		return;
	}

	if (m_bDataDump == false) {		// 通常時
		const char* endPoint = m_buffer.c_str() + m_buffer.size();
		int decrimentCount = _findEndPoint(m_buffer.c_str(), endPoint);
		if (decrimentCount == -1) {
			if (data.size() > 0)
				return ;	// 末尾が見つからなかったら危ないので帰る
			decrimentCount = 0;
		}
		// あまりを保存しておく
		m_tailBuffer.assign(endPoint, decrimentCount);
		auto validBufferSize = endPoint - m_buffer.c_str();
		if (validBufferSize > 0) {
			UErrorCode	err = U_ZERO_ERROR;
			UnicodeString	appendBuff(m_buffer.c_str(), (int32_t)validBufferSize, m_pConverter, err);
			ATLASSERT( U_SUCCESS(err) );
			m_buffer.clear();	 // あまりは保存したのでclearして大丈夫
			m_unicodeBuffer.append(appendBuff);
		}
	} else {	// DataDump時
		UErrorCode	err = U_ZERO_ERROR;
		UnicodeString	appendBuff(m_buffer.c_str(), (int32_t)m_buffer.length(), m_pConverter, err);
		ATLASSERT( U_SUCCESS(err) );
		m_buffer.clear();
		m_unicodeBuffer.append(appendBuff);
	}
	const UChar* bufStart = m_unicodeBuffer.getBuffer();
	int len = m_unicodeBuffer.length();
	const UChar* bufEnd   = bufStart + len;
    const UChar* index    = bufStart;
    const UChar* done     = bufStart;

	std::stringstream out;
	if (m_owner.bypassBody == false) {	// Debug + BypassBodyの時もあるので

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
						if (m_owner.url.getSource()) {
							string buf;
							CUtil::htmlEscape(buf, ConvertFromUTF16(replaceText, m_pConverter));
							out << buf;
						} else {
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
							replaceText.c_str(), (int32_t)replaceText.length());
						len = m_unicodeBuffer.length();
						bufStart = m_unicodeBuffer.getBuffer();
						bufEnd = bufStart + len;
						index = bufStart;
						done = bufStart;
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

	m_bDataDump = true;

	DataFeed("");
	m_output->DataDump();
	m_owner.killed = true;
}


void CTextBuffer::escapeOutput(std::stringstream& out, const UChar *data, size_t len)
{
    if (m_owner.url.getDebug()) {
		std::wstring buf = CUtil::htmlEscape(data, len);
		out << ConvertFromUTF16(buf, m_pConverter);
    } else {
		out << ConvertFromUTF16(data, (int)len, m_pConverter);
    }
}


void CTextBuffer::_firstDebugOutput(const std::string& charaCode)
{
	std::string buf =
		"<!DOCTYPE html>\n"
		"<html>\n<head>\n";
	buf += "<meta charset=\"" + charaCode + "\">\n";
	buf += "<title>Source of ";
	CUtil::htmlEscape(buf, UTF8fromUTF16(m_owner.url.getProtocol()) + "://" + UTF8fromUTF16(m_owner.url.getFromHost()));
	buf += "</title>\n"
		"<link rel=\"stylesheet\" media=\"all\" "
		"href=\"";
	buf += "//local.ptron/ViewSource.css";
	buf += "\" />\n"
		"</head>\n\n<body>\n";
	
	buf += "<div id=\"headers\">\n";
	buf += "<div class=\"res\">";
	CUtil::htmlEscape(buf, m_owner.responseLine.ver);
	buf += " ";
	CUtil::htmlEscape(buf, m_owner.responseLine.code);
	buf += " ";
	CUtil::htmlEscape(buf, m_owner.responseLine.msg);
	buf += "</div>\n";

	buf += "<div class=\"rawInHeaders\">";
	buf += "[raw data]";
	for (auto& pair : m_owner.inHeaders) {
		buf += "<div class=\"hdr\">";
		CUtil::htmlEscape(buf, UTF8fromUTF16(pair.first));
		buf += ": <span class=\"val\">";
		CUtil::htmlEscape(buf, UTF8fromUTF16(pair.second));
		buf += "</span></div>\n";
	}
	buf += "</div>";

	buf += "<div class=\"filteredInHeaders\">";
	buf += "[filtered data]";
	for (auto& pair : m_owner.inHeadersFiltered) {
		buf += "<div class=\"hdr\">";
		CUtil::htmlEscape(buf, UTF8fromUTF16(pair.first));
		buf += ": <span class=\"val\">";
		CUtil::htmlEscape(buf, UTF8fromUTF16(pair.second));
		buf += "</span></div>\n";
	}
	buf += "</div>";

	buf += "</div>";	// id="headers" />
	buf += "<div id=\"body\">\n";

	m_output->DataFeed(buf);
}

