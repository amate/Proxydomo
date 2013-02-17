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


#ifndef __requestmanager__
#define __requestmanager__

#include "receptor.h"
#include "filterowner.h"
#include <wx/socket.h>
#include <string>
#include <vector>
class CMatcher;
class CTextFilter;
class CHeaderFilter;
class CGifFilter;
class CZlibBuffer;
class CFilter;

using namespace std;


/* Structures for request/response lines
 */
struct SRequestLine { string method, url, ver; };
struct SResponseLine { string ver, code, msg; };


/* The request manager is the proxy by itself: it reads the browser's request
 * on the acquired socket, processes OUT headers, connects to the requested
 * website, filters IN headers and html data, and sends it back to the browser.
 *
 * If the request is not manageable (not GET nor POST, not http://, MIME type
 * unknown) the data travels through the manager unchanged (cf bool bypassing).
 *
 * The bool valid is only here for CProxy use, which will set it to false
 * when the configuration has changed (filters activated/deactivated, additions
 * to files...), so that it knows it must destroy them when they become
 * available again. Indeed, request managers are recyclable, as long as
 * the tree searches and filter selection hardwired in it don't change.
 *
 * Text filters are chained at construction. The chain starts and finishes
 * into the request manager.
 */
class CRequestManager : public CDataReceptor,   // for terminating filter chain
                        public CFilterOwner {   // for document-specific info

public:
    CRequestManager();
    ~CRequestManager();

    bool valid;
    bool available;

    void manage(wxSocketBase* browser);
    void abort();
    
    void OnBrowserEvent(wxSocketEvent& event);
    void OnWebsiteEvent(wxSocketEvent& event);

    void dataReset();
    void dataFeed(const string& data);
    void dataDump();

private:
    // Filter instances
    vector<CHeaderFilter*> INfilters;
    vector<CHeaderFilter*> OUTfilters;
    CGifFilter* GIFfilter;
    CDataReceptor* chain;

    // Sockets
    wxSocketBase*   browser;
    wxSocketClient* website;
    wxIPV4address   addr;
    string previousHost;
    void connectWebsite();
    void destroy();

    // Processing steps (incoming or outgoing)
    enum STEP { STEP_START,     // marks the beginning of a request/response
                STEP_FIRSTLINE, // read GET/POST line, or status line
                STEP_HEADERS,   // read headers
                STEP_DECODE,    // check and filter headers
                STEP_CHUNK,     // read chunk size / trailers
                STEP_RAW,       // read raw message body
                STEP_TUNNELING, // read data until disconnection
                STEP_FINISH };  // marks the end of a request/response

    // Variables and functions for outgoing processing
    STEP   outStep;
    int    outSize;
    bool   outChunked;
    SRequestLine requestLine;

    bool   receiveOut();
    string recvOutBuf;
    void   processOut();
    string sendOutBuf;
    bool   sendOut();

    // for recv events only
    string logRequest;
    string logResponse;
    string logPostData;
    
    // for correct incoming body processing
    bool recvConnectionClose; // should the connection be closed when finished
    bool sendConnectionClose;
    int  recvContentCoding;   // 0: plain/compress, 1: gzip, 2:deflate (zlib)
    int  sendContentCoding;
    CZlibBuffer* decompressor;
    CZlibBuffer* compressor;

    // Variables and functions for incoming processing
    STEP   inStep;
    int    inSize;
    bool   inChunked;
    SResponseLine responseLine;

    bool   receiveIn();
    string recvInBuf;
    void   processIn();
    string sendInBuf;
    bool   sendIn();

    bool   useChain;
    bool   useGifFilter;
    void   endFeeding();
    bool   dumped;          // incoming data flow is over, don't send anything else in
    int    redirectedIn;    // number of redirections by incoming headers
    
    // Bypass-URL matcher
    CFilter*  urlFilter;  // needed between *urlMatcher and *this
    CMatcher* urlMatcher;

    // verify Content-Type header
    bool   verifyContentType(string& content);
    
    // fake a website response
    void fakeResponse(string code, string filename, bool replace = false,
                      string str1 = "", string str2 = "", string str3 = "");
};

#endif
// vi:ts=4:sw=4:et
