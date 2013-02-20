/**
*	@file	RequestManager.cpp
*	@brief	ブラウザ⇔プロクシ⇔サーバー間の処理を受け持つ
*/


#include "stdafx.h"
#include "RequestManager.h"
#include <sstream>
#include <boost\lexical_cast.hpp>
#include "DebugWindow.h"
#include "Log.h"

namespace CUtil {

using namespace std;

#define CR	'\r'
#define LF	'\n'
#define CRLF "\r\n"

// Locates the next end-of-line (can be CRLF or LF)
bool endOfLine(const string& str, size_t start,
                      size_t& pos, size_t& len, int nbr = 1) {

    while (true) {
        size_t index = str.find(LF, start);
        if (index == string::npos) return false;
        if (start < index && str[index-1] == CR) {
            len = 2; pos = index - 1;
        } else {
            len = 1; pos = index;
        }
        if (nbr <= 1) {
            return true;
        } else {
            int remain = nbr-1;
            string::const_iterator it = str.begin() + (index + 1);
            while (remain && it != str.end()) {
                if (*it == LF) {
                    len++; it++; remain--;
                } else if (*it == CR) {
                    len++; it++;
                } else {
                    break;
                }
            }
            if (remain) {
                start = pos+len;
            } else {
                return true;
            }
        }
    }
}

// Case-insensitive compare
bool noCaseEqual(const string& s1, const string& s2) 
{
    if (s1.size() != s2.size()) 
		return false;
    return ::_stricmp(s1.c_str(), s2.c_str()) == 0;
}

// Put string in lowercase
string& lower(string& s) {
    for (string::iterator it = s.begin(); it != s.end(); it++) {
        *it = tolower(*it);
    }
    return s;
}

// Returns true if s2 contains s1
bool noCaseContains(const string& s1, const string& s2)
{
    string nc1 = s1, nc2 = s2;
    lower(nc1);
    lower(nc2);
    return (nc2.find(nc1) != string::npos);
}

// Returns true if s2 begins with s1
bool noCaseBeginsWith(const string& s1, const string& s2)
{
	return ::_strnicmp(s2.c_str(), s1.c_str(), s1.size()) == 0;
}


// Trim string
string& trim(string& s, string list = string(" \t\r\n"))
{
    size_t p1 = s.find_first_not_of(list);
    if (p1 == string::npos) 
		return s = "";
    size_t p2 = s.find_last_not_of(list);
    return s = s.substr(p1, p2+1-p1);
}

// Decode hexadecimal number at string start
unsigned int readHex(const string& s)
{
    unsigned int n = 0, h = 0;
    string H("0123456789ABCDEF");
    for (string::const_iterator c = s.begin(); c != s.end()
            && (h = H.find(toupper(*c))) != string::npos; c++)
        n = n*16 + h;
    return n;
}

// Make a hex representation of a number
std::string makeHex(unsigned int n) {
    std::stringstream ss;
    ss << std::uppercase << hex << n;
    return ss.str();
}

}	// namespace CUtil



/////////////////////////////////////////////////////////////////////////////////////////////////////
// CRequestManager

CRequestManager::CRequestManager(std::unique_ptr<CSocket>&& psockBrowser) :
	m_textFilterChain(m_filterOwner, this),
	m_psockBrowser(std::move(psockBrowser)),
	m_valid(true),
	m_dumped(false),
	m_outSize(0),
	m_outChunked(false),
	m_inSize(0),
	m_inChunked(false)
{
	m_filterOwner.requestNumber = 0;
}


CRequestManager::~CRequestManager(void)
{
    // If we were processing outgoing or incoming
    // data, we were an active request
    if (m_outStep != STEP_START || m_inStep != STEP_START) {
		CLog::DecrementActiveRequestCount();
		CLog::ProxyEvent(kLogProxyEndRequest, m_ipFromAddress);
    }

	// Destroy sockets
	m_psockBrowser->Close();
	m_psockWebsite->Close();
}

void CRequestManager::Manage()
{
	clock_t stopTime = 0;
	m_outStep = STEP_START;
	m_inStep = STEP_START;
	m_ipFromAddress = m_psockBrowser->GetFromAddress();
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
			TRACEIN("Manage ポート %d : サイトとつながりました！[%s]", m_ipFromAddress.GetPortNumber(), m_filterOwner.contactHost.c_str());
			do {
				// Full processing
				bool bRest = true;
				if (_SendOut())		// サイトへデータを送信
					bRest = false;

				if (_ReceiveIn()) {	// サイトからのデータを受信
					bRest = false;
					_ProcessIn();	// サイトからのデータを処理
				} else if (m_psockWebsite->IsConnectionKilledFromPeer()) {
					_ProcessIn();	// サイトからのデータを処理
				}

				if (_SendIn())		// ブラウザへサイトからのデータを送信
					bRest = false;
				if (_ReceiveOut()) { // ブラウザからのデータを受信(SSLかパイプライン以外では用無しのはず。。。)
					bRest = false;
					_ProcessOut();
				} else if (m_psockBrowser->IsConnectionKilledFromPeer()) {
					_ProcessOut();
					if (m_valid == false) 
						m_psockBrowser->Close();
				}

				if (bRest) {
					if (m_outStep != STEP_START) {
						if (stopTime) 
							stopTime = 0;
					} else {
						if (stopTime == 0) 
							stopTime = clock() + 10 * CLOCKS_PER_SEC;
						if (m_valid == false || clock() > stopTime)
							m_psockBrowser->Close();
					}
					::Sleep(10);
				}

				// どちらからもデータが受信できなかったら切る
				if (m_psockWebsite->IsConnectionKilledFromPeer() && m_psockBrowser->IsConnectionKilledFromPeer()) {
					TRACEIN("Manage ポート %d : outStep[%d] inStep[%d] ブラウザとサイトのデータ受信が終了しました", m_ipFromAddress.GetPortNumber(), m_outStep, m_inStep);
					m_psockWebsite->Close();
					m_psockBrowser->Close();
				}

			} while (m_psockWebsite->IsConnected() &&  m_psockBrowser->IsConnected());
			TRACE("Manage ポート %d : outStep[%d] inStep[%d] ", m_ipFromAddress.GetPortNumber(), m_outStep, m_inStep);
			if (m_psockWebsite->IsConnected() == false)
				TRACEIN("サイトとの接続が切れました [%s]", m_filterOwner.contactHost.c_str());
			else
				TRACEIN("ブラウザとの接続が切れました [%s]", m_filterOwner.contactHost.c_str());

			//if (m_outStep == STEP_TUNNELING && m_psockBrowser->IsConnected()) {
			//	_ProcessIn();
			//}

			// Terminate feeding browser
			_ReceiveIn();
			_ProcessIn();
			if (m_inStep == STEP_RAW || m_inStep == STEP_CHUNK) {
				m_inStep = STEP_FINISH;
				_EndFeeding();
				_ProcessIn();
			}
			_SendIn();
			if (m_outStep == STEP_FINISH) {
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
			if (m_outStep != STEP_START) {
				if (stopTime)
					stopTime = 0;
			} else {
				if (stopTime == 0)
					stopTime = clock() + /*SOCKET_EXPIRATION*/10 * CLOCKS_PER_SEC;
				if (m_valid == false || clock() > stopTime)
					m_psockBrowser->Close();
			}
			::Sleep(50);
		}

	}
	} catch (SocketException& e) {
		TRACEIN("例外が発生しました！ : ポート %d 例外:%s(%d)", m_ipFromAddress.GetPortNumber(), e.msg, e.err); 
	} catch (...) {
		TRACEIN("例外が発生しました！ : ポート %d", m_ipFromAddress.GetPortNumber());
	}
	TRACEIN(_T("Manage finish : ポート %d"), m_ipFromAddress.GetPortNumber());
}

bool	CRequestManager::IsDoInvalidPossible()
{
	if (m_valid == false) {
		return false;
	}
	if (m_psockBrowser->IsConnected() && m_psockBrowser->IsConnectionKilledFromPeer()) {
		if (m_outStep == STEP_TUNNELING) {
			return true;
		}
	}
	return false;
}

bool CRequestManager::_ReceiveOut()
{
	char buf[kReadBuffSize + 1];
	bool bDataReceived = false;
	while (m_psockBrowser->IsDataAvailable() && m_psockBrowser->Read(buf, kReadBuffSize)) {
		int count = m_psockBrowser->GetLastReadCount();
		if (count == 0)
			break;
		//buf[count] = 0;
		m_recvOutBuf += std::string(buf, count);
		bDataReceived = true;
	}
	return bDataReceived;
}

/// Send outgoing data to website
bool CRequestManager::_SendOut()
{
	bool ret = false;
	// Send everything to website, if connected
	if (m_sendOutBuf.size() > 0 && m_psockWebsite->IsConnected()) {
		ret = true;
		if (m_psockWebsite->Write(m_sendOutBuf.c_str(), m_sendOutBuf.size()) == false)
			m_psockWebsite->Close();	// Trouble with browser's socket, end of all things
		else
			m_sendOutBuf.erase(0, m_psockWebsite->GetLastWriteCount());
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
		case STEP_START:
			{
				if (m_recvOutBuf.empty())
					return ;			// 処理するのに十分なデータがないので帰る
			
				m_outStep = STEP_FIRSTLINE;

				// Update active request stat, but if processIn() did it already
				if (m_inStep == STEP_START) {
					CLog::IncrementActiveRequestCount();
					CLog::ProxyEvent(kLogProxyNewRequest, m_ipFromAddress);
				}

				// Reset variables relative to a single request/response
				m_filterOwner.bypassOut = false;
				m_filterOwner.bypassIn = false;
				m_filterOwner.bypassBody = false;
				m_filterOwner.bypassBodyForced = false;
				m_filterOwner.killed = false;
				//variables.clear();
				m_filterOwner.requestNumber = CLog::IncrementRequestCount();
				//rdirMode = 0;
				//rdirToHost.clear();
				//redirectedIn = 0;
				m_recvConnectionClose = false;
				m_sendConnectionClose = false;
				m_recvContentCoding = m_sendContentCoding = 0;
				m_logPostData.clear();
			}
			break;

		// Read first HTTP request line
		case STEP_FIRSTLINE:
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

				// Next step will be to read headers
				m_outStep = STEP_HEADERS;

				// Unless we have Content-Length or Transfer-Encoding headers,
				// we won't expect anything after headers.
				m_outSize = 0;
				m_outChunked = false;
				m_filterOwner.outHeaders.clear();
				// (in case a silly OUT filter wants to read IN headers)
				m_filterOwner.inHeaders.clear();
				m_filterOwner.responseCode.clear();
			}
			break;

		// Read and process headers, as long as there are any
		case STEP_HEADERS:
			{
				for (;;) {
					// Look for end of line
					size_t pos, len;
					if (CUtil::endOfLine(m_recvOutBuf, 0, pos, len) == false)
						return ;	// 改行がないので帰る

					// Check if we reached the empty line
					if (pos == 0) {
						m_recvOutBuf.erase(0, len);
						m_outStep = STEP_DECODE;
						break;
					}

					// Find the header end
					while (	  pos + len >= m_recvOutBuf.size()
						   || m_recvOutBuf[pos + len] == ' '
						   || m_recvOutBuf[pos + len] == '\t'
						  )
					{
						if (CUtil::endOfLine(m_recvOutBuf, pos + len, pos, len) == false)
							return ;
					}

					// Record header
					size_t colon = m_recvOutBuf.find(':');
					if (colon != std::string::npos) {
						std::string name = m_recvOutBuf.substr(0, colon);
						std::string value = m_recvOutBuf.substr(colon + 1, pos - colon - 1);
						CUtil::trim(value);
						m_filterOwner.outHeaders.emplace_back(name, value);
					}
					m_logRequest += m_recvOutBuf.substr(0, pos + len);
					m_recvOutBuf.erase(0, pos + len);
				}
			}
			break;

		// Decode and process headers
		case STEP_DECODE:
			{
				CLog::HttpEvent(kLogHttpRecvOut, m_ipFromAddress, m_filterOwner.requestNumber, m_logRequest);

				// Get the URL and the host to contact (unless we use a proxy)
				m_filterOwner.url.parseUrl(m_requestLine.url);
			    m_filterOwner.bypassIn   = m_filterOwner.url.getBypassIn();
				m_filterOwner.bypassOut  = m_filterOwner.url.getBypassOut();
				m_filterOwner.bypassBody = m_filterOwner.url.getBypassText();

				m_filterOwner.contactHost = m_filterOwner.url.getHostPort();

				// We must read the Content-Length and Transfer-Encoding
				// headers to know if thre is POSTed data
				if (CUtil::noCaseContains("chunked", m_filterOwner.GetOutHeader("Transfer-Encoding")))
					m_outChunked = true;

				std::string contentLength = m_filterOwner.GetOutHeader("Content-Length");
				if (contentLength.size() > 0)
					m_outSize = boost::lexical_cast<int>(contentLength);

				// We'll work on a copy, since we don't want to alter
				// the real headers that $IHDR and $OHDR may access
				auto outHeadersFiltered = m_filterOwner.outHeaders;

				// If we haven't connected to ths host yet, we do it now.
				// This will allow incoming processing to start. If the
				// connection fails, incoming processing will jump to
				// the finish state, so that we can accept other requests
				// on the same browser socket (persistent connection).
				_ConnectWebsite();

				// If website is read, send headers
				if (m_inStep == STEP_START) {

					// Update URL within request
					CFilterOwner::SetHeader(outHeadersFiltered, "Host", m_filterOwner.url.getHost());

					if (CUtil::noCaseContains("Keep-Alive", CFilterOwner::GetHeader(outHeadersFiltered, "Proxy-Connection"))) {
						CFilterOwner::RemoveHeader(outHeadersFiltered, "Proxy-Connection");
						CFilterOwner::SetHeader(outHeadersFiltered, "Connection", "Keep-Alive");
					}

				
					if (m_filterOwner.contactHost == m_filterOwner.url.getHostPort())
						m_requestLine.url = m_filterOwner.url.getAfterHost();
					else
						m_requestLine.url = m_filterOwner.url.getUrl();
					if (m_requestLine.url.empty())
						m_requestLine.url = "/";

					// Now we can put everything in the filtered buffer
					m_sendOutBuf = m_requestLine.method + " " + m_requestLine.url + " " + m_requestLine.ver + CRLF;
					for (auto& pair : outHeadersFiltered)
						m_sendOutBuf += pair.first + ": " + pair.second + CRLF;

					// Log outgoing headers
					CLog::HttpEvent(kLogHttpSendOut, m_ipFromAddress, m_filterOwner.requestNumber, m_sendOutBuf);

					m_sendOutBuf += CRLF;
				}

				// Decide next step
				if (m_requestLine.method == "HEAD") {
					// HEAD requests have no body, even if a size is provided
					m_outStep = STEP_FINISH;
				} else if (m_requestLine.method == "CONNECT") {
					// SSL Tunneling (we won't process exchanged data)
					m_outStep = STEP_TUNNELING;
				} else {
					m_outStep = (m_outChunked ? STEP_CHUNK : m_outSize ? STEP_RAW : STEP_FINISH);
				}
			}
			break;

		// Read CRLF HexRawLength * CRLF before raw data
		// or CRLF zero * CRLF CRLF
		case STEP_CHUNK:
			{
				TRACEIN(_T("ProcessOut #%d : STEP_CHUNK"), m_filterOwner.requestNumber);
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
					m_outStep = STEP_RAW;
				} else {
					if (CUtil::endOfLine(m_recvOutBuf, 0, pos, len, 2) == false)
						return ;
					m_sendOutBuf += m_recvOutBuf.substr(0, pos) + CRLF CRLF;
					m_recvOutBuf.erase(0, pos + len);

					CLog::HttpEvent(kLogHttpPostOut, m_ipFromAddress, m_filterOwner.requestNumber, m_logPostData);

					m_outStep = STEP_FINISH;

					// We shouldn't send anything to website if it is not expecting data
					// (i.e we obtained a faked response)
					if (m_inStep == STEP_FINISH)
						m_sendOutBuf.clear();
				}
			}
			break;

		// The next outSize bytes are left untouched
		case STEP_RAW:
			{
				TRACEIN(_T("ProcessOut #%d : STEP_RAW"), m_filterOwner.requestNumber);
				int copySize = m_recvOutBuf.size();
				if (copySize > m_outSize)
					copySize = m_outSize;
				if (copySize == 0 && m_outSize)
					return ;

				std::string postData = m_recvOutBuf.substr(0, copySize);
				m_sendOutBuf += postData;
				m_logPostData += postData;
				m_recvOutBuf.erase(0, copySize);
				m_outSize -= copySize;

				// If we finished reading as mush raw data as was expected,
				// continue to next step (finish or next chunk)
				if (m_outSize == 0) {
					if (m_outChunked) {
						m_outStep = STEP_CHUNK;
					} else {
						CLog::HttpEvent(kLogHttpPostOut, m_ipFromAddress, m_filterOwner.requestNumber, m_logPostData);

						m_outStep = STEP_FINISH;
					}
				}

				// We shouldn't send anything to website if it is not expecting data
				// (i.e we obtained a faked response)
				if (m_inStep == STEP_FINISH)
					m_sendOutBuf.clear();
			}
			break;

		// Foward outgoing data to website
		case STEP_TUNNELING:
			{
				m_sendOutBuf += m_recvOutBuf;
				m_recvOutBuf.clear();

				if ( m_psockWebsite->IsConnected() == false || m_psockWebsite->IsConnectionKilledFromPeer() ) 
				{
					m_sendConnectionClose = true;
					m_outStep = STEP_FINISH;
					m_inStep = STEP_FINISH;
					break;
				}
				return ;
			}
			break;

		// We'll wait for response completion
		case STEP_FINISH:
			{
				TRACEIN(_T("ProcessOut #%d : STEP_FINISH inStep[%d]"), m_filterOwner.requestNumber, m_inStep);
				if (m_inStep != STEP_FINISH)
					return ;

				m_outStep = STEP_START;
				m_inStep = STEP_START;

				// Update active request stat
				CLog::DecrementActiveRequestCount();
				CLog::ProxyEvent(kLogProxyEndRequest, m_ipFromAddress);

				// Disconnect browser if we send it "Connection:close"
				// If manager has been set invalid, close connection too
				if (m_sendConnectionClose || m_valid == false) {
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
	if (m_inStep == STEP_DECODE) {
		m_psockWebsite->Close();
		m_inStep = STEP_START;
		m_filterOwner.bypassBody = m_filterOwner.bypassBodyForced = false;
	}

    _ReceiveIn();
    m_recvInBuf.clear();
    m_sendInBuf.clear();
#if 0
    // Test for "local.ptron" host
    if (url.getHost() == "local.ptron") {
        rdirToHost = url.getUrl();
    }
    if (CUtil::noCaseBeginsWith("http://local.ptron", rdirToHost)) {
        rdirToHost = "http://file//./html" + CUrl(rdirToHost).getPath();
    }
#endif

#if 0
    // Test for redirection __to file__
    // ($JUMP to file will behave like a transparent redirection,
    // since the browser may not be on the same file system)
    if (CUtil::noCaseBeginsWith("http://file//", rdirToHost)) {

        string filename = CUtil::makePath(rdirToHost.substr(13));
        if (wxFile::Exists(S2W(filename))) {
            fakeResponse("200 OK", filename);
        } else {
            fakeResponse("404 Not Found", "./html/error.html", true,
                         CSettings::ref().getMessage("404_NOT_FOUND"),
                         filename);
        }
        rdirToHost.clear();
        return;
    }
#endif

#if 0
    // Test for non-transparent redirection ($JUMP)
    if (!rdirToHost.empty() && rdirMode == 0) {
        // We'll keep browser's socket, for persistent connections, and
        // continue processing outgoing data (which will not be moved to
        // send buffer).
        inStep = STEP_FINISH;
        sendInBuf =
            "HTTP/1.0 302 Found" CRLF
            "Location: " + rdirToHost + CRLF;
        CLog::ref().logHttpEvent(pmEVT_HTTP_TYPE_SENDIN, addr,
                                 reqNumber, sendInBuf);
        sendInBuf += CRLF;
        rdirToHost.clear();
        sendConnectionClose = true;
        return;
    }
#endif

#if 0

    // Test for transparent redirection to URL ($RDIR)
    // Note: new URL will not go through URL* OUT filters
    if (!rdirToHost.empty() && rdirMode == 1) {

        // Change URL
        url.parseUrl(rdirToHost);
        if (url.getBypassIn())    bypassIn   = true;
        if (url.getBypassOut())   bypassOut  = true;
        if (url.getBypassText())  bypassBody = true;
        useSettingsProxy = CSettings::ref().useNextProxy;
        contactHost = url.getHostPort();
        setHeader(outHeaders, "Host", url.getHost());

        rdirToHost.clear();
    }
#endif

#if 0
    // If we must contact the host via the settings' proxy,
    // we now override the contactHost
    if (useSettingsProxy && !CSettings::ref().nextProxy.empty()) {
        contactHost = CSettings::ref().nextProxy;
    }
#endif
    // Unless we are already connected to this host, we try and connect now
	if (m_previousHost != m_filterOwner.contactHost || m_psockWebsite->IsConnected() == false) {

        // Disconnect from previous host
        m_psockWebsite->Close();
        m_previousHost = m_filterOwner.contactHost;

        // The host string is composed of host and port
        std::string name = m_filterOwner.contactHost;
		std::string port;
        size_t colon = name.find(':');
        if (colon != std::string::npos) {    // (this should always happen)
            port = name.substr(colon+1);
            name = name.substr(0,colon);
        }
        if (port.empty()) 
			port = "80";

        // Check the host (Hostname() asks the DNS)
		IPv4Address host;
		if (name.empty() || host.SetService(port) == false || host.SetHostName(name) == false) {
            // The host address is invalid (or unknown by DNS)
            // so we won't try a connection.
			_FakeResponse("502 Bad Gateway");
            //fakeResponse("502 Bad Gateway", "./html/error.html", true,
            //             CSettings::ref().getMessage("502_BAD_GATEWAY"),
            //             name);
            return ;
		}
#if 0
        wxIPV4address host;
        host.Service(S2W(port));
        if (name.empty() || !host.Hostname(S2W(name))) {
            // The host address is invalid (or unknown by DNS)
            // so we won't try a connection.
            fakeResponse("502 Bad Gateway", "./html/error.html", true,
                         CSettings::ref().getMessage("502_BAD_GATEWAY"),
                         name);
            return;
        }
#endif

        // Connect
		m_psockWebsite->Connect(host);
#if 0
        for (int i=0, time=0; website->IsDisconnected() && i<5 && time<1; i++) {
            if (!website->Connect(host, false)) {
                while (!website->WaitOnConnect(1,0)) {
                    if (browser->IsDisconnected()) return;
                    time++;
                }
            }
        }
#endif
		if (m_psockWebsite->IsConnected() == false) {
            // Connection failed, warn the browser
			_FakeResponse("503 Service Unavailable");
            //fakeResponse("503 Service Unavailable", "./html/error.html", true,
            //             CSettings::ref().getMessage("503_UNAVAILABLE"),
            //             contactHost);
            return ;
        }
        if (m_requestLine.method == "CONNECT") {
            // for CONNECT method, incoming data is encrypted from start
            m_sendInBuf = "HTTP/1.0 200 Connection established" CRLF
						  "Proxy-agent: " "Proxydomo/0.5"/*APP_NAME " " APP_VERSION*/ CRLF CRLF;
			CLog::HttpEvent(kLogHttpSendIn, m_ipFromAddress, m_filterOwner.requestNumber, m_sendInBuf);
            m_inStep = STEP_TUNNELING;
        }
    }

    // Prepare and send a simple request, if we did a transparent redirection
    if (m_outStep != STEP_DECODE && m_inStep == STEP_START) {
        if (m_filterOwner.contactHost == m_filterOwner.url.getHostPort())
            m_requestLine.url = m_filterOwner.url.getAfterHost();
        else
            m_requestLine.url = m_filterOwner.url.getUrl();
        if (m_requestLine.url.empty()) 
			m_requestLine.url = "/";
        m_sendOutBuf =
            m_requestLine.method + " " + m_requestLine.url + " HTTP/1.1" CRLF
            "Host: " + m_filterOwner.url.getHost() + CRLF;

		CLog::HttpEvent(kLogHttpSendOut, m_ipFromAddress, m_filterOwner.requestNumber, m_sendOutBuf);

        m_sendOutBuf += m_logPostData + CRLF;
        // Send request
        _SendOut();
    }
}


bool	CRequestManager::_ReceiveIn()
{
	char buf[kReadBuffSize + 1];
	bool bDataReceived = false;
	while (m_psockWebsite->IsDataAvailable() && m_psockWebsite->Read(buf, kReadBuffSize)) {
		int count = m_psockWebsite->GetLastReadCount();
		if (count == 0)
			break;
		//buf[count] = 0;
		m_recvInBuf += std::string(buf, count);
		bDataReceived = true;
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
		case STEP_START:
			{
				// We need something in the buffer
				if (m_recvInBuf.empty())
					return ;	// 処理するバッファが無いので帰る

				m_inStep = STEP_FIRSTLINE;
				// Update active request stat, but if processOut() did it already
				if (m_outStep == STEP_START) {
					CLog::IncrementActiveRequestCount();
					CLog::ProxyEvent(kLogProxyNewRequest, m_ipFromAddress);
				}
			}
			break;

		// Read first HTTP response line
		case STEP_FIRSTLINE:
			{
				// Do we have the full first line yet?
				size_t pos, len;
				if (CUtil::endOfLine(m_recvInBuf, 0, pos, len) == false)
					return ;		// まだ改行まで読み込んでないので帰る

				// Parse it
				size_t p1 = m_recvInBuf.find_first_of(" ");
				size_t p2 = m_recvInBuf.find_first_of(" ", p1+1);
				m_responseLine.ver  = m_recvInBuf.substr(0,p1);
				m_responseLine.code = m_recvInBuf.substr(p1+1, p2-p1-1);
				m_responseLine.msg  = m_recvInBuf.substr(p2+1, pos-p2-1);
				m_filterOwner.responseCode = m_recvInBuf.substr(p1+1, pos-p1-1);

				// Remove it from receive-in buffer.
				// we don't place it immediately on send-in buffer,
				// as we may be willing to fake it (cf $REDIR)
				m_logResponse = m_recvInBuf.substr(0, pos + len);
				m_recvInBuf.erase(0, pos + len);

				// Next step will be to read headers
				m_inStep = STEP_HEADERS;

				// Unless we have Content-Length or Transfer-Encoding headers,
				// we won't expect anything after headers.
				m_inSize = 0;
				m_inChunked = false;
				m_filterOwner.inHeaders.clear();
				//m_filterOwner.responseCode.clear();	 // だめだろ・・・
				m_recvConnectionClose = false;
				m_sendConnectionClose = false;
				m_recvContentCoding = m_sendContentCoding = 0;
				//useGifFilter = false;
			}
			break;

		// Read and process headers, as long as there are any
		case STEP_HEADERS:
			{
				for (;;) {
					// Look for end of line
					size_t pos, len;
					if (CUtil::endOfLine(m_recvInBuf, 0, pos, len) == false)
						return ;

					// Check if we reached the empty line
					if (pos == 0) {
						m_recvInBuf.erase(0, len);
						m_inStep = STEP_DECODE;
						// In case of 'HTTP 100 Continue,' expect another set of
						// response headers before the data
						if (m_responseLine.code == "100") {
							m_inStep = STEP_FIRSTLINE;
							m_responseLine.ver.clear();
							m_responseLine.code.clear();
							m_responseLine.msg.clear();
							m_filterOwner.responseCode.clear();
						}
						break;
					}

					// Find the header end
					while (   pos+len >= m_recvInBuf.size()
						|| m_recvInBuf[pos+len] == ' '
						|| m_recvInBuf[pos+len] == '\t' 
						)
					{
						if (CUtil::endOfLine(m_recvInBuf, pos+len, pos, len) == false)
							return ;
					}

					// Record header
					size_t colon = m_recvInBuf.find(':');
					if (colon != std::string::npos) {
						std::string name = m_recvInBuf.substr(0, colon);
						std::string value = m_recvInBuf.substr(colon+1, pos-colon-1);
						CUtil::trim(value);
						m_filterOwner.inHeaders.emplace_back(name, value);
					}
					m_logResponse += m_recvInBuf.substr(0, pos + len);
					m_recvInBuf.erase(0, pos+len);
				}
			}
			break;

		// Decode and process headers
		case STEP_DECODE:
			{
				CLog::HttpEvent(kLogHttpRecvIn, m_ipFromAddress, m_filterOwner.requestNumber, m_logResponse);

				// We must read the Content-Length and Transfer-Encoding
				// headers to know body length
				if (CUtil::noCaseContains("chunked", m_filterOwner.GetInHeader("Transfer-Encoding")))
					m_inChunked = true;

				std::string contentLength = m_filterOwner.GetInHeader("Content-Length");
				if (contentLength.size() > 0) 
					m_inSize = boost::lexical_cast<int>(contentLength);

				std::string contentType = m_filterOwner.GetInHeader("Content-Type");
				if (contentType.size() > 0) {
					// If size is not given, we'll read body until connection closes
					if (contentLength.empty()) 
						m_inSize = /*BIG_NUMBER*/ 0x7FFFFFFF;

					// Check for filterable MIME types
					if (_VerifyContentType(contentType) == false && m_filterOwner.bypassBodyForced == false)
						m_filterOwner.bypassBody = true;

					// Check for GIF to freeze
					//if (CUtil::noCaseContains("image/gif",
					//			getHeader(inHeaders, "Content-Type"))) {
					//	useGifFilter = CSettings::ref().filterGif;
					//}
				}

				if (   CUtil::noCaseContains("close", m_filterOwner.GetInHeader("Connection"))
					|| (CUtil::noCaseBeginsWith("HTTP/1.0", m_responseLine.ver) && 
					    CUtil::noCaseContains("Keep-Alive", m_filterOwner.GetInHeader("Connection")) == false) ) {
						m_recvConnectionClose = true;
				}


				if (CUtil::noCaseContains("gzip", m_filterOwner.GetInHeader("Content-Encoding"))) {
					m_recvContentCoding = 1;
					if (m_decompressor == nullptr) 
						m_decompressor.reset(new CZlibBuffer());
					m_decompressor->reset(false, true);
				} else if (CUtil::noCaseContains("deflate", m_filterOwner.GetInHeader( "Content-Encoding"))) {
					m_recvContentCoding = 2;
					if (m_decompressor == nullptr) 
						m_decompressor.reset(new CZlibBuffer());
					m_decompressor->reset(false, false);
				} else if (CUtil::noCaseContains("compress", m_filterOwner.GetInHeader("Content-Encoding"))) {
					m_filterOwner.bypassBody = true;
				}

				if (CUtil::noCaseBeginsWith("206", m_filterOwner.responseCode))
					m_filterOwner.bypassBody = true;

				// We'll work on a copy, since we don't want to alter
				// the real headers that $IHDR and $OHDR may access
				auto inHeadersFiltered = m_filterOwner.inHeaders;

				// Test for redirection (limited to 3, to prevent infinite loop)
				//if (!rdirToHost.empty() && redirectedIn < 3) {
				//	// (This function will also take care of incoming variables)
				//	connectWebsite();
				//	redirectedIn++;
				//	continue;
				//}

				// Decode new headers to control browser-side beehaviour
				if (CUtil::noCaseContains("close", CFilterOwner::GetHeader(inHeadersFiltered, "Connection")))
					m_sendConnectionClose = true;


				if (CUtil::noCaseContains("gzip", CFilterOwner::GetHeader(inHeadersFiltered, "Content-Encoding"))) {
					CFilterOwner::RemoveHeader(inHeadersFiltered, "Content-Encoding");
					//m_sendContentCoding = 1;
					//if (m_compressor == nullptr) 
					//	m_compressor.reset(new CZlibBuffer());
					//m_compressor->reset(true, true);
				} else if (CUtil::noCaseContains("deflate", CFilterOwner::GetHeader(inHeadersFiltered, "Content-Encoding"))) {
					CFilterOwner::RemoveHeader(inHeadersFiltered, "Content-Encoding");
					//m_sendContentCoding = 2;
					//if (m_compressor == nullptr) 
					//	m_compressor.reset(new CZlibBuffer());
					//m_compressor->reset(true, false);
				}

				if (m_inChunked || m_inSize) {
					// Our output will always be chunked: filtering can
					// change body size. So let's force this header.
					CFilterOwner::SetHeader(inHeadersFiltered, "Transfer-Encoding", "chunked");
					CFilterOwner::RemoveHeader(inHeadersFiltered, "Content-Length");	// Content-Lengthを消さないとおかしい
				}

				// Now we can put everything in the filtered buffer
				// (1.1 is necessary to have the browser understand chunked
				// transfer)
				//if (m_url.getDebug()) {
				//	m_sendInBuf += "HTTP/1.1 200 OK" CRLF
				//		"Content-Type: text/html" CRLF
				//		"Connection: close" CRLF
				//		"Transfer-Encoding: chunked" CRLF CRLF;
				//	std::string buf =
				//		"<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n"
				//		"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\""
				//		" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
				//		"<html>\n<head>\n<title>Source of ";
				//	CUtil::htmlEscape(buf, url.getProtocol() + "://" +
				//						   url.getFromHost());
				//	buf += "</title>\n"
				//		"<link rel=\"stylesheet\" media=\"all\" "
				//		"href=\"http://local.ptron/ViewSource.css\" />\n"
				//		"</head>\n\n<body>\n<div id=\"headers\">\n";
				//	buf += "<div class=\"res\">";
				//	CUtil::htmlEscape(buf, responseLine.ver);
				//	buf += " ";
				//	CUtil::htmlEscape(buf, responseLine.code);
				//	buf += " ";
				//	CUtil::htmlEscape(buf, responseLine.msg);
				//	buf += "</div>\n";
				//	string name;
				//	for (vector<SHeader>::iterator it = inHeadersFiltered.begin();
				//			it != inHeadersFiltered.end(); it++) {
				//		buf += "<div class=\"hdr\">";
				//		CUtil::htmlEscape(buf, it->name);
				//		buf += ": <span class=\"val\">";
				//		CUtil::htmlEscape(buf, it->value);
				//		buf += "</span></div>\n";
				//	}
				//	buf += "</div><div id=\"body\">\n";
				//	useChain = true;
				//	useGifFilter = false;
				//	chain->dataReset();
				//	dataReset();
				//	dataFeed(buf);
				//} else {

				m_sendInBuf = "HTTP/1.1 " + m_responseLine.code + " " + m_responseLine.msg  + CRLF ;
				std::string name;
				for (auto& pair : inHeadersFiltered) {
					m_sendInBuf += pair.first + ": " + pair.second + CRLF;
				}
				CLog::HttpEvent(kLogHttpSendIn, m_ipFromAddress, m_filterOwner.requestNumber, m_sendInBuf);

				m_sendInBuf += CRLF;

				// Tell text filters to see whether they should work on it
				//useChain = (!bypassBody && CSettings::ref().filterText);
				//if (useChain) chain->dataReset(); else dataReset();
				//if (useGifFilter) GIFfilter->dataReset();

				DataReset();

				// File type will be reevaluated using first block of data
				m_filterOwner.fileType.clear();
				//}

				// Decide what to do next
				m_inStep = (m_responseLine.code == "204"  ? STEP_FINISH :
							m_responseLine.code == "304"  ? STEP_FINISH :
							m_responseLine.code[0] == '1' ? STEP_FINISH :
							m_inChunked                   ? STEP_CHUNK  :
							m_inSize                      ? STEP_RAW    :
							STEP_FINISH );
			}
			break;

		// Read  (CRLF) HexRawLength * CRLF before raw data
		// or    zero * CRLF CRLF
		case STEP_CHUNK:
			{
				TRACEIN(_T("ProcessIn #%d : STEP_CHUNK"), m_filterOwner.requestNumber);
				size_t pos, len;
				while (CUtil::endOfLine(m_recvInBuf, 0, pos, len) && pos == 0)
					m_recvInBuf.erase(0,len);

				if (CUtil::endOfLine(m_recvInBuf, 0, pos, len) == false)
					return;
				m_inSize = CUtil::readHex(m_recvInBuf.substr(0,pos));

				if (m_inSize > 0) {
					m_recvInBuf.erase(0, pos+len);
					m_inStep = STEP_RAW;
				} else {
					if (CUtil::endOfLine(m_recvInBuf, 0, pos, len, 2) == false)
						return;
					m_recvInBuf.erase(0, pos + len);
					m_inStep = STEP_FINISH;
					_EndFeeding();
				}
			}
			break;

		// The next inSize bytes are left untouched
		case STEP_RAW:
			{
				int copySize = m_recvInBuf.size();
				if (copySize > m_inSize)
					copySize = m_inSize;

				if (m_inChunked == false && m_psockWebsite->IsConnectionKilledFromPeer())
					m_inSize = 0;
				if (copySize == 0 && m_inSize)
					return ;

				std::string data = m_recvInBuf.substr(0, copySize);
				m_recvInBuf.erase(0, copySize);
				m_inSize -= copySize;

				TRACEIN(_T("ProcessIn #%d : STEP_RAW copySize[%d] inSize(rest)[%d]"), m_filterOwner.requestNumber, copySize, m_inSize);

				// We must decompress compressed data,
				// unless bypassed body with same coding
				if (m_recvContentCoding /*&& (useChain ||
										m_recvContentCoding != m_sendContentCoding)*/) {
					m_decompressor->feed(data);
					m_decompressor->read(data);
				}


				//if (useChain) {
				//	// provide filter chain with raw unfiltered data
				//	chain->dataFeed(data);
				//} else if (useGifFilter) {
				//	// Freeze GIF
				//	GIFfilter->dataFeed(data);
				//} else {

					// In case we bypass the body, we directly send data to
					// the end of chain (the request manager itself)
					DataFeed(data);
					//}

					// If we finished reading as much raw data as was expected,
					// continue to next step (finish or next chunk)
					if (m_inSize == 0) {
						if (m_inChunked) {
							m_inStep = STEP_CHUNK;
						} else {
							m_inStep = STEP_FINISH;
							_EndFeeding();
						}
					}
				//}
			}
			break;

		// Forward incoming data to browser
		case STEP_TUNNELING:
			{
				m_sendInBuf += m_recvInBuf;
				m_recvInBuf.clear();

				// サイトからの接続が切れたらブラウザへ残りのデータを送信してブラウザとの接続を切る
				if (m_psockWebsite->IsConnected() == false || m_psockWebsite->IsConnectionKilledFromPeer()) {
					m_inStep = STEP_FINISH;
					m_outStep = STEP_FINISH;	// サイトとの接続は切れてるのでこっちでやっても問題ないはず。。。
					m_sendConnectionClose = true;
					break;
					//_SendIn();
					//m_psockBrowser->Close();
				}
				return ;
			}
			break;

			// A few things have to be done before we go back to start state...
		case STEP_FINISH:
			{

				TRACEIN(_T("ProcessIn #%d : STEP_FINISH outStep[%d]"), m_filterOwner.requestNumber, m_outStep);
				if (m_outStep != STEP_FINISH && m_outStep != STEP_START)
					return ;
				m_outStep = STEP_START;
				m_inStep = STEP_START;

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
			}
		}
	}




/// Send incoming data to browser
/// ブラウザへレスポンスを送信
bool	CRequestManager::_SendIn()
{
	bool ret = false;
	// Send everything to browser, if connected
	if (m_sendInBuf.size() > 0 && m_psockBrowser->IsConnected()) {
		ret = true;
		if (m_psockBrowser->Write(m_sendInBuf.c_str(), m_sendInBuf.size()) == false)
			m_psockBrowser->Close();	// Trouble with browser's socket, end of all things
		else
			m_sendInBuf.erase(0, m_psockBrowser->GetLastWriteCount());
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
      return true;
    }
    m_filterOwner.fileType = "oth";
    return false;
}



void	CRequestManager::DataFeed(const std::string& data)
{
    if (m_dumped) 
		return;

    size_t size = data.size();
    if (size && m_sendContentCoding
        /*&& (useChain || recvContentCoding != sendContentCoding)*/) {

        // Send a chunk of compressed data
        string tmp;
        m_compressor->feed(data);
        m_compressor->read(tmp);
        size = tmp.size();
        if (size)
            m_sendInBuf += CUtil::makeHex(size) + CRLF + tmp + CRLF;

    } else if (size) {
        // Send a chunk of uncompressed/unchanged data
        m_sendInBuf += CUtil::makeHex(size) + CRLF + data + CRLF;
    }
}

/// Record that the chain emptied itself, finish compression if needed
void	CRequestManager::DataDump()
{
    if (m_dumped) 
		return;
    m_dumped = true;

    if (m_sendContentCoding
        /*&& (useChain || recvContentCoding != sendContentCoding)*/) {

        string tmp;
        m_compressor->dump();
        m_compressor->read(tmp);
        size_t size = tmp.size();
        if (size)
            m_sendInBuf += CUtil::makeHex(size) + CRLF + tmp + CRLF;
    }

    if (m_inStep != STEP_FINISH) {
        // Data has been dumped by a filter, not by this request manager.
        // So we must disconnect the website to stop downloading.
        m_psockWebsite->Close();
    }
}

/* Send the last deflated bytes to the filter chain then dump the chain
 * and append last empty chunk
 */
void	CRequestManager::_EndFeeding() {

    if (m_recvContentCoding /*&& (useChain ||
                recvContentCoding != sendContentCoding)*/) {
        string data;
        m_decompressor->dump();
        m_decompressor->read(data);

        //if (useChain) {
        //    chain->dataFeed(data);
        //} else if (useGifFilter) {
        //    GIFfilter->dataFeed(data);
        //} else {
            DataFeed(data);
        //}
    }
#if 0
    if (useChain) {
        if (url.getDebug())
            dataFeed("\n</div>\n</body>\n</html>\n");
        chain->dataDump();
    } else if (useGifFilter) {
        GIFfilter->dataDump();
    } else 
#endif
	{
        DataDump();
    }
    // Write last chunk (size 0)
    m_sendInBuf += "0" CRLF CRLF;
}

void	CRequestManager::_FakeResponse(const std::string& code)
{
	m_inStep = STEP_FINISH;
	m_sendInBuf = 
        "HTTP/1.1 " + code + CRLF
        "Content-Length: " + "0" + CRLF;
	CLog::HttpEvent(kLogHttpSendIn, m_ipFromAddress, m_filterOwner.requestNumber, m_sendInBuf);
	m_sendInBuf += CRLF;
	m_sendConnectionClose = true;
}

