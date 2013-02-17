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


#include "requestmanager.h"
#include "util.h"
#include "settings.h"
#include "log.h"
#include "const.h"
#include "matcher.h"
#include "giffilter.h"
#include "headerfilter.h"
#include "textbuffer.h"
#include "zlibbuffer.h"
#include <wx/thread.h>
#include <wx/filefn.h>
#include <wx/file.h>
#include <algorithm>
#include <sstream>
#include <map>

using namespace std;

/* Constructor
 */
CRequestManager::CRequestManager() {

    valid = true;
    available = true;
    browser = NULL;
    website = NULL;
    urlMatcher = NULL;
    urlFilter = NULL;
    compressor = NULL;
    decompressor = NULL;
    reqNumber = 0;
    cnxNumber = 0;
    GIFfilter = new CGifFilter(this);
    chain = new CTextBuffer(*this, this);

    // Retrieve filter descriptions to embed
    CSettings& se = CSettings::ref();
    vector<CFilterDescriptor> filters;
    set<int> ids = se.configs[se.currentConfig];
    for (set<int>::iterator it = ids.begin(); it != ids.end(); it++) {
        map<int,CFilterDescriptor>::iterator itm = se.filters.find(*it);
        if (itm != se.filters.end()
                && itm->second.filterType != CFilterDescriptor::TEXT) {
            filters.push_back(itm->second);
        }
    }

    // Sort filters by decreasing priority
    sort(filters.begin(), filters.end());

    // Create header filters
    for (vector<CFilterDescriptor>::iterator it = filters.begin();
                it != filters.end(); it++) {
        if (it->errorMsg.empty()) {
            try {
                if (it->filterType == CFilterDescriptor::HEADOUT)
                    OUTfilters.push_back(new CHeaderFilter(*it, *this));
                else if (it->filterType == CFilterDescriptor::HEADIN)
                    INfilters.push_back(new CHeaderFilter(*it, *this));

            } catch (parsing_exception) {
                // Invalid filters are just ignored
            }
        }
    }

    // Construct bypass-URL matcher
    urlFilter = new CFilter(*this);
    if (!se.bypass.empty()) {
        try {
            urlMatcher = new CMatcher(se.bypass, *urlFilter);
        } catch (parsing_exception e) {
            // Ignore invalid bypass pattern
        }
    }
}


/* Destructor
 */
CRequestManager::~CRequestManager() {

    destroy();

    if (urlMatcher) delete urlMatcher;
    if (urlFilter)  delete urlFilter;
    if (compressor) delete compressor;
    if (decompressor) delete decompressor;

    CUtil::deleteVector<CHeaderFilter>(OUTfilters);
    CUtil::deleteVector<CHeaderFilter>(INfilters);
    delete GIFfilter;
    delete chain;
}


/* Main loop with socket I/O and data processing.
 *
 * Note: One could add  && (!wxThread::This() || !wxThread::This()->TestDestroy())
 * to the two while() conditions, but since we only call wxThread::Destroy()
 * on terminated threads or after calling abort(), the condition
 * browser->IsConnected() is enough
 */
void CRequestManager::manage(wxSocketBase* browser) {

    clock_t stopTime = 0;
    this->browser = browser;
    browser->GetPeer(addr);
    website = new wxSocketClient();
    cnxNumber = ++CLog::ref().numConnections;

    // Reset processing states
    recvOutBuf.clear();
    sendOutBuf.clear();
    outStep = STEP_START;
    recvInBuf.clear();
    sendInBuf.clear();
    inStep  = STEP_START;
    previousHost.clear();

    // Main loop, continues as long as the browser is connected
    while (browser->IsConnected()) {

        // Process only outgoing data
        bool rest = true;
        if (receiveOut()) { rest = false; processOut(); }
        if (sendIn())     { rest = false; }

        if (website->IsConnected()) {
            do {
                // Full processing
                bool rest = true;
                if (sendOut())      { rest = false; }
                if (receiveIn())    { rest = false; processIn(); }
                if (sendIn())       { rest = false; }
                if (receiveOut())   { rest = false; processOut(); }
                if (rest) {
                    if (outStep != STEP_START) {
                        if (stopTime) stopTime = 0;
                    } else {
                        if (!stopTime) stopTime = clock() + SOCKET_EXPIRATION *
                                                  CLOCKS_PER_SEC;
                        if (!valid || clock() > stopTime) browser->Close();
                    }
                    wxThread::Sleep(5);
                }
            } while (website->IsConnected() && browser->IsConnected());

            // Terminate feeding browser
            receiveIn();
            processIn();
            if (inStep == STEP_RAW || inStep == STEP_CHUNK) {
                inStep = STEP_FINISH;
                endFeeding();
                processIn();
            }
            sendIn();
            if (outStep == STEP_FINISH) {
                // The website aborted connection before sending the body
                sendInBuf =
                    "HTTP/1.1 503 Service Unavailable" CRLF;
                CLog::ref().logHttpEvent(pmEVT_HTTP_TYPE_SENDIN, addr,
                                         reqNumber, sendInBuf);
                sendInBuf += CRLF;
                sendIn();
                browser->Close();
            }
        }
        if (rest) {
            if (outStep != STEP_START) {
                if (stopTime) stopTime = 0;
            } else {
                if (!stopTime) stopTime = clock() + SOCKET_EXPIRATION *
                                          CLOCKS_PER_SEC;
                if (!valid || clock() > stopTime) browser->Close();
            }
            wxThread::Sleep(50);
        }
    }
    destroy();
}


/* Close the browser connection, if connected.
 * At next execution loop in the dedicated thread, the
 * sockets will be destroyed and the statistics updated.
 */
void CRequestManager::abort() {

    if (browser && browser->IsConnected()) {
        browser->Close();
        while (browser) wxThread::Sleep(5);
    }
}


/* Destroy sockets and update statistics
 */
void CRequestManager::destroy() {

    if (!browser) return; // (already done)

    // Update statistics
    --CLog::ref().numOpenSockets;
    CLog::ref().logProxyEvent(pmEVT_PROXY_TYPE_ENDSOCK, addr);

    // If we were processing outgoing or incoming
    // data, we were an active request
    if (outStep != STEP_START || inStep != STEP_START) {
        --CLog::ref().numActiveRequests;
        CLog::ref().logProxyEvent(pmEVT_PROXY_TYPE_ENDREQ, addr);
    }

    // Destroy sockets
    browser->Close();
    website->Close();
    browser->Destroy();
    website->Destroy();
    browser = NULL;
    website = NULL;
}


/* Receive outgoing data from browser.
 * Fixed: using the wxSOCKET_NONE flag in combination with WaitForRead()
 * fixes a severe bug when loading pages from certain websites, the pages
 * of which were truncated in the middle.
 */
bool CRequestManager::receiveOut() {

    // Read socket by blocks
    char buf[CRM_READSIZE];
    bool dataReceived = false;
    // Loop as long as we can read something from the socket
    browser->SetFlags(wxSOCKET_BLOCK | wxSOCKET_NONE);
    while (browser->WaitForRead(0,0)) {
        // Read the socket
        browser->Read(buf, CRM_READSIZE);
        // if we read 0 byte, stop for now
        int count = browser->LastCount();
        if (count == 0) break;
        // put the data into the buffer
        recvOutBuf += string(buf, count);
        dataReceived = true;
    }
    return dataReceived;
}


/* Send outgoing data to website
 */
bool CRequestManager::sendOut() {

    bool ret = false;
    // Send everything to website, if connected
    if (!sendOutBuf.empty() && website->IsConnected() /* && inStep == STEP_START */) {
        ret = true;
        website->SetFlags(wxSOCKET_WAITALL);
        website->Write(sendOutBuf.c_str(), sendOutBuf.size());
        sendOutBuf.erase(0, website->LastCount());
        if (website->Error()) {
            // Trouble with website's socket
            website->Close();
        }
    }
    return ret;
}


/* Receive incoming data from website
 * Fixed: using the wxSOCKET_NONE flag in combination with WaitForRead()
 * fixes a severe bug when loading pages from certain websites, the pages
 * of which were truncated in the middle.
 */
bool CRequestManager::receiveIn() {

    // Read socket by blocks
    char buf[CRM_READSIZE];
    bool dataReceived = false;
    // Loop as long as we can read something from the socket
    website->SetFlags(wxSOCKET_BLOCK | wxSOCKET_NONE);
    while (website->WaitForRead(0,0)) {
        // Read the socket
        website->Read(buf, CRM_READSIZE);
        // if we read 0 byte, stop for now
        int count = website->LastCount();
        if (count == 0) break;
        // put the data into the buffer
        recvInBuf += string(buf, count);
        dataReceived = true;
    }
    return dataReceived;
}


/* Send incoming data to browser
 */
bool CRequestManager::sendIn() {

    bool ret = false;
    // Send everything to browser, if connected
    if (!sendInBuf.empty() && browser->IsConnected()) {
        ret = true;
        browser->SetFlags(wxSOCKET_WAITALL);
        browser->Write(sendInBuf.c_str(), sendInBuf.size());
        sendInBuf.erase(0, browser->LastCount());
        if (browser->Error()) {
            // Trouble with browser's socket, end of all things
            browser->Close();
        }
    }
    return ret;
}


/* Buffer data went through filter chain, make a chunk
 */
void CRequestManager::dataFeed(const string& data) {

    if (dumped) return;

    size_t size = data.size();
    if (size && sendContentCoding
        && (useChain || recvContentCoding != sendContentCoding)) {

        // Send a chunk of compressed data
        string tmp;
        compressor->feed(data);
        compressor->read(tmp);
        size = tmp.size();
        if (size)
            sendInBuf += CUtil::makeHex(size) + CRLF + tmp + CRLF;

    } else if (size) {

        // Send a chunk of uncompressed/unchanged data
        sendInBuf += CUtil::makeHex(size) + CRLF + data + CRLF;
    }
}


/* Record that the chain emptied itself, finish compression if needed
 */
void CRequestManager::dataDump() {

    if (dumped) return;
    dumped = true;

    if (sendContentCoding
        && (useChain || recvContentCoding != sendContentCoding)) {

        string tmp;
        compressor->dump();
        compressor->read(tmp);
        size_t size = tmp.size();
        if (size)
            sendInBuf += CUtil::makeHex(size) + CRLF + tmp + CRLF;
    }

    if (inStep != STEP_FINISH) {
        // Data has been dumped by a filter, not by this request manager.
        // So we must disconnect the website to stop downloading.
        website->Close();
    }
}


/* Record the fact that forwarding processed data is possible, until next dump
 */
void CRequestManager::dataReset() {
    dumped = false;
}


/* Determine if a MIME type can go through body filters.  Also set $TYPE()
 * accordingly.
 */
bool CRequestManager::verifyContentType(string& ctype) {
    size_t end = ctype.find(';');
    if (end == string::npos)  end = ctype.size();

    // Remove top-level type, e.g., text/html -> html
    size_t slash = ctype.find('/');
    if (slash == string::npos || slash >= end) {
      fileType = "oth";
      return false;
    }
    slash++;

    // Strip off any x- before the type (e.g., x-javascript)
    if (ctype[slash] == 'x' && ctype[slash + 1] == '-')
      slash += 2;
    string type = ctype.substr(slash, end - slash);
    CUtil::lower(type);

    if (type == "html") {
      fileType = "htm";
      return true;
    } else if (type == "javascript") {
      fileType = "js";
      return true;
    } else if (type == "css") {
      fileType = "css";
      return true;
    } else if (type == "vbscript") {
      fileType = "vbs";
      return true;
    }
    fileType = "oth";
    return false;
}


/* Process outgoing data (from browser to website)
 */
void CRequestManager::processOut() {

    // We will exit from a step that has not enough data to complete
    while (1) {

        // Marks the beginning of a request
        if (outStep == STEP_START) {

            // We need something in the buffer
            if (recvOutBuf.empty()) return;
            outStep = STEP_FIRSTLINE;

            // Update active request stat, but if processIn() did it already
            if (inStep == STEP_START) {
                ++CLog::ref().numActiveRequests;
                CLog::ref().logProxyEvent(pmEVT_PROXY_TYPE_NEWREQ, addr);
            }

            // Reset variables relative to a single request/response
            bypassOut = false;
            bypassIn = false;
            bypassBody = false;
            bypassBodyForced = false;
            killed = false;
            variables.clear();
            reqNumber = ++CLog::ref().numRequests;
            rdirMode = 0;
            rdirToHost.clear();
            redirectedIn = 0;
            recvConnectionClose = sendConnectionClose = false;
            recvContentCoding = sendContentCoding = 0;
            logPostData.clear();
        }

        // Read first HTTP request line
        else if (outStep == STEP_FIRSTLINE) {

            // Do we have the full first line yet?
            size_t pos, len;
            if (!CUtil::endOfLine(recvOutBuf, 0, pos, len)) return;

            // Get it and record it
            size_t p1 = recvOutBuf.find_first_of(" ");
            size_t p2 = recvOutBuf.find_first_of(" ", p1+1);
            requestLine.method = recvOutBuf.substr(0,    p1);
            requestLine.url    = recvOutBuf.substr(p1+1, p2 -p1-1);
            requestLine.ver    = recvOutBuf.substr(p2+1, pos-p2-1);
            logRequest = recvOutBuf.substr(0, pos+len);
            recvOutBuf.erase(0, pos+len);

            // Next step will be to read headers
            outStep = STEP_HEADERS;

            // Unless we have Content-Length or Transfer-Encoding headers,
            // we won't expect anything after headers.
            outSize = 0;
            outChunked = false;
            outHeaders.clear();
            // (in case a silly OUT filter wants to read IN headers)
            inHeaders.clear();
            responseCode.clear();
        }

        // Read and process headers, as long as there are any
        else if (outStep == STEP_HEADERS) {

            while (true) {
                // Look for end of line
                size_t pos, len;
                if (!CUtil::endOfLine(recvOutBuf, 0, pos, len)) return;

                // Check if we reached the empty line
                if (pos == 0) {
                    recvOutBuf.erase(0, len);
                    outStep = STEP_DECODE;
                    break;
                }

                // Find the header end
                while (   pos+len >= recvOutBuf.size()
                       || recvOutBuf[pos+len] == ' '
                       || recvOutBuf[pos+len] == '\t' ) {
                    if (!CUtil::endOfLine(recvOutBuf, pos+len, pos, len)) return;
                }

                // Record header
                size_t colon = recvOutBuf.find(':');
                if (colon != string::npos) {
                    string name = recvOutBuf.substr(0, colon);
                    string value = recvOutBuf.substr(colon+1, pos-colon-1);
                    CUtil::trim(value);
                    SHeader header = {name, value};
                    outHeaders.push_back(header);
                }
                logRequest += recvOutBuf.substr(0, pos+len);
                recvOutBuf.erase(0, pos+len);
            }
        }

        // Decode and process headers
        else if (outStep == STEP_DECODE) {

            CLog::ref().logHttpEvent(pmEVT_HTTP_TYPE_RECVOUT, addr,
                                      reqNumber, logRequest);

            // Get the URL and the host to contact (unless we use a proxy)
            url.parseUrl(requestLine.url);
            if (url.getBypassIn())    bypassIn   = true;
            if (url.getBypassOut())   bypassOut  = true;
            if (url.getBypassText())  bypassBody = true;
            useSettingsProxy = CSettings::ref().useNextProxy;
            contactHost = url.getHostPort();

            // Test URL with bypass-URL matcher, if matches we'll bypass all
            if (urlMatcher) {
                urlFilter->clearMemory();
                const char *start = url.getFromHost().c_str();
                const char *stop  = start + url.getFromHost().size();
                const char *end, *reached;
                if (urlMatcher->match(start, stop, end, reached)) {
                    bypassOut = bypassIn = bypassBody = true;
                }
            }

            // We must read the Content-Length and Transfer-Encoding
            // headers to know if there is POSTed data
            if (CUtil::noCaseContains("chunked",
                        getHeader(outHeaders, "Transfer-Encoding"))) {
                outChunked = true;
            }
            if (!getHeader(outHeaders, "Content-Length").empty()) {
                stringstream ss(getHeader(outHeaders, "Content-Length"));
                ss >> outSize;
            }

            // We'll work on a copy, since we don't want to alter
            // the real headers that $IHDR and $OHDR may access
            vector<SHeader> outHeadersFiltered = outHeaders;

            // Filter outgoing headers
            if (!bypassOut && CSettings::ref().filterOut
                           && url.getHost() != "local.ptron") {

                // Apply filters one by one
                for (vector<CHeaderFilter*>::iterator itf =
                        OUTfilters.begin(); itf != OUTfilters.end(); itf++) {

                    (*itf)->bypassed = false;
                    string name = (*itf)->headerName;

                    if (!CUtil::noCaseBeginsWith("url", name)) {

                        // If header is absent, temporarily create one
                        if (getHeader(outHeadersFiltered, name).empty())
                            setHeader(outHeadersFiltered, name, "");
                        // Test headers one by one
                        for (vector<SHeader>::iterator ith =
                                outHeadersFiltered.begin();
                                ith != outHeadersFiltered.end(); ith++) {
                            if (CUtil::noCaseEqual(name, ith->name))
                                (*itf)->filter(ith->value);
                        }
                        // Remove null headers
                        cleanHeaders(outHeadersFiltered);

                    } else {

                        // filter works on a copy of the URL
                        string test = url.getUrl();
                        (*itf)->filter(test);
                        CUtil::trim(test);
                        // if filter changed the url, update variables
                        if (!killed && !test.empty() && test != url.getUrl()) {
                            // We won't change contactHost if it has been
                            // set to a proxy address by a $SETPROXY command
                            bool changeHost = (contactHost == url.getHostPort());
                            // update URL
                            url.parseUrl(test);
                            if (url.getBypassIn())    bypassIn   = true;
                            if (url.getBypassOut())   bypassOut  = true;
                            if (url.getBypassText())  bypassBody = true;
                            if (changeHost) contactHost = url.getHostPort();
                        }
                    }

                    if (killed) {
                        // There has been a \k in a header filter, so we
                        // redirect to an empty file and stop processing headers
                        if (url.getPath().find(".gif") != string::npos)
                            rdirToHost = "http://file//./html/killed.gif";
                        else
                            rdirToHost = "http://file//./html/killed.html";
                        break;
                    }
                }
            }

            // If we haven't connected to this host yet, we do it now.
            // This will allow incoming processing to start. If the
            // connection fails, incoming processing will jump to
            // the finish state, so that we can accept other requests
            // on the same browser socket (persistent connection).
            connectWebsite();

            // If website is ready, send headers
            if (inStep == STEP_START) {

                // Update URL within request
                setHeader(outHeadersFiltered, "Host", url.getHost());
                removeHeader(outHeadersFiltered, "Proxy-Connection"); // (must do)
                if (contactHost == url.getHostPort())
                    requestLine.url = url.getAfterHost();
                else
                    requestLine.url = url.getUrl();
                if (requestLine.url.empty()) requestLine.url = "/";

                // Now we can put everything in the filtered buffer
                sendOutBuf = requestLine.method + " " +
                             requestLine.url    + " " +
                             requestLine.ver    + CRLF ;
                string name;
                for (vector<SHeader>::iterator it = outHeadersFiltered.begin();
                        it != outHeadersFiltered.end(); it++) {
                    sendOutBuf += it->name + ": " + it->value + CRLF;
                }
                // Log outgoing headers
                CLog::ref().logHttpEvent(pmEVT_HTTP_TYPE_SENDOUT, addr,
                                         reqNumber, sendOutBuf);
                sendOutBuf += CRLF;
            }

            // Decide next step
            if (requestLine.method == "HEAD") {
                // HEAD requests have no body, even if a size is provided
                outStep = STEP_FINISH;
            } else if (requestLine.method == "CONNECT") {
                // SSL Tunneling (we won't process exchanged data)
                outStep = STEP_TUNNELING;
            } else {
                outStep = (outChunked ? STEP_CHUNK : outSize ? STEP_RAW : STEP_FINISH);
            }
        }

        // Read CRLF HexRawLength * CRLF before raw data
        // or CRLF zero * CRLF CRLF
        else if (outStep == STEP_CHUNK) {

            size_t pos, len;
            while (CUtil::endOfLine(recvOutBuf, 0, pos, len) && pos == 0) {
                sendOutBuf += CRLF;
                recvOutBuf.erase(0,len);
            }

            if (!CUtil::endOfLine(recvOutBuf, 0, pos, len)) return;
            outSize = CUtil::readHex(recvOutBuf.substr(0,pos));

            if (outSize > 0) {
                sendOutBuf += recvOutBuf.substr(0,pos) + CRLF;
                recvOutBuf.erase(0, pos+len);
                outStep = STEP_RAW;
            } else {
                if (!CUtil::endOfLine(recvOutBuf, 0, pos, len, 2)) return;
                sendOutBuf += recvOutBuf.substr(0,pos) + CRLF CRLF;
                recvOutBuf.erase(0, pos+len);
                CLog::ref().logHttpEvent(pmEVT_HTTP_TYPE_POSTOUT, addr,
                                         reqNumber, logPostData);
                outStep = STEP_FINISH;
            }

            // We shouldn't send anything to website if it is not expecting
            // data (i.e we obtained a faked response)
            if (inStep == STEP_FINISH) sendOutBuf.clear();
        }

        // The next outSize bytes are left untouched
        else if (outStep == STEP_RAW) {

            int copySize = recvOutBuf.size();
            if (copySize > outSize) copySize = outSize;
            if (!copySize && outSize) return;

            string postData = recvOutBuf.substr(0, copySize);
            sendOutBuf += postData;
            logPostData += postData;
            recvOutBuf.erase(0, copySize);
            outSize -= copySize;

            // If we finished reading as much raw data as was expected,
            // continue to next step (finish or next chunk)
            if (!outSize) {
                if (outChunked) {
                    outStep = STEP_CHUNK;
                } else {
                    CLog::ref().logHttpEvent(pmEVT_HTTP_TYPE_POSTOUT, addr,
                                             reqNumber, logPostData);
                    outStep = STEP_FINISH;
                }
            }

            // We shouldn't send anything to website if it is not expecting
            // data (i.e we obtained a faked response)
            if (inStep == STEP_FINISH) sendOutBuf.clear();
        }

        // Forward outgoing data to website
        else if (outStep == STEP_TUNNELING) {

            sendOutBuf += recvOutBuf;
            recvOutBuf.clear();
            return;
        }

        // We'll wait for response completion
        else if (outStep == STEP_FINISH) {

            if (inStep != STEP_FINISH) return;

            outStep = STEP_START;
            inStep = STEP_START;

            // Update active request stat
            --CLog::ref().numActiveRequests;
            CLog::ref().logProxyEvent(pmEVT_PROXY_TYPE_ENDREQ, addr);

            // Disconnect browser if we sent it "Connection:close"
            // If manager has been set invalid, close connection too
            if (sendConnectionClose || !valid) {
                sendIn();
                browser->Close();
            }

            // Disconnect website if it sent us "Connection:close"
            if (recvConnectionClose || browser->IsDisconnected()) {
                website->Close();
            }
        }
    }
}


/* Process incoming data (from website to browser)
 */
void CRequestManager::processIn() {

    // We will exit from a step that has not enough data to complete
    while (1) {

        // Marks the beginning of a request
        if (inStep == STEP_START) {

            // We need something in the buffer
            if (recvInBuf.empty()) return;
            inStep = STEP_FIRSTLINE;
            // Update active request stat, but if processOut() did it already
            if (outStep == STEP_START) {
                ++CLog::ref().numActiveRequests;
                CLog::ref().logProxyEvent(pmEVT_PROXY_TYPE_NEWREQ, addr);
            }
        }

        // Read first HTTP response line
        else if (inStep == STEP_FIRSTLINE) {

            // Do we have the full first line yet?
            size_t pos, len;
            if (!CUtil::endOfLine(recvInBuf, 0, pos, len)) return;

            // Parse it
            size_t p1 = recvInBuf.find_first_of(" ");
            size_t p2 = recvInBuf.find_first_of(" ", p1+1);
            responseLine.ver  = recvInBuf.substr(0,p1);
            responseLine.code = recvInBuf.substr(p1+1, p2-p1-1);
            responseLine.msg  = recvInBuf.substr(p2+1, pos-p2-1);
            responseCode = recvInBuf.substr(p1+1, pos-p1-1);

            // Remove it from receive-in buffer.
            // we don't place it immediately on send-in buffer,
            // as we may be willing to fake it (cf $REDIR)
            logResponse = recvInBuf.substr(0, pos+len);
            recvInBuf.erase(0, pos+len);

            // Next step will be to read headers
            inStep = STEP_HEADERS;

            // Unless we have Content-Length or Transfer-Encoding headers,
            // we won't expect anything after headers.
            inSize = 0;
            inChunked = false;
            inHeaders.clear();
            responseCode.clear();
            recvConnectionClose = sendConnectionClose = false;
            recvContentCoding = sendContentCoding = 0;
            useGifFilter = false;
        }

        // Read and process headers, as long as there are any
        else if (inStep == STEP_HEADERS) {

            while (true) {
                // Look for end of line
                size_t pos, len;
                if (!CUtil::endOfLine(recvInBuf, 0, pos, len)) return;

                // Check if we reached the empty line
                if (pos == 0) {
                    recvInBuf.erase(0, len);
                    inStep = STEP_DECODE;
                    // In case of 'HTTP 100 Continue,' expect another set of
                    // response headers before the data
                    if (responseLine.code == "100") {
                        inStep = STEP_FIRSTLINE;
                        responseLine.ver.clear();
                        responseLine.code.clear();
                        responseLine.msg.clear();
                        responseCode.clear();
                    }
                    break;
                }

                // Find the header end
                while (   pos+len >= recvInBuf.size()
                       || recvInBuf[pos+len] == ' '
                       || recvInBuf[pos+len] == '\t' ) {
                    if (!CUtil::endOfLine(recvInBuf, pos+len, pos, len)) return;
                }

                // Record header
                size_t colon = recvInBuf.find(':');
                if (colon != string::npos) {
                    string name = recvInBuf.substr(0, colon);
                    string value = recvInBuf.substr(colon+1, pos-colon-1);
                    CUtil::trim(value);
                    SHeader header = {name, value};
                    inHeaders.push_back(header);
                }
                logResponse += recvInBuf.substr(0, pos+len);
                recvInBuf.erase(0, pos+len);
            }
        }

        // Decode and process headers
        else if (inStep == STEP_DECODE) {

            CLog::ref().logHttpEvent(pmEVT_HTTP_TYPE_RECVIN, addr,
                                      reqNumber, logResponse);

            // We must read the Content-Length and Transfer-Encoding
            // headers to know body length
            if (CUtil::noCaseContains("chunked",
                        getHeader(inHeaders, "Transfer-Encoding"))) {
                inChunked = true;
            }
            if (!getHeader(inHeaders, "Content-Length").empty()) {
                stringstream ss(getHeader(inHeaders, "Content-Length"));
                ss >> inSize;
            }

            if (!getHeader(inHeaders, "Content-Type").empty()) {
                // If size is not given, we'll read body until connection closes
                if (getHeader(inHeaders, "Content-Length").empty()) {
                    inSize = BIG_NUMBER;
                }
                // Check for filterable MIME types
                if (!verifyContentType(getHeader(inHeaders, "Content-Type"))
                            && !bypassBodyForced) {
                    bypassBody = true;
                }
                // Check for GIF to freeze
                if (CUtil::noCaseContains("image/gif",
                            getHeader(inHeaders, "Content-Type"))) {
                    useGifFilter = CSettings::ref().filterGif;
                }
            }

            if (CUtil::noCaseContains("close", getHeader(inHeaders, "Connection"))
                        || CUtil::noCaseBeginsWith("HTTP/1.0", responseLine.ver)) {
                recvConnectionClose = true;
            }

            if (CUtil::noCaseContains("gzip",
                        getHeader(inHeaders, "Content-Encoding"))) {
                recvContentCoding = 1;
                if (!decompressor) decompressor = new CZlibBuffer();
                decompressor->reset(false, true);
            } else if (CUtil::noCaseContains("deflate",
                        getHeader(inHeaders, "Content-Encoding"))) {
                recvContentCoding = 2;
                if (!decompressor) decompressor = new CZlibBuffer();
                decompressor->reset(false, false);
            } else if (CUtil::noCaseContains("compress",
                        getHeader(inHeaders, "Content-Encoding"))) {
                bypassBody = true;
            }

            if (CUtil::noCaseBeginsWith("206", responseCode)) {
                bypassBody = true;
            }

            // We'll work on a copy, since we don't want to alter
            // the real headers that $IHDR and $OHDR may access
            vector<SHeader> inHeadersFiltered = inHeaders;

            if (!bypassIn && CSettings::ref().filterIn) {

                // Apply filters one by one
                for (vector<CHeaderFilter*>::iterator itf =
                        INfilters.begin(); itf != INfilters.end(); itf++) {

                    (*itf)->bypassed = false;
                    string name = (*itf)->headerName;

                    // If header is absent, temporarily create one
                    if (getHeader(inHeadersFiltered, name).empty()) {
                        if (CUtil::noCaseBeginsWith("url", name)) {
                            setHeader(inHeadersFiltered, name, url.getUrl());
                        } else {
                            setHeader(inHeadersFiltered, name, "");
                        }
                    }
                    // Test headers one by one
                    for (vector<SHeader>::iterator ith =
                            inHeadersFiltered.begin();
                            ith != inHeadersFiltered.end(); ith++) {
                        if (CUtil::noCaseEqual(name, ith->name))
                            (*itf)->filter(ith->value);
                    }
                    // Remove null headers
                    cleanHeaders(inHeadersFiltered);

                    if (killed) {
                        // There has been a \k in a header filter, so we
                        // redirect to an empty file and stop processing headers
                        if (url.getPath().find(".gif") != string::npos)
                            rdirToHost = "http://file//./html/killed.gif";
                        else
                            rdirToHost = "http://file//./html/killed.html";
                        rdirMode = 0;   // (to use non-transp code below)
                        break;
                    }
                }
            }

            // Test for redirection (limited to 3, to prevent infinite loop)
            if (!rdirToHost.empty() && redirectedIn < 3) {
                // (This function will also take care of incoming variables)
                connectWebsite();
                redirectedIn++;
                continue;
            }

            // Decode new headers to control browser-side beehaviour
            if (CUtil::noCaseContains("close",
                        getHeader(inHeadersFiltered, "Connection"))) {
                sendConnectionClose = true;
            }
            if (CUtil::noCaseContains("gzip",
                        getHeader(inHeadersFiltered, "Content-Encoding"))) {
                sendContentCoding = 1;
                if (!compressor) compressor = new CZlibBuffer();
                compressor->reset(true, true);
            } else if (CUtil::noCaseContains("deflate",
                        getHeader(inHeadersFiltered, "Content-Encoding"))) {
                sendContentCoding = 2;
                if (!compressor) compressor = new CZlibBuffer();
                compressor->reset(true, false);
            }

            if (inChunked || inSize) {
                // Our output will always be chunked: filtering can
                // change body size. So let's force this header.
                setHeader(inHeadersFiltered, "Transfer-Encoding", "chunked");
            }

            // Now we can put everything in the filtered buffer
            // (1.1 is necessary to have the browser understand chunked
            // transfer)
            if (url.getDebug()) {
                sendInBuf += "HTTP/1.1 200 OK" CRLF
                    "Content-Type: text/html" CRLF
                    "Connection: close" CRLF
                    "Transfer-Encoding: chunked" CRLF CRLF;
                string buf =
                    "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n"
                    "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\""
                    " \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
                    "<html>\n<head>\n<title>Source of ";
                CUtil::htmlEscape(buf, url.getProtocol() + "://" +
                                       url.getFromHost());
                buf += "</title>\n"
                    "<link rel=\"stylesheet\" media=\"all\" "
                    "href=\"http://local.ptron/ViewSource.css\" />\n"
                    "</head>\n\n<body>\n<div id=\"headers\">\n";
                buf += "<div class=\"res\">";
                CUtil::htmlEscape(buf, responseLine.ver);
                buf += " ";
                CUtil::htmlEscape(buf, responseLine.code);
                buf += " ";
                CUtil::htmlEscape(buf, responseLine.msg);
                buf += "</div>\n";
                string name;
                for (vector<SHeader>::iterator it = inHeadersFiltered.begin();
                        it != inHeadersFiltered.end(); it++) {
                    buf += "<div class=\"hdr\">";
                    CUtil::htmlEscape(buf, it->name);
                    buf += ": <span class=\"val\">";
                    CUtil::htmlEscape(buf, it->value);
                    buf += "</span></div>\n";
                }
                buf += "</div><div id=\"body\">\n";
                useChain = true;
                useGifFilter = false;
                chain->dataReset();
                dataReset();
                dataFeed(buf);
            } else {
                sendInBuf = "HTTP/1.1 " +
                            responseLine.code + " " +
                            responseLine.msg  + CRLF ;
                string name;
                for (vector<SHeader>::iterator it = inHeadersFiltered.begin();
                        it != inHeadersFiltered.end(); it++) {
                    sendInBuf += it->name + ": " + it->value + CRLF;
                }
                CLog::ref().logHttpEvent(pmEVT_HTTP_TYPE_SENDIN, addr,
                                         reqNumber, sendInBuf);
                sendInBuf += CRLF;

                // Tell text filters to see whether they should work on it
                useChain = (!bypassBody && CSettings::ref().filterText);
                if (useChain) chain->dataReset(); else dataReset();
                if (useGifFilter) GIFfilter->dataReset();

                // File type will be reevaluated using first block of data
                fileType.clear();
            }

            // Decide what to do next
            inStep = (responseLine.code == "204"  ? STEP_FINISH :
                      responseLine.code == "304"  ? STEP_FINISH :
                      responseLine.code[0] == '1' ? STEP_FINISH :
                      inChunked                   ? STEP_CHUNK  :
                      inSize                      ? STEP_RAW    :
                                                    STEP_FINISH );
        }

        // Read  (CRLF) HexRawLength * CRLF before raw data
        // or    zero * CRLF CRLF
        else if (inStep == STEP_CHUNK) {

            size_t pos, len;
            while (CUtil::endOfLine(recvInBuf, 0, pos, len) && pos == 0)
                recvInBuf.erase(0,len);

            if (!CUtil::endOfLine(recvInBuf, 0, pos, len)) return;
            inSize = CUtil::readHex(recvInBuf.substr(0,pos));

            if (inSize > 0) {
                recvInBuf.erase(0, pos+len);
                inStep = STEP_RAW;
            } else {
                if (!CUtil::endOfLine(recvInBuf, 0, pos, len, 2)) return;
                recvInBuf.erase(0, pos+len);
                inStep = STEP_FINISH;
                endFeeding();
            }
        }

        // The next inSize bytes are left untouched
        else if (inStep == STEP_RAW) {

            int copySize = recvInBuf.size();
            if (copySize > inSize) copySize = inSize;
            if (!copySize && inSize) return;

            string data = recvInBuf.substr(0, copySize);
            recvInBuf.erase(0, copySize);
            inSize -= copySize;

            // We must decompress compressed data,
            // unless bypassed body with same coding
            if (recvContentCoding && (useChain ||
                        recvContentCoding != sendContentCoding)) {
                decompressor->feed(data);
                decompressor->read(data);
            }

            if (useChain) {
                // provide filter chain with raw unfiltered data
                chain->dataFeed(data);
            } else if (useGifFilter) {
                // Freeze GIF
                GIFfilter->dataFeed(data);
            } else {
                // In case we bypass the body, we directly send data to
                // the end of chain (the request manager itself)
                dataFeed(data);
            }

            // If we finished reading as much raw data as was expected,
            // continue to next step (finish or next chunk)
            if (!inSize) {
                if (inChunked) {
                    inStep = STEP_CHUNK;
                } else {
                    inStep = STEP_FINISH;
                    endFeeding();
                }
            }
        }

        // Forward incoming data to browser
        else if (inStep == STEP_TUNNELING) {

            sendInBuf += recvInBuf;
            recvInBuf.clear();
            return;
        }

        // A few things have to be done before we go back to start state...
        else if (inStep == STEP_FINISH) {

            if (outStep != STEP_FINISH && outStep != STEP_START) return;
            outStep = STEP_START;
            inStep = STEP_START;

            // Update active request stat
            --CLog::ref().numActiveRequests;
            CLog::ref().logProxyEvent(pmEVT_PROXY_TYPE_ENDREQ, addr);

            // Disconnect browser if we sent it "Connection:close"
            // If manager has been set invalid, close connection too
            if (sendConnectionClose || !valid) {
                sendIn();
                browser->Close();
            }

            // Disconnect website if it sent us "Connection:close"
            if (recvConnectionClose || browser->IsDisconnected()) {
                website->Close();
            }
        }
    }
}


/* Connect to the website or to the proxy.
 * If connection fails, a custom HTTP response is sent to the browser
 * and incoming processing continues to STEP_FINISH, as if the website
 * terminated responding.
 * This functions waits for the connection to succeed or fail.
 */
void CRequestManager::connectWebsite() {

    // If download was on, disconnect (to avoid downloading genuine document)
    if (inStep == STEP_DECODE) {
        website->Close();
        inStep = STEP_START;
        bypassBody = bypassBodyForced = false;
    }

    receiveIn();
    recvInBuf.clear();
    sendInBuf.clear();

    // Test for "local.ptron" host
    if (url.getHost() == "local.ptron") {
        rdirToHost = url.getUrl();
    }
    if (CUtil::noCaseBeginsWith("http://local.ptron", rdirToHost)) {
        rdirToHost = "http://file//./html" + CUrl(rdirToHost).getPath();
    }

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

    // If we must contact the host via the settings' proxy,
    // we now override the contactHost
    if (useSettingsProxy && !CSettings::ref().nextProxy.empty()) {
        contactHost = CSettings::ref().nextProxy;
    }

    // Unless we are already connected to this host, we try and connect now
    if (previousHost != contactHost || website->IsDisconnected()) {

        // Disconnect from previous host
        website->Close();
        previousHost = contactHost;

        // The host string is composed of host and port
        string name = contactHost, port;
        size_t colon = name.find(':');
        if (colon != string::npos) {    // (this should always happen)
            port = name.substr(colon+1);
            name = name.substr(0,colon);
        }
        if (port.empty()) port = "80";

        // Check the host (Hostname() asks the DNS)
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

        // Connect
        for (int i=0, time=0; website->IsDisconnected() && i<5 && time<1; i++) {
            if (!website->Connect(host, false)) {
                while (!website->WaitOnConnect(1,0)) {
                    if (browser->IsDisconnected()) return;
                    time++;
                }
            }
        }
        if (website->IsDisconnected()) {
            // Connection failed, warn the browser
            fakeResponse("503 Service Unavailable", "./html/error.html", true,
                         CSettings::ref().getMessage("503_UNAVAILABLE"),
                         contactHost);
            return;
        }
        if (requestLine.method == "CONNECT") {
            // for CONNECT method, incoming data is encrypted from start
            sendInBuf = "HTTP/1.0 200 Connection established" CRLF
                        "Proxy-agent: " APP_NAME " " APP_VERSION CRLF CRLF;
            CLog::ref().logHttpEvent(pmEVT_HTTP_TYPE_SENDIN, addr,
                                     reqNumber, sendInBuf);
            inStep = STEP_TUNNELING;
        }
    }

    // Prepare and send a simple request, if we did a transparent redirection
    if (outStep != STEP_DECODE && inStep == STEP_START) {
        if (contactHost == url.getHostPort())
            requestLine.url = url.getAfterHost();
        else
            requestLine.url = url.getUrl();
        if (requestLine.url.empty()) requestLine.url = "/";
        sendOutBuf =
            requestLine.method + " " + requestLine.url + " HTTP/1.1" CRLF
            "Host: " + url.getHost() + CRLF;
        CLog::ref().logHttpEvent(pmEVT_HTTP_TYPE_SENDOUT, addr,
                                 reqNumber, sendOutBuf);
        sendOutBuf += logPostData + CRLF;
        // Send request
        sendOut();
    }
}


/* Generate a fake response (headers + body). The body comes from a
 * file in the local directory system.
 * The function accepts replacement strings for the content of the
 * file, where tokens are %%1%%, %%2%% and %%3%%.
 */
void CRequestManager::fakeResponse(string code, string filename, bool replace,
                                   string str1, string str2, string str3) {
    string content = CUtil::getFile(filename);
    if (replace) {
        content = CUtil::replaceAll(content, "%%1%%", str1);
        content = CUtil::replaceAll(content, "%%2%%", str2);
        content = CUtil::replaceAll(content, "%%3%%", str3);
    }
    stringstream ss;
    ss << content.length();
    inStep = STEP_FINISH;
    sendInBuf =
        "HTTP/1.1 " + code + CRLF
        "Content-Type: " + CUtil::getMimeType(filename) + CRLF
        "Content-Length: " + ss.str() + CRLF;
    CLog::ref().logHttpEvent(pmEVT_HTTP_TYPE_SENDIN, addr,
                             reqNumber, sendInBuf);
    sendInBuf += CRLF + content;
    sendConnectionClose = true;
}


/* Send the last deflated bytes to the filter chain then dump the chain
 * and append last empty chunk
 */
void CRequestManager::endFeeding() {

    if (recvContentCoding && (useChain ||
                recvContentCoding != sendContentCoding)) {
        string data;
        decompressor->dump();
        decompressor->read(data);
        if (useChain) {
            chain->dataFeed(data);
        } else if (useGifFilter) {
            GIFfilter->dataFeed(data);
        } else {
            dataFeed(data);
        }
    }
    if (useChain) {
        if (url.getDebug())
            dataFeed("\n</div>\n</body>\n</html>\n");
        chain->dataDump();
    } else if (useGifFilter) {
        GIFfilter->dataDump();
    } else {
        dataDump();
    }
    // Write last chunk (size 0)
    sendInBuf += "0" CRLF CRLF;
}
// vi:ts=4:sw=4:et
