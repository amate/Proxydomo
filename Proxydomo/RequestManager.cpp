/**
*	@file	RequestManager.cpp
*	@brief	ブラウザ⇔プロクシ⇔サーバー間の処理を受け持つ
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
#include "RequestManager.h"
#include <sstream>
#include <limits>
#include <boost\lexical_cast.hpp>
#include "DebugWindow.h"
#include "Log.h"
#include "proximodo\util.h"
#include "Misc.h"
#include "Settings.h"
#include "Node.h"
#include "Matcher.h"
#include "proximodo\filter.h"
#include "Logger.h"
#include "CodeConvert.h"
#include "ConnectionMonitor.h"
using namespace CodeConvert;

#define CR	'\r'
#define LF	'\n'
#define CRLF "\r\n"



/////////////////////////////////////////////////////////////////////////////////////////////////////
// CRequestManager

namespace {

bool	GetHeaders(std::string& buf, HeadPairList& headers, std::string& log)
{
	for (;;) {
		// Look for end of line
		size_t pos, len;
		if (CUtil::endOfLine(buf, 0, pos, len) == false)
			return false;	// 改行がないので帰る

		// Check if we reached the empty line
		if (pos == 0) {
			buf.erase(0, len);
			return true;	// 終了
		}

		// Find the header end
		while (	  pos + len >= buf.size()
				|| buf[pos + len] == ' '
				|| buf[pos + len] == '\t'
				)
		{
			if (CUtil::endOfLine(buf, pos + len, pos, len) == false)
				return false;
		}

		// Record header
		size_t colon = buf.find(':');
		if (colon != std::string::npos) {
			std::wstring name = UTF16fromUTF8(buf.substr(0, colon));
			std::wstring value = UTF16fromUTF8(buf.substr(colon + 1, pos - colon - 1));
			CUtil::trim(value);
			headers.emplace_back(std::move(name), std::move(value));
		}
		log += buf.substr(0, pos + len);
		buf.erase(0, pos + len);
	}
}

}	// namespace

CRequestManager::CRequestManager(std::unique_ptr<CSocket>&& psockBrowser) :
	m_useChain(false),
	m_textFilterChain(m_filterOwner, this),
	m_psockBrowser(std::move(psockBrowser)),
	m_valid(true),
	m_dumped(false),
	m_redirectedIn(0),
	m_outSize(0),
	m_outChunked(false),
	m_inSize(0),
	m_inChunked(false),
	m_RequestCountFromBrowser(0)
{
	m_filterOwner.requestNumber = 0;

	CSettings::EnumActiveFilter([&, this](CFilterDescriptor* filter) {
		try {
			if (filter->filterType == filter->kFilterHeadIn)
				m_vecpInFilter.emplace_back(new CHeaderFilter(*filter, m_filterOwner));
			else if (filter->filterType == filter->kFilterHeadOut)
				m_vecpOutFilter.emplace_back(new CHeaderFilter(*filter, m_filterOwner));
		} catch (...) {
			// Invalid filters are just ignored
		}
	});

	m_connectionData = CConnectionManager::CreateConnectionData();
}


CRequestManager::~CRequestManager(void)
{
    // If we were processing outgoing or incoming
    // data, we were an active request
    if (m_outStep != STEP::STEP_START || m_inStep != STEP::STEP_START) {
		CLog::DecrementActiveRequestCount();
		CLog::ProxyEvent(kLogProxyEndRequest, m_ipFromAddress);
    }

	// Destroy sockets
	try {
		m_psockBrowser->Close();
		m_psockWebsite->Close();
	} catch (SocketException& e) {
		ATLTRACE(e.what());
		ERROR_LOG << L"~CRequestManager : " << e.what();
	}

	CConnectionManager::RemoveConnectionData(m_connectionData);
}

void CRequestManager::Manage()
{
	m_outStep = STEP::STEP_START;
	m_inStep = STEP::STEP_START;
	m_ipFromAddress = m_psockBrowser->GetFromAddress();
	m_psockBrowser->SetBlocking(false);
	m_psockWebsite.reset(new CSocket);

	TRACEIN(_T("Manage start : ポート %d"), m_ipFromAddress.GetPortNumber());
	try {
	// Main loop, continues as long as the browser is connected
	while (m_psockBrowser->IsConnected()) {

		// Process only outgoin data
		bool bRest = true;
		if (_ReceiveOut()) {	// ブラウザからのリクエストを受信
			bRest = false;
			_ProcessOut();		// ブラウザからのリクエストを処理 (サイトに接続)
		}
		if (_SendIn())			// ブラウザへレスポンスを送信
			bRest = false;

		if (m_psockWebsite->IsConnected()) {
			TRACEIN(L"Manage ポート %d : サイトとつながりました！[%s]", 
					m_ipFromAddress.GetPortNumber(), m_filterOwner.contactHost.c_str());
			do {
				// Full processing
				bool bRest = true;
				if (_SendOut())		// サイトへデータを送信
					bRest = false;

				if (_ReceiveIn()) {	// サイトからのデータを受信
					bRest = false;
					_ProcessIn();	// サイトからのデータを処理
				}

				if (_SendIn())		// ブラウザへサイトからのデータを送信
					bRest = false;
				if (_ReceiveOut()) { // ブラウザからのデータを受信
					bRest = false;
					_ProcessOut();
				}

				if (bRest) {
					_JudgeManageContinue();

					::Sleep(10);	// 増やすな！
				}
			} while (m_psockWebsite->IsConnected() && m_psockBrowser->IsConnected());

			TRACEIN(L"Manage ポート %d : outStep[%s] inStep[%s] whileEnd、sitecon(%d) brocon(%d)", 
					m_ipFromAddress.GetPortNumber(), STEPtoString(m_outStep), STEPtoString(m_inStep), m_psockWebsite->IsConnected(), m_psockBrowser->IsConnected());

			// Terminate feeding browser
			_ReceiveIn();
			_ProcessIn();
			if (m_inStep == STEP::STEP_RAW || m_inStep == STEP::STEP_CHUNK) {
				m_inStep = STEP::STEP_FINISH;
				m_connectionData->SetInStep(m_inStep);
				_EndFeeding();
				_ProcessIn();
			}
			_SendIn();
			if (m_outStep == STEP::STEP_FINISH) {
				// The website aborted connection before sending the body
				m_sendInBuf = "HTTP/1.1 503 Service Unavailable" CRLF;
                //CLog::ref().logHttpEvent(pmEVT_HTTP_TYPE_SENDIN, addr,
                //                         reqNumber, sendInBuf);
                m_sendInBuf += CRLF;
                _SendIn();
                m_psockBrowser->Close();
			}
		}

		if (bRest) {
			_JudgeManageContinue();

			::Sleep(10);
		}
	}	// while
	} catch (std::exception& e) {
		TRACEIN("例外が発生しました！ : ポート %d 例外:%s", m_ipFromAddress.GetPortNumber(), e.what()); 
		e;
	} catch (...) {
		TRACEIN("例外が発生しました！ : ポート %d", m_ipFromAddress.GetPortNumber());
	}
	TRACEIN(_T("Manage finish : ポート %d"), m_ipFromAddress.GetPortNumber());
}


void	CRequestManager::SwitchToInvalid() 
{
	m_valid = false;
	if (m_outStep != STEP::STEP_START) {
		if (m_psockWebsite)
			m_psockWebsite->WriteStop();
		if (m_psockBrowser)
			m_psockBrowser->WriteStop();
		if (m_pSSLServerSession)
			m_pSSLServerSession->WriteStop();
		if (m_pSSLClientSession)
			m_pSSLClientSession->WriteStop();
	}
}


void	CRequestManager::_JudgeManageContinue()
{
	if (m_valid == false) {	// 接続を強制終了させる
		if (m_pSSLServerSession) {
			m_pSSLServerSession->Close();
		}
		if (m_pSSLClientSession) {
			m_pSSLClientSession->Close();
		}
		m_psockWebsite->Close();
		m_psockBrowser->Close();
		return;
	}

	// SSL
	if (m_pSSLServerSession && m_outStep == STEP::STEP_START) {	
		bool browserConnected = true;
		if (m_filterOwner.url.getHttps()) {
			browserConnected = m_psockBrowser->IsConnected();
		} else {
			browserConnected = m_pSSLClientSession->IsConnected();
		}
		if (m_pSSLServerSession->IsConnected() == false || browserConnected == false ) {
			INFO_LOG << L"#" << m_ipFromAddress.GetPortNumber()
				<< L" SSL すべての STEP が START なので接続を切ります。"
				<< L" svcon:" << m_pSSLServerSession->IsConnected()
				<< L" clcon:" << browserConnected;
			m_pSSLServerSession->Close();
			if (m_filterOwner.url.getHttps()) {
				m_psockBrowser->Close();
			} else {
				m_pSSLClientSession->Close();
			}
			return;
		}
	}

	if (m_pSSLServerSession && m_filterOwner.responseLine.code != "101")
		return;

	if (m_outStep == STEP::STEP_TUNNELING) {
		if (m_psockBrowser->IsConnected() == false || m_psockWebsite->IsConnected() == false) {
			m_psockBrowser->Close();
			m_psockWebsite->Close();
		}
		return;
	}

	using std::chrono::steady_clock;
	typedef steady_clock::time_point time_point;

	// 規定秒(10秒)処理の開始待ち時間が経過すると切断する
	if (m_outStep != STEP::STEP_START) {
		if (m_processIdleTime != time_point())
			m_processIdleTime = time_point();		// 処理が行われてるので時間をリセットする

	} else {	// m_outStep == STE_START
		if (m_processIdleTime == time_point()) {
			m_processIdleTime = steady_clock::now();
		} else if ((steady_clock::now() - m_processIdleTime) > std::chrono::seconds(10)) {
			m_psockBrowser->Close();
		}
	}
}


bool CRequestManager::_ReceiveOut()
{
	bool bDataReceived = false;
	if (m_pSSLClientSession) {
		while (m_pSSLClientSession->Read(m_readBuffer, kReadBuffSize)) {
			int count = m_pSSLClientSession->GetLastReadCount();
			if (count == 0)
				break;
			m_recvOutBuf.append(m_readBuffer, count);
			bDataReceived = true;
		}
	} else {
		while (m_psockBrowser->IsDataAvailable() && m_psockBrowser->Read(m_readBuffer, kReadBuffSize)) {
			int count = m_psockBrowser->GetLastReadCount();
			if (count == 0)
				break;
			m_recvOutBuf.append(m_readBuffer, count);
			bDataReceived = true;
		}
	}
	return bDataReceived;
}

/// Send outgoing data to website
bool CRequestManager::_SendOut()
{
	if (m_sendOutBuf.empty())
		return false;

	bool ret = false;
	if (m_pSSLServerSession) {
		if (m_pSSLServerSession->Write(m_sendOutBuf.c_str(), (int)m_sendOutBuf.size())) {
			m_sendOutBuf.erase(0, m_pSSLServerSession->GetLastWriteCount());
			ret = true;
		}
	} else {
		// Send everything to website, if connected
		if (m_psockWebsite->IsConnected()) {			
			if (m_psockWebsite->Write(m_sendOutBuf.c_str(), (int)m_sendOutBuf.size())) {
				m_sendOutBuf.erase(0, m_psockWebsite->GetLastWriteCount());
				ret = true;
			}
		}
	}
	return ret;
}

/// Process outgoing data (from browser to website)
/// ブラウザ ⇒ Proxy(this) ⇒ サイト
void CRequestManager::_ProcessOut()
{
	// We will exxet from a step that has not enough data to complete
	for (;;) {
		switch (m_outStep) {
		// Marks the beginning of a request
		case STEP::STEP_START:
			{
				if (m_recvOutBuf.empty())
					return ;			// 処理するのに十分なデータがないので帰る
			
				m_outStep = STEP::STEP_FIRSTLINE;
				m_connectionData->SetOutStep(m_outStep);

				// Update active request stat, but if processIn() did it already
				if (m_inStep == STEP::STEP_START) {
					CLog::IncrementActiveRequestCount();
					CLog::ProxyEvent(kLogProxyNewRequest, m_ipFromAddress);
				}

				// Reset variables relative to a single request/response
				m_filterOwner.Reset();
				m_filterOwner.requestNumber = CLog::IncrementRequestCount();

				m_redirectedIn = 0;
				m_recvConnectionClose = false;
				m_sendConnectionClose = false;
				m_recvContentCoding = 0;
				m_bPostData = false;
				m_logPostData.clear();

				++m_RequestCountFromBrowser;
			}
			break;

		// Read first HTTP request line
		case STEP::STEP_FIRSTLINE:
			{
				// Do we have the full first line yet?
				size_t pos, len;
				if (CUtil::endOfLine(m_recvOutBuf, 0, pos, len) == false)
					return;				// 最初の改行まで来てないので帰る

				// Get it and record it
				size_t p1 = m_recvOutBuf.find_first_of(" ");
				size_t p2 = m_recvOutBuf.find_first_of(" ", p1+1);
				m_requestLine.method = m_recvOutBuf.substr(0,    p1);
				m_requestLine.url    = m_recvOutBuf.substr(p1+1, p2 -p1-1);
				m_requestLine.ver    = m_recvOutBuf.substr(p2+1, pos-p2-1);
				m_logRequest = m_recvOutBuf.substr(0, pos + len);
				m_recvOutBuf.erase(0, pos + len);

				m_connectionData->SetVerb(UTF16fromUTF8(m_requestLine.method));

				// Next step will be to read headers
				m_outStep = STEP::STEP_HEADERS;
				m_connectionData->SetOutStep(m_outStep);

				// Unless we have Content-Length or Transfer-Encoding headers,
				// we won't expect anything after headers.
				m_outSize = 0;
				m_outChunked = false;
			}
			break;

		// Read and process headers, as long as there are any
		case STEP::STEP_HEADERS:
			{
				if (GetHeaders(m_recvOutBuf, m_filterOwner.outHeaders, m_logRequest) == false)
					return ;
				
				m_outStep = STEP::STEP_DECODE;
				m_connectionData->SetOutStep(m_outStep);
		}
			break;

		// Decode and process headers
		case STEP::STEP_DECODE:
			{
				CLog::HttpEvent(kLogHttpRecvOut, m_ipFromAddress, m_filterOwner.requestNumber, m_logRequest);

				// Get the URL and the host to contact (unless we use a proxy)
				if (m_pSSLClientSession) {
					if (m_requestLine.url.length() > 0 && m_requestLine.url[0] == L'/') {
						std::wstring sslurl = L"https://" + m_filterOwner.url.getHost() + UTF16fromUTF8(m_requestLine.url);
						m_filterOwner.url.parseUrl(sslurl);
					} else {
						ATLASSERT(::strncmp(m_requestLine.url.c_str(), "https://", 0) == 0);
						m_filterOwner.url.parseUrl(UTF16fromUTF8(m_requestLine.url));
					}
				} else if (m_requestLine.method == "CONNECT") {
					m_filterOwner.url.parseUrl(L"https://" + UTF16fromUTF8(m_requestLine.url) + L"/");
				} else {
					m_filterOwner.url.parseUrl(UTF16fromUTF8(m_requestLine.url));
				}
				m_connectionData->SetUrl(m_filterOwner.url.getUrl());

				CLog::HttpEvent(kLogHttpNewRequest, m_ipFromAddress, m_filterOwner.requestNumber, UTF8fromUTF16(m_filterOwner.url.getUrl()));

				if (m_filterOwner.url.getBypassIn())
					m_filterOwner.bypassIn = true;
				if (m_filterOwner.url.getBypassOut())
					m_filterOwner.bypassOut = true;
				if (m_filterOwner.url.getBypassText())
					m_filterOwner.bypassBody = true;

				m_filterOwner.useSettingsProxy = CSettings::s_useRemoteProxy;
				m_filterOwner.contactHost = m_filterOwner.url.getHostPort();

				// Test URL with bypass-URL matcher, if matches we'll bypass all
				{
					m_bypass = false;
					CFilter filter(m_filterOwner);
					std::wstring urlFromHost = m_filterOwner.url.getFromHost();
					if (CSettings::s_bypass || CSettings::s_pBypassMatcher->match(urlFromHost, &filter)) {
						m_bypass = true;
						m_filterOwner.bypassOut = m_filterOwner.bypassIn = m_filterOwner.bypassBody = true;
					}
				}

				// We must read the Content-Length and Transfer-Encoding
				// headers to know if thre is POSTed data
				if (CUtil::noCaseContains(L"chunked", m_filterOwner.GetOutHeader(L"Transfer-Encoding")))
					m_outChunked = true;

				std::wstring contentLength = m_filterOwner.GetOutHeader(L"Content-Length");
				if (contentLength.size() > 0)
					m_outSize = boost::lexical_cast<int64_t>(contentLength);

				std::wstring contentType = m_filterOwner.GetOutHeader(L"Content-Type");
				if (CUtil::noCaseContains(L"application/x-www-form-urlencoded", contentType)
					|| CUtil::noCaseContains(L"text/xml", contentType)) 
				{
					m_bPostData = true;
				}

				// We'll work on a copy, since we don't want to alter
				// the real headers that $IHDR and $OHDR may access
				m_filterOwner.outHeadersFiltered = m_filterOwner.outHeaders;

				// Filter outgoing headers
				if (m_filterOwner.bypassOut == false && CSettings::s_filterOut &&
					m_filterOwner.url.getHost() != L"local.ptron") {
					// Apply filters one by one
					for (auto& headerfilter : m_vecpOutFilter) {
						headerfilter->bypassed = false;
						const wstring& name = headerfilter->headerName;

						if (CUtil::noCaseBeginsWith(L"url", name) == false) {

							// If header is absent, temporarily create one
							if (CFilterOwner::GetHeader(m_filterOwner.outHeadersFiltered, name).empty())
								CFilterOwner::SetHeader(m_filterOwner.outHeadersFiltered, name, L"");
							// Test headers one by one
							for (auto& pair : m_filterOwner.outHeadersFiltered) {
								if (CUtil::noCaseEqual(name, pair.first))
									headerfilter->filter(pair.second);
							}
							// Remove null headers
							CFilterOwner::CleanHeader(m_filterOwner.outHeadersFiltered);

						} else {

							// filter works on a copy of the URL
							std::wstring test = m_filterOwner.url.getUrl();
							headerfilter->filter(test);
							CUtil::trim(test);
							// if filter changed the url, update variables
							if (!m_filterOwner.killed && !test.empty() && test != m_filterOwner.url.getUrl()) {
								// We won't change contactHost if it has been
								// set to a proxy address by a $SETPROXY command
								bool changeHost = (m_filterOwner.contactHost == m_filterOwner.url.getHostPort());
								// update URL
								m_filterOwner.url.parseUrl(test);
								if (m_filterOwner.url.getBypassIn())    m_filterOwner.bypassIn   = true;
								if (m_filterOwner.url.getBypassOut())   m_filterOwner.bypassOut  = true;
								if (m_filterOwner.url.getBypassText())  m_filterOwner.bypassBody = true;
								if (changeHost) m_filterOwner.contactHost = m_filterOwner.url.getHostPort();
							}
						}
						if (m_filterOwner.rdirToHost.size() > 0 && m_filterOwner.rdirMode == 1) {
							if (CUrl(m_filterOwner.rdirToHost).getBypassOut())
								break;
						}

						if (m_filterOwner.killed) {
							// There has been a \k in a header filter, so we
							// redirect to an empty file and stop processing headers
							if (m_filterOwner.url.getPath().find(L".gif") != wstring::npos)
								m_filterOwner.rdirToHost = L"http://file//./html/killed.gif";
							else
								m_filterOwner.rdirToHost = L"http://file//./html/killed.html";
							break;
						}
					}
				}

				// CONNECTリクエストで$FILTER(false)された場合はバイパス扱いする
				if (m_requestLine.method == "CONNECT" && m_filterOwner.bypassBody) {
					m_bypass = true;
				}

				// CONNECTリクエストはリダイレクトしないようにする
				if (m_requestLine.method == "CONNECT" && m_filterOwner.killed == false && m_filterOwner.rdirToHost.size() > 0) {
					WARN_LOG << L"CONNECT redirect clear, src : " << m_filterOwner.url.getHost() 
							<< L" rdirToHost : " << m_filterOwner.rdirToHost << L" rdirMode : " << m_filterOwner.rdirMode;
					m_filterOwner.rdirToHost.clear();
					m_filterOwner.rdirMode = 0;
				}

				// If we haven't connected to ths host yet, we do it now.
				// This will allow incoming processing to start. If the
				// connection fails, incoming processing will jump to
				// the finish state, so that we can accept other requests
				// on the same browser socket (persistent connection).
				if (m_pSSLServerSession && m_filterOwner.killed == false) {
					m_inStep = STEP::STEP_START;
					m_connectionData->SetInStep(m_inStep);

					// $SETPROXYされた場合、CONNECTリクエストしなおさないといけない場合がある
					if (m_previousHost != m_filterOwner.contactHost || m_filterOwner.rdirToHost.length() > 0) {
						_ConnectWebsite();
					}

				} else {
					_ConnectWebsite();
				}

				// If website is read, send headers
				if (m_inStep == STEP::STEP_START) {

					// Update URL within request
					// おそらく送信ヘッダフィルターで、URL:を使用してURLが書き換えられた時にHostも変更するようにしているんだろう
					CFilterOwner::SetHeader(m_filterOwner.outHeadersFiltered, L"Host", m_filterOwner.url.getHost());

					if (CUtil::noCaseContains(L"Keep-Alive", CFilterOwner::GetHeader(m_filterOwner.outHeadersFiltered, L"Proxy-Connection"))) {
						CFilterOwner::RemoveHeader(m_filterOwner.outHeadersFiltered, L"Proxy-Connection");
						CFilterOwner::SetHeader(m_filterOwner.outHeadersFiltered, L"Connection", L"Keep-Alive");
					}
				
					if (m_filterOwner.contactHost == m_filterOwner.url.getHostPort()) {
						m_requestLine.url = UTF8fromUTF16(m_filterOwner.url.getAfterHost());
					} else {
						m_requestLine.url = UTF8fromUTF16(m_filterOwner.url.getUrl());
					}
					if (m_requestLine.url.empty())
						m_requestLine.url = "/";

					// Now we can put everything in the filtered buffer
					m_sendOutBuf = m_requestLine.method + " " + m_requestLine.url + " " + m_requestLine.ver + CRLF;
					
					// デバッグ有効時、"304 Not Modified"が帰ってこないようにする
					if (m_filterOwner.url.getDebug()) {
						CFilterOwner::RemoveHeader(m_filterOwner.outHeadersFiltered, L"If-Modified-Since");
						CFilterOwner::RemoveHeader(m_filterOwner.outHeadersFiltered, L"If-None-Match");
					}

					for (auto& pair : m_filterOwner.outHeadersFiltered)
						m_sendOutBuf += UTF8fromUTF16(pair.first) + ": " + UTF8fromUTF16(pair.second) + CRLF;

					// Log outgoing headers
					CLog::HttpEvent(kLogHttpSendOut, m_ipFromAddress, m_filterOwner.requestNumber, m_sendOutBuf);

					m_sendOutBuf += CRLF;
				}

				// Decide next step
				if (m_requestLine.method == "HEAD") {
					// HEAD requests have no body, even if a size is provided
					m_outStep = STEP::STEP_FINISH;
					m_connectionData->SetOutStep(m_outStep);
				} else if (m_requestLine.method == "CONNECT") {
					// SSL Tunneling (we won't process exchanged data)
					m_outStep = STEP::STEP_TUNNELING;
					m_connectionData->SetOutStep(m_outStep);
					if (m_pSSLServerSession)
						return;
				} else {
					m_outStep = (m_outChunked ? STEP::STEP_CHUNK : m_outSize ? STEP::STEP_RAW : STEP::STEP_FINISH);
					m_connectionData->SetOutStep(m_outStep);
				}
			}
			break;

		// Read CRLF HexRawLength * CRLF before raw data
		// or CRLF zero * CRLF CRLF
		case STEP::STEP_CHUNK:
			{
				TRACEIN(_T("ProcessOut #%d : STEP::STEP_CHUNK"), m_filterOwner.requestNumber);
				size_t pos, len;
				while (CUtil::endOfLine(m_recvOutBuf, 0, pos, len) && pos == 0) {
					m_sendOutBuf += CRLF;
					m_recvOutBuf.erase(0, len);
				}

				if (CUtil::endOfLine(m_recvOutBuf, 0, pos, len) == false)
					return ;
				m_outSize = CUtil::readHex(m_recvOutBuf.substr(0, pos));
				if (m_outSize > 0) {
					m_sendOutBuf += m_recvOutBuf.substr(0, pos) + CRLF;
					m_recvOutBuf.erase(0, pos + len);
					m_outStep = STEP::STEP_RAW;
					m_connectionData->SetOutStep(m_outStep);
				} else {
					if (CUtil::endOfLine(m_recvOutBuf, 0, pos, len, 2) == false)
						return ;
					m_sendOutBuf += m_recvOutBuf.substr(0, pos) + CRLF CRLF;
					m_recvOutBuf.erase(0, pos + len);

					if (m_bPostData) {
						CLog::HttpEvent(kLogHttpPostOut, m_ipFromAddress, m_filterOwner.requestNumber, m_logPostData);
					}

					m_outStep = STEP::STEP_FINISH;
					m_connectionData->SetOutStep(m_outStep);

					// We shouldn't send anything to website if it is not expecting data
					// (i.e we obtained a faked response)
					if (m_inStep == STEP::STEP_FINISH)
						m_sendOutBuf.clear();
				}
			}
			break;

		// The next outSize bytes are left untouched
		case STEP::STEP_RAW:
			{
				TRACEIN(_T("ProcessOut #%d : STEP::STEP_RAW"), m_filterOwner.requestNumber);
				int copySize = (int)m_recvOutBuf.size();
				if (copySize > m_outSize)
					copySize = (int)m_outSize;
				if (copySize == 0 && m_outSize)
					return ;

				if (copySize == m_recvOutBuf.size() && m_sendOutBuf.empty()) {
					m_sendOutBuf = std::move(m_recvOutBuf);
					m_recvOutBuf.reserve(m_sendOutBuf.capacity());
				} else {
					m_sendOutBuf.append(m_recvOutBuf.c_str(), copySize);
					m_recvOutBuf.erase(0, copySize);
				}
				//std::string postData = m_recvOutBuf.substr(0, copySize);
				//m_sendOutBuf += postData;
				//m_logPostData += postData;
				if (m_bPostData) {
					m_logPostData += m_sendOutBuf.substr(m_sendOutBuf.size() - copySize);
				}
				m_outSize -= copySize;

				// If we finished reading as mush raw data as was expected,
				// continue to next step (finish or next chunk)
				if (m_outSize == 0) {
					if (m_outChunked) {
						m_outStep = STEP::STEP_CHUNK;
						m_connectionData->SetOutStep(m_outStep);
					} else {
						if (m_bPostData) {
							CLog::HttpEvent(kLogHttpPostOut, m_ipFromAddress, m_filterOwner.requestNumber, m_logPostData);
						}

						m_outStep = STEP::STEP_FINISH;
						m_connectionData->SetOutStep(m_outStep);
					}
				}

				// We shouldn't send anything to website if it is not expecting data
				// (i.e we obtained a faked response)
				if (m_inStep == STEP::STEP_FINISH)
					m_sendOutBuf.clear();
			}
			break;

		// Foward outgoing data to website
		case STEP::STEP_TUNNELING:
			{
				if (m_pSSLClientSession && m_filterOwner.responseLine.code != "101") {
					m_outStep = STEP::STEP_START;
					m_connectionData->SetOutStep(m_outStep);
					continue;
				}

				m_sendOutBuf += m_recvOutBuf;
				m_recvOutBuf.clear();
				return ;
			}
			break;

		// We'll wait for response completion
		case STEP::STEP_FINISH:
			{
				// サイトからのデータを処理中ならここでは終了処理を行わない
				// _ProcessIn で終了処理が行われる
				if (m_inStep != STEP::STEP_FINISH)
					return ;

				m_outStep = STEP::STEP_START;
				m_connectionData->SetOutStep(m_outStep);
				m_inStep = STEP::STEP_START;
				m_connectionData->SetInStep(m_inStep);

				// Update active request stat
				CLog::DecrementActiveRequestCount();
				CLog::ProxyEvent(kLogProxyEndRequest, m_ipFromAddress);

				// Disconnect browser if we send it "Connection:close"
				// If manager has been set invalid, close connection too
				if (m_sendConnectionClose) {
					_SendIn();
					m_psockBrowser->Close();
				}

				// Disconnect website if it send us "Connection:close"
				if (m_recvConnectionClose || m_psockBrowser->IsConnected() == false)
					m_psockWebsite->Close();
			}
			break;
		}

	}
}


void CRequestManager::_ConnectWebsite()
{
	// If download was on, disconnect (to avoid downloading genuine document)
	// 受信ヘッダフィルターでリダイレクトを指示された場合
	if (m_inStep == STEP::STEP_DECODE) {
		if (m_pSSLServerSession) {
			m_pSSLServerSession.reset();
		} else {
			m_psockWebsite->Close();
		}
		m_inStep = STEP::STEP_START;
		m_connectionData->SetInStep(m_inStep);
		m_filterOwner.bypassBody = m_filterOwner.bypassBodyForced = false;
	}

    _ReceiveIn();
    m_recvInBuf.clear();
    m_sendInBuf.clear();

    // Test for "local.ptron" host
	if (_HandleLocalPtron())
		return;

	// Test for transparent redirection to URL ($RDIR)
	// Test for non-transparent redirection ($JUMP)
	if (_HandleRedirectToHost())
		return;

    // If we must contact the host via the settings' proxy,
    // we now override the contactHost
	if (m_filterOwner.useSettingsProxy && CSettings::s_defaultRemoteProxy.length() > 0) {
		m_filterOwner.contactHost = UTF16fromUTF8(CSettings::s_defaultRemoteProxy);
    }

	// The host string is composed of host and port
	std::string name = UTF8fromUTF16(m_filterOwner.contactHost);
	std::string port;
	size_t colon = name.find(':');
	if (colon != std::string::npos) {    // (this should always happen)
		port = name.substr(colon + 1);
		name = name.substr(0, colon);
	}
	if (port.empty())
		port = "80";

    // Unless we are already connected to this host, we try and connect now
	if (m_previousHost != m_filterOwner.contactHost || 
		m_psockWebsite->IsConnected() == false ) {

        // Disconnect from previous host
		if (m_pSSLServerSession) {
			m_pSSLServerSession.reset();
		} else {
			m_psockWebsite->Close();
		}
        m_previousHost = m_filterOwner.contactHost;

        // Check the host (Hostname() asks the DNS)
		IPv4Address host;
		if (name.empty() || host.SetService(port) == false || host.SetHostName(name) == false) {
            // The host address is invalid (or unknown by DNS)
            // so we won't try a connection.
			_FakeResponse("502 Bad Gateway", L"./html/error.html");
            return ;
		}

        // Connect
		do {
			if (m_psockWebsite->Connect(host))
				break;
		} while (host.SetNextHost());

		if (m_psockWebsite->IsConnected() == false) {
            // Connection failed, warn the browser
			_FakeResponse("503 Service Unavailable", L"./html/error.html");
            return ;
        }

		if (m_requestLine.method == "CONNECT") {
			// for CONNECT method, incoming data is encrypted from start
			m_sendInBuf = "HTTP/1.0 200 Connection established" CRLF
				"Proxy-agent: " "Proxydomo/1.0"/*APP_NAME " " APP_VERSION*/ CRLF CRLF;
			CLog::HttpEvent(kLogHttpSendIn, m_ipFromAddress, m_filterOwner.requestNumber, m_sendInBuf);
			m_inStep = STEP::STEP_TUNNELING;
			m_connectionData->SetInStep(m_inStep);
			_SendIn();

			// CONNECTリクエストのやり直し用に保存しておく
			m_previousConnectRequest = m_logRequest + CRLF;
		}

		// remote proxy
		_SendConnectRequestToRemoteProxy(name);

		if (m_filterOwner.url.getProtocol() == L"https") {
			if (CSettings::s_SSLFilter && m_bypass == false) {
				if (m_pSSLServerSession = CSSLSession::InitClientSession(m_psockWebsite.get(), name, m_psockBrowser.get())) {
					if (m_requestLine.method == "CONNECT") {
						m_pSSLClientSession = CSSLSession::InitServerSession(m_psockBrowser.get(), name);
					} else {
						// リダイレクト or URLコマンド[https.]
						ATLASSERT(m_pSSLClientSession || m_filterOwner.url.getHttps());
					}
					// ブラウザとのTLS接続が確立できなければ、リクエストを終了させる
					if (m_pSSLClientSession == nullptr && m_filterOwner.url.getHttps() == false) {
						m_pSSLServerSession.reset();
						throw GeneralException("CSSLSession::InitServerSession(m_psockBrowser.get()) failed");
					}
				}
			}
		} else {
			m_psockWebsite->SetBlocking(false);	// not CONNECT
		}
    }

    // Prepare and send a simple request, if we did a transparent redirection
    if (m_outStep != STEP::STEP_DECODE && m_inStep == STEP::STEP_START) {
        if (m_filterOwner.contactHost == m_filterOwner.url.getHostPort())
            m_requestLine.url = UTF8fromUTF16(m_filterOwner.url.getAfterHost());
        else
			m_requestLine.url = UTF8fromUTF16(m_filterOwner.url.getUrl());
        if (m_requestLine.url.empty()) 
			m_requestLine.url = "/";

        m_sendOutBuf = m_requestLine.method + " " + m_requestLine.url + " HTTP/1.1" CRLF
					   "Host: " + UTF8fromUTF16(m_filterOwner.url.getHost()) + CRLF;

		CLog::HttpEvent(kLogHttpSendOut, m_ipFromAddress, m_filterOwner.requestNumber, m_sendOutBuf);

        m_sendOutBuf += m_logPostData + CRLF;
        // Send request
        _SendOut();
    }
}


bool	CRequestManager::_HandleLocalPtron()
{
	if (m_filterOwner.url.getHost() == L"local.ptron") {
		m_filterOwner.rdirToHost = m_filterOwner.url.getUrl();
	}
	if (CUtil::noCaseBeginsWith(L"http://local.ptron", m_filterOwner.rdirToHost)) {
		m_filterOwner.rdirToHost = L"http://file//./html" + CUrl(m_filterOwner.rdirToHost).getPath();
	}

	// https://local.ptron/ への接続
	if (CSettings::s_SSLFilter && CUtil::noCaseBeginsWith(L"https://local.ptron", m_filterOwner.rdirToHost)) {
		wstring subpath;
		if (m_requestLine.method == "CONNECT") {
			m_sendInBuf = "HTTP/1.0 200 Connection established" CRLF
				"Proxy-agent: " "Proxydomo/1.0"/*APP_NAME " " APP_VERSION*/ CRLF CRLF;
			CLog::HttpEvent(kLogHttpSendIn, m_ipFromAddress, m_filterOwner.requestNumber, m_sendInBuf);
			_SendIn();
			m_pSSLClientSession = CSSLSession::InitServerSession(m_psockBrowser.get(), "local.ptron");

			if (m_pSSLClientSession == nullptr) {
				throw GeneralException("LocalSSLServer handshake failed");
			}

			// ブラウザからリクエストURLを取得する
			auto findGetRequestLineURL = [this]() -> bool {
				// Do we have the full first line yet?
				size_t pos, len;
				if (CUtil::endOfLine(m_recvOutBuf, 0, pos, len) == false)
					return false;				// 最初の改行まで来てないので帰る

												// Get it and record it
				size_t p1 = m_recvOutBuf.find_first_of(" ");
				ATLASSERT(p1 != std::string::npos);
				size_t p2 = m_recvOutBuf.find_first_of(" ", p1 + 1);
				ATLASSERT(p2 != std::string::npos);
				m_requestLine.method = m_recvOutBuf.substr(0, p1);
				m_requestLine.url = m_recvOutBuf.substr(p1 + 1, p2 - p1 - 1);
				m_requestLine.ver = m_recvOutBuf.substr(p2 + 1, pos - p2 - 1);
				m_logRequest = m_recvOutBuf.substr(0, pos + len);
				m_recvOutBuf.erase(0, pos + len);
				return true;
			};
			for (; m_pSSLClientSession->IsConnected();) {
				if (_ReceiveOut()) {
					if (findGetRequestLineURL())
						break;
				}
				::Sleep(10);
			}
			m_recvOutBuf.clear();

			subpath = L"./html" + CUrl(UTF16fromUTF8(m_requestLine.url)).getPath();

		} else {
			// リダイレクト
			ATLASSERT(m_pSSLClientSession);
			if (m_pSSLClientSession == nullptr) {
				throw GeneralException("LocalSSLServer handshake failed");
			}

			subpath = L"./html" + CUrl(m_filterOwner.rdirToHost).getPath();
		}

		if (ManageCertificateAPI(m_requestLine.url, m_pSSLClientSession.get())) {
			return true;
		}

		wstring filename = CUtil::makePath(subpath);
		if (::PathFileExists(Misc::GetFullPath_ForExe(filename.c_str()))) {
			_FakeResponse("200 OK", filename);
		} else {
			_FakeResponse("404 Not Found");
		}
		while (_SendIn());	// 最後まで送信してしまう

		m_filterOwner.rdirToHost.clear();
		return true;
	}

	// Test for redirection __to file__
	// ($JUMP to file will behave like a transparent redirection,
	// since the browser may not be on the same file system)
	if (CUtil::noCaseBeginsWith(L"http://file//", m_filterOwner.rdirToHost)) {

		wstring filename = CUtil::makePath(m_filterOwner.rdirToHost.substr(13));
		if (::PathFileExists(Misc::GetFullPath_ForExe(filename.c_str()))) {
			_FakeResponse("200 OK", filename);
		} else {
			_FakeResponse("404 Not Found");
		}
		while (_SendIn());	// 最後まで送信してしまう

		m_filterOwner.rdirToHost.clear();
		return true;
	}

	return false;
}

bool	CRequestManager::_HandleRedirectToHost()
{
	// Test for non-transparent redirection ($JUMP)
	if (m_filterOwner.rdirToHost.size() > 0 && m_filterOwner.rdirMode == 0) {
		// We'll keep browser's socket, for persistent connections, and
		// continue processing outgoing data (which will not be moved to
		// send buffer).
		m_inStep = STEP::STEP_FINISH;
		m_connectionData->SetInStep(m_inStep);
		m_sendInBuf =
			"HTTP/1.0 302 Found" CRLF
			"Location: " + UTF8fromUTF16(m_filterOwner.rdirToHost) + CRLF;

		CLog::HttpEvent(kLogHttpSendIn, m_ipFromAddress, m_filterOwner.requestNumber, m_sendInBuf);

		m_sendInBuf += CRLF;
		m_filterOwner.rdirToHost.clear();
		m_sendConnectionClose = true;
		return true;
	}

	// Test for transparent redirection to URL ($RDIR)
	// Note: new URL will not go through URL* OUT filters
	if (m_filterOwner.rdirToHost.size() > 0 && m_filterOwner.rdirMode == 1) {

		// Change URL
		m_filterOwner.url.parseUrl(m_filterOwner.rdirToHost);
		m_connectionData->SetUrl(m_filterOwner.url.getUrl());
		if (m_filterOwner.url.getBypassIn())    m_filterOwner.bypassIn = true;
		if (m_filterOwner.url.getBypassOut())	m_filterOwner.bypassOut = true;
		if (m_filterOwner.url.getBypassText())  m_filterOwner.bypassBody = true;
		m_filterOwner.useSettingsProxy = CSettings::s_useRemoteProxy;
		m_filterOwner.contactHost = m_filterOwner.url.getHostPort();
		m_filterOwner.SetOutHeader(L"Host", m_filterOwner.url.getHost());

		m_filterOwner.rdirToHost.clear();
	}
	return false;
}

void	CRequestManager::_SendConnectRequestToRemoteProxy(std::string& name)
{
	//ATLASSERT(m_requestLine.method == "CONNECT");
	if (m_filterOwner.contactHost == m_filterOwner.url.getHostPort())
		return ;	// proxy接続ではない

	// nameを実際の接続ホスト名へ変更	
	name = UTF8fromUTF16(m_filterOwner.url.getHost());
	size_t colon = name.find(':');
	if (colon != std::string::npos) {    // (this should always happen)
		name = name.substr(0, colon);
	}

	// Proxyへ CONNECTリクエストを送信
	if (m_requestLine.method == "CONNECT") {
		m_sendOutBuf = m_logRequest + CRLF;
	} else {
		if (m_previousConnectRequest.empty())
			return;		// この接続はCONNECTリクエストから始まらなかった
		m_sendOutBuf = m_previousConnectRequest;
	}
	_SendOut();

	auto fucnGetResponse = [this]() -> bool {
		// Do we have the full first line yet?
		size_t pos, len;
		if (CUtil::endOfLine(m_recvInBuf, 0, pos, len) == false)
			return false;				// 最初の改行まで来てないので帰る

		// Parse it
		size_t p1 = m_recvInBuf.find_first_of(" ");
		ATLASSERT(p1 != std::string::npos);
		size_t p2 = m_recvInBuf.find_first_of(" ", p1 + 1);
		ATLASSERT(p2 != std::string::npos);
		m_filterOwner.responseLine.ver = m_recvInBuf.substr(0, p1);
		m_filterOwner.responseLine.code = m_recvInBuf.substr(p1 + 1, p2 - p1 - 1);
		m_filterOwner.responseLine.msg = m_recvInBuf.substr(p2 + 1, pos - p2 - 1);
		m_filterOwner.responseCode = m_recvInBuf.substr(p1 + 1, pos - p1 - 1);

		m_recvInBuf.erase(0, pos + len);
		return true;
	};

	for (; m_psockWebsite->IsConnected();) {
		if (_ReceiveIn()) {
			if (fucnGetResponse())
				break;
		}
		::Sleep(10);
	}
	m_recvInBuf.clear();

	if (m_filterOwner.responseLine.code != "200") {
		WARN_LOG << L"RemoteProxy SSL Connection Establish failed, proxy : " << m_filterOwner.contactHost << L" ConnectHost : " << name << L" ResponseCode : " << m_filterOwner.responseCode;
		throw GeneralException("RemoteProxy SSL Connection Establish failed");
	}
}


/// サイト ⇒ Proxy(this)
bool	CRequestManager::_ReceiveIn()
{
	bool bDataReceived = false;
	if (m_pSSLServerSession) {
		while (m_pSSLServerSession->Read(m_readBuffer, kReadBuffSize)) {
			int count = m_pSSLServerSession->GetLastReadCount();
			if (count == 0)
				break;
			m_recvInBuf.append(m_readBuffer, count);
			bDataReceived = true;
		}
	} else {
		while (m_psockWebsite->IsDataAvailable() && m_psockWebsite->Read(m_readBuffer, kReadBuffSize)) {
			int count = m_psockWebsite->GetLastReadCount();
			if (count == 0)
				break;
			m_recvInBuf.append(m_readBuffer, count);
			bDataReceived = true;
		}
	}
	return bDataReceived;
}

/// Process incoming data (from website to browser) 
/// サイト ⇒ Proxy(this) ⇒ ブラウザ
void	CRequestManager::_ProcessIn()
{
	// We will exit from a step that has not enough data to complete
	for (;;) {

		switch (m_inStep) {

		// Marks the beginning of a request
		case STEP::STEP_START:
			{
				// We need something in the buffer
				if (m_recvInBuf.empty())
					return ;	// 処理するバッファが無いので帰る

				m_inStep = STEP::STEP_FIRSTLINE;
				m_connectionData->SetInStep(m_inStep);
				// Update active request stat, but if processOut() did it already
				if (m_outStep == STEP::STEP_START) {
					CLog::IncrementActiveRequestCount();
					CLog::ProxyEvent(kLogProxyNewRequest, m_ipFromAddress);
				}
			}
			break;

		// Read first HTTP response line
		case STEP::STEP_FIRSTLINE:
			{
				// Do we have the full first line yet?
				if (m_recvInBuf.size() < 4)
					return ;
				// ゴミデータが詰まってるので終わる
				if (::strncmp(m_recvInBuf.c_str(), "HTTP", 4) != 0) {
					m_recvInBuf.clear();
					m_inStep = STEP::STEP_FINISH;
					m_connectionData->SetInStep(m_inStep);
					break;
				}

				size_t pos, len;
				if (CUtil::endOfLine(m_recvInBuf, 0, pos, len) == false)
					return ;		// まだ改行まで読み込んでないので帰る

				// Parse it
				size_t p1 = m_recvInBuf.find_first_of(" ");
				size_t p2 = m_recvInBuf.find_first_of(" ", p1+1);
				m_filterOwner.responseLine.ver = m_recvInBuf.substr(0, p1);
				m_filterOwner.responseLine.code = m_recvInBuf.substr(p1 + 1, p2 - p1 - 1);
				m_filterOwner.responseLine.msg = m_recvInBuf.substr(p2 + 1, pos - p2 - 1);
				m_filterOwner.responseCode = m_recvInBuf.substr(p1+1, pos-p1-1);

				// Remove it from receive-in buffer.
				// we don't place it immediately on send-in buffer,
				// as we may be willing to fake it (cf $REDIR)
				m_logResponse = m_recvInBuf.substr(0, pos + len);
				m_recvInBuf.erase(0, pos + len);

				// Next step will be to read headers
				m_inStep = STEP::STEP_HEADERS;
				m_connectionData->SetInStep(m_inStep);

				// Unless we have Content-Length or Transfer-Encoding headers,
				// we won't expect anything after headers.
				m_inSize = 0;
				m_inChunked = false;

				m_recvConnectionClose = false;
				m_sendConnectionClose = false;
				m_recvContentCoding = 0;
			}
			break;

		// Read and process headers, as long as there are any
		case STEP::STEP_HEADERS:
			{
				if (GetHeaders(m_recvInBuf, m_filterOwner.inHeaders, m_logResponse) == false)
					return ;

				m_inStep = STEP::STEP_DECODE;
				m_connectionData->SetInStep(m_inStep);
				// In case of 'HTTP 100 Continue,' expect another set of
				// response headers before the data
				if (m_filterOwner.responseLine.code == "100") {
					m_inStep = STEP::STEP_FIRSTLINE;
					m_connectionData->SetInStep(m_inStep);
					m_filterOwner.responseLine.ver.clear();
					m_filterOwner.responseLine.code.clear();
					m_filterOwner.responseLine.msg.clear();
					m_filterOwner.responseCode.clear();
				}
			}
			break;

		// Decode and process headers
		case STEP::STEP_DECODE:
			{
				CLog::HttpEvent(kLogHttpRecvIn, m_ipFromAddress, m_filterOwner.requestNumber, m_logResponse);

				// We must read the Content-Length and Transfer-Encoding
				// headers to know body length
				if (CUtil::noCaseContains(L"chunked", m_filterOwner.GetInHeader(L"Transfer-Encoding")) &&
					m_requestLine.method != "HEAD")
					m_inChunked = true;

				std::string contentLength = UTF8fromUTF16(m_filterOwner.GetInHeader(L"Content-Length"));
				if (contentLength.size() > 0 && m_requestLine.method != "HEAD") 
					m_inSize = boost::lexical_cast<int64_t>(contentLength);

				std::string contentType = UTF8fromUTF16(m_filterOwner.GetInHeader(L"Content-Type"));
				if (contentType.size() > 0) {
					// If size is not given, we'll read body until connection closes
					if (contentLength.empty() && m_requestLine.method != "HEAD")
						m_inSize = std::numeric_limits<int64_t>::max();

					// Check for filterable MIME types
					if (_VerifyContentType(contentType) == false && m_filterOwner.bypassBodyForced == false)
						m_filterOwner.bypassBody = true;
				} else {
					// Content-Typeがなければバイパスする
					m_filterOwner.bypassBody = true;
				}

				if (   CUtil::noCaseContains(L"close", m_filterOwner.GetInHeader(L"Connection"))
					|| (CUtil::noCaseBeginsWith("HTTP/1.0", m_filterOwner.responseLine.ver) &&
					    CUtil::noCaseContains(L"Keep-Alive", m_filterOwner.GetInHeader(L"Connection")) == false) ) {
						m_recvConnectionClose = true;
				}

				if (CUtil::noCaseBeginsWith("206", m_filterOwner.responseCode))
					m_filterOwner.bypassBody = true;

				// We'll work on a copy, since we don't want to alter
				// the real headers that $IHDR and $OHDR may access
				m_filterOwner.inHeadersFiltered = m_filterOwner.inHeaders;
				if (m_filterOwner.bypassIn == false && CSettings::s_filterIn) {
					// Apply filters one by one
					for (auto& headerfilter : m_vecpInFilter) {
						headerfilter->bypassed = false;
						const wstring& name = headerfilter->headerName;

						// If header is absent, temporarily create one
						if (CFilterOwner::GetHeader(m_filterOwner.inHeadersFiltered, name).empty()) {
							if (CUtil::noCaseBeginsWith(L"url", name)) {
								CFilterOwner::SetHeader(m_filterOwner.inHeadersFiltered, name, m_filterOwner.url.getUrl());
							} else {
								CFilterOwner::SetHeader(m_filterOwner.inHeadersFiltered, name, L"");
							}
						}
						// Test headers one by one
						for (auto& pair : m_filterOwner.inHeadersFiltered) {
							if (CUtil::noCaseEqual(name, pair.first))
								headerfilter->filter(pair.second);
						}
						// Remove null headers
						CFilterOwner::CleanHeader(m_filterOwner.inHeadersFiltered);

						if (m_filterOwner.killed) {
							// There has been a \k in a header filter, so we
							// redirect to an empty file and stop processing headers
							if (m_filterOwner.url.getPath().find(L".gif") != wstring::npos)
								m_filterOwner.rdirToHost = L"http://file//./html/killed.gif";
							else
								m_filterOwner.rdirToHost = L"http://file//./html/killed.html";
							m_filterOwner.rdirMode = 0;   // (to use non-transp code below)
							break;
						}
					}
				}

				// 受信ヘッダでリダイレクトを指示されたらリダイレクトする
				// (limited to 3, to prevent infinite loop)
				if (m_filterOwner.rdirToHost.size() > 0 && m_redirectedIn < 3) {
					// (This function will also take care of incoming variables)
					_ConnectWebsite();
					++m_redirectedIn;
					continue;
				}

				// Tell text filters to see whether they should work on it
				if (m_filterOwner.url.getDebug()) {
					m_useChain = true;
				} else {
					m_useChain = (m_filterOwner.bypassBody == false && CSettings::s_filterText);
				}

				if (m_useChain) {
					std::wstring contentEncoding = m_filterOwner.GetInHeader(L"Content-Encoding");
					if (CUtil::noCaseContains(L"gzip", contentEncoding)) {
						m_recvContentCoding = 1;
						if (m_decompressor == nullptr)
							m_decompressor.reset(new CZlibBuffer());
						m_decompressor->reset(false, true);
						CFilterOwner::RemoveHeader(m_filterOwner.inHeadersFiltered, L"Content-Encoding");

					} else if (CUtil::noCaseContains(L"deflate", contentEncoding)) {
						m_recvContentCoding = 2;
						if (m_decompressor == nullptr)
							m_decompressor.reset(new CZlibBuffer());
						m_decompressor->reset(false, false);
						CFilterOwner::RemoveHeader(m_filterOwner.inHeadersFiltered, L"Content-Encoding");

					} else if (contentEncoding.length() > 0) {	// 解釈できない圧縮形式
						m_useChain = false;
						m_filterOwner.bypassBody = true;
					}
				}

				// Decode new headers to control browser-side beehaviour
				if (CUtil::noCaseContains(L"close", CFilterOwner::GetHeader(m_filterOwner.inHeadersFiltered, L"Connection")))
					m_sendConnectionClose = true;

				if (m_useChain && (m_inChunked || m_inSize)) {
					// Our output will always be chunked: filtering can
					// change body size. So let's force this header.
					CFilterOwner::SetHeader(m_filterOwner.inHeadersFiltered, L"Transfer-Encoding", L"chunked");
					// Content-Lengthを消さないとおかしい
					CFilterOwner::RemoveHeader(m_filterOwner.inHeadersFiltered, L"Content-Length");
				}

				// Now we can put everything in the filtered buffer
				// (1.1 is necessary to have the browser understand chunked
				// transfer)
				if (m_filterOwner.url.getDebug()) {
					m_sendInBuf += "HTTP/1.1 200 OK" CRLF
						"Content-Type: text/html" CRLF
						"Connection: close" CRLF
						"Transfer-Encoding: chunked" CRLF;
					
					CLog::HttpEvent(kLogHttpSendIn, m_ipFromAddress, m_filterOwner.requestNumber, m_sendInBuf); 
					m_sendInBuf += CRLF;

					m_useChain = true;
					m_textFilterChain.DataReset();
					DataReset();

				} else {
					m_sendInBuf = "HTTP/1.1 " + m_filterOwner.responseLine.code + " " + m_filterOwner.responseLine.msg + CRLF;
					std::string name;
					for (auto& pair : m_filterOwner.inHeadersFiltered)
						m_sendInBuf += UTF8fromUTF16(pair.first) + ": " + UTF8fromUTF16(pair.second) + CRLF;
					
					CLog::HttpEvent(kLogHttpSendIn, m_ipFromAddress, m_filterOwner.requestNumber, m_sendInBuf);

					m_sendInBuf += CRLF;

					if (m_useChain) {
						m_textFilterChain.DataReset(); 
					} else {
						DataReset();
					}

					// File type will be reevaluated using first block of data
					//m_filterOwner.fileType.clear();	// ここでは消さない
				}

				CLog::AddNewRequest(m_filterOwner.requestNumber, m_filterOwner.responseLine.code, contentType, m_inChunked ? std::string("-1") : contentLength, UTF8fromUTF16(m_filterOwner.url.getUrl()));

				// Decide what to do next
				if (m_filterOwner.responseLine.code == "101") {
					m_inStep  = STEP::STEP_TUNNELING;
					m_connectionData->SetInStep(m_inStep);
					m_outStep = STEP::STEP_TUNNELING;
					m_connectionData->SetOutStep(m_outStep);

				} else {
					m_inStep = (m_filterOwner.responseLine.code == "204" ? STEP::STEP_FINISH :
								m_filterOwner.responseLine.code == "304" ? STEP::STEP_FINISH :
								m_filterOwner.responseLine.code[0] == '1' ? STEP::STEP_FINISH :
								m_inChunked ? STEP::STEP_CHUNK :
								m_inSize ? STEP::STEP_RAW :
								STEP::STEP_FINISH);
					m_connectionData->SetInStep(m_inStep);
				}
			}
			break;

		// Read  (CRLF) HexRawLength * CRLF before raw data
		// or    zero * CRLF CRLF
		case STEP::STEP_CHUNK:
			{
				size_t pos, len;
				while (CUtil::endOfLine(m_recvInBuf, 0, pos, len) && pos == 0)
					m_recvInBuf.erase(0,len);

				if (CUtil::endOfLine(m_recvInBuf, 0, pos, len) == false)
					return;
				m_inSize = CUtil::readHex(m_recvInBuf.substr(0,pos));

				if (m_inSize > 0) {
					m_recvInBuf.erase(0, pos+len);
					m_inStep = STEP::STEP_RAW;
					m_connectionData->SetInStep(m_inStep);
				} else {
					if (CUtil::endOfLine(m_recvInBuf, 0, pos, len, 2) == false)
						return;
					m_recvInBuf.erase(0, pos + len);
					m_inStep = STEP::STEP_FINISH;
					m_connectionData->SetInStep(m_inStep);
					_EndFeeding();
				}
			}
			break;

		// The next inSize bytes are left untouched
		case STEP::STEP_RAW:
			{
				int copySize = (int)m_recvInBuf.size();
				if (copySize > m_inSize)
					copySize = (int)m_inSize;
				if (copySize == 0 && m_inSize) 
					return ;	// 処理するデータがなくなったので

				std::string data;
				if (copySize == m_recvInBuf.size()) {
					data = std::move(m_recvInBuf);
					m_recvInBuf.reserve(data.capacity());
				} else {
					data = m_recvInBuf.substr(0, copySize);
					m_recvInBuf.erase(0, copySize);
				}
				m_inSize -= copySize;

				// We must decompress compressed data,
				// unless bypassed body with same coding
				if (m_recvContentCoding && m_useChain) {
					m_decompressor->feed(data);
					m_decompressor->read(data);
				}

				/// フィルターに食わせる
				if (m_useChain) {
					// provide filter chain with raw unfiltered data
					m_textFilterChain.DataFeed(data);
				} else {
					// In case we bypass the body, we directly send data to
					// the end of chain (the request manager itself)
					DataFeed(data);
				}

				// If we finished reading as much raw data as was expected,
				// continue to next step (finish or next chunk)
				if (m_inSize == 0) {
					if (m_inChunked) {
						m_inStep = STEP::STEP_CHUNK;
						m_connectionData->SetInStep(m_inStep);
					} else {
						m_inStep = STEP::STEP_FINISH;
						m_connectionData->SetInStep(m_inStep);
						_EndFeeding();
					}
				}
			}
			break;

		// Forward incoming data to browser
		case STEP::STEP_TUNNELING:
			{
				if (m_pSSLServerSession && m_filterOwner.responseLine.code != "101") {
					m_inStep = STEP::STEP_START;
					m_connectionData->SetInStep(m_inStep);
					continue;
				}

				m_sendInBuf += m_recvInBuf;
				m_recvInBuf.clear();
				return;
			}
			break;

			// A few things have to be done before we go back to start state...
		case STEP::STEP_FINISH:
			{
				// ブラウザからのデータを処理中ならここでは終了処理をしない
				// 終了処理は _ProcessOut で行われる
				if (m_outStep != STEP::STEP_START && m_outStep != STEP::STEP_FINISH)
					return ;

				m_outStep = STEP::STEP_START;
				m_connectionData->SetOutStep(m_outStep);
				m_inStep = STEP::STEP_START;
				m_connectionData->SetInStep(m_inStep);

				// Update active request stat
				CLog::DecrementActiveRequestCount();
				CLog::ProxyEvent(kLogProxyEndRequest, m_ipFromAddress);

				// Disconnect browser if we sent it "Connection:close"
				// If manager has been set invalid, close connection too
				if (m_sendConnectionClose || m_valid == false) {
					_SendIn();
					m_psockBrowser->Close();
				}

				// Disconnect website if it sent us "Connection:close"
				if (m_recvConnectionClose || m_psockBrowser->IsConnected() == false) {
					m_psockWebsite->Close();
				}	
			}
			break;	

		}	// swtich
	}
}




/// Send incoming data to browser
/// ブラウザへレスポンスを送信
bool	CRequestManager::_SendIn()
{
	if (m_sendInBuf.empty())
		return false;

	bool ret = false;
	if (m_pSSLClientSession) {
		if (m_pSSLClientSession->Write(m_sendInBuf.c_str(), (int)m_sendInBuf.size())) {
			m_sendInBuf.erase(0, m_pSSLClientSession->GetLastWriteCount());
			ret = true;
		}
	} else {
		// Send everything to browser, if connected
		if (m_psockBrowser->IsConnected()) {			
			// Trouble with browser's socket, end of all things
			if (m_psockBrowser->Write(m_sendInBuf.c_str(), (int)m_sendInBuf.size())) {	
				m_sendInBuf.erase(0, m_psockBrowser->GetLastWriteCount());
				ret = true;
			}
		}
	}
	return ret;
}


/// Determine if a MIME type can go through body filters.  Also set $TYPE() accordingly.
bool	CRequestManager::_VerifyContentType(std::string& ctype)
{
    size_t end = ctype.find(';');
    if (end == std::string::npos)  
		end = ctype.size();

    // Remove top-level type, e.g., text/html -> html
    size_t slash = ctype.find('/');
    if (slash == std::string::npos || slash >= end) {
      m_filterOwner.fileType = "oth";
      return false;
    }
    slash++;

    // Strip off any x- before the type (e.g., x-javascript)
    if (ctype[slash] == 'x' && ctype[slash + 1] == '-')
      slash += 2;
    std::string type = ctype.substr(slash, end - slash);
    CUtil::lower(type);

    if (type == "html") {
		m_filterOwner.fileType = "htm";
		return true;
    } else if (type == "javascript") {
		m_filterOwner.fileType = "js";
		return true;
    } else if (type == "css") {
		m_filterOwner.fileType = "css";
		return true;
	} else if (type == "vbscript") {
		m_filterOwner.fileType = "vbs";
		return false;
	} else if (type == "json") {
		m_filterOwner.fileType = "json";
		return false;
	} else if (type == "xml") {
		m_filterOwner.fileType = "xml";
		return false;
	} else if (type == "plain") {
		m_filterOwner.fileType = "plain";
		return false;
	}

    m_filterOwner.fileType = "oth";
    return false;
}



void	CRequestManager::DataFeed(const std::string& data)
{
    if (m_dumped) 
		return;

    size_t size = data.size();
	if (size) {
		if (m_useChain || m_inChunked) {
			m_sendInBuf += CUtil::makeHex((unsigned int)size) + CRLF + data + CRLF;

		} else {
			// Send a chunk of uncompressed/unchanged data
			//m_sendInBuf += CUtil::makeHex(size) + CRLF + data + CRLF;
			m_sendInBuf += data;
		}
	}
}

/// Record that the chain emptied itself, finish compression if needed
void	CRequestManager::DataDump()
{
    if (m_dumped) 
		return;
    m_dumped = true;

    if (m_inStep != STEP::STEP_FINISH) {
        // Data has been dumped by a filter, not by this request manager.
        // So we must disconnect the website to stop downloading.
        m_psockWebsite->Close();
    }
}

/* Send the last deflated bytes to the filter chain then dump the chain
 * and append last empty chunk
 */
void	CRequestManager::_EndFeeding() {

    if (m_recvContentCoding && m_useChain) {
        string data;
        m_decompressor->dump();
        m_decompressor->read(data);

        if (m_useChain) {
            m_textFilterChain.DataFeed(data);
        } else {
            DataFeed(data);
        }
    }

    if (m_useChain) {
        m_textFilterChain.DataDump();
		if (m_filterOwner.url.getDebug()) {
			m_dumped = false;
			DataFeed("\n</div>\n</body>\n</html>\n");
			m_dumped = true;
		}
    } else {
        DataDump();
    }

	if (m_useChain || m_inChunked) {
		// Write last chunk (size 0)
		m_sendInBuf += "0" CRLF CRLF;
	}
}

void	CRequestManager::_FakeResponse(const std::string& code, const std::wstring& filename /*= L""*/)
{
	std::string contentType = filename.empty() ? std::string("text/plain") : CUtil::getMimeType(UTF8fromUTF16(filename));
	std::string content;
	if (filename.size() > 0)
		content = CUtil::getFile(filename);
	if (contentType == "text/html" && content.size() > 0)
		content = CUtil::replaceAll(content, "%%1%%", code);
	m_inStep = STEP::STEP_FINISH;
	m_connectionData->SetInStep(m_inStep);
	m_sendInBuf =
		"HTTP/1.1 " + code + CRLF
		"Content-Type: " + contentType + CRLF
		"Content-Length: " + boost::lexical_cast<std::string>(content.size()) + CRLF
		"Connection: close" + CRLF CRLF;
	CLog::HttpEvent(kLogHttpSendIn, m_ipFromAddress, m_filterOwner.requestNumber, m_sendInBuf);
	m_sendInBuf += content;
	m_sendConnectionClose = true;
}

