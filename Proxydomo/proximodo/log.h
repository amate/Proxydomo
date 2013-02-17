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


#ifndef __log__
#define __log__

#include "events.h"
#include <wx/thread.h>
#include <set>
#include <string>
class CLogFrame;
class CMainFrame;
class CTrayIcon;

using namespace std;

/* This class contains statistics, and collects technical events.
 * It is implemented as a singleton, because there need be only one.
 * Each received event is immediately broadcasted to registered event handlers.
 *
 * To send events to the log system, any function can do:
 *
 *       CLog::ref().logFilterEvent(type, reqNum, title, text);
 *       CLog::ref().logProxyEvent(type, addr);
 *       CLog::ref().logHttpEvent(type, addr, reqNum, text);
 *
 * To listen to them, a class that inherits wxEvtHandler does:
 *
 *       CLog::ref().filterListeners.insert(this);
 *       CLog::ref().proxyListeners.insert(this);
 *       CLog::ref().httpListeners.insert(this);
 *
 * To stop listening, the class unregisters itself:
 *
 *       CLog::ref().filterListeners.erase(this);
 *       CLog::ref().proxyListeners.erase(this);
 *       CLog::ref().httpListeners.erase(this);
 *
 * The class must have in the BEGIN_EVENT_TABLE section the corresponding lines:
 *
 *       EVT_FILTER (CLogViewer::OnProxyEvent)
 *       EVT_PROXY  (CLogViewer::OnProxyEvent)
 *       EVT_HTTP   (CLogViewer::OnProxyEvent)
 *
 * The handling functions should look like this:
 *
 *       void myListeningClass::OnProxyEvent(CProxyEvent& evt) {
 *           // ...display or use evt properties...
 *       }
 */
class CLog {

public:
    // Singleton methods
    static CLog& ref();
    static void destroy();

    // Total number of requests received since Proximodo started
    int numRequests;
    
    // Total number of browser connections since Proximodo started
    int numConnections;

    // Number of requests being processed
    int numActiveRequests;
    
    // Number of open browser sockets
    int numOpenSockets;

    // Functions to log event
    void logProxyEvent(pmEVT_PROXY_TYPE type, const wxIPV4address& addr);
    void logHttpEvent(pmEVT_HTTP_TYPE type, const wxIPV4address& addr,
                      int req, const string& text);
    void logFilterEvent(pmEVT_FILTER_TYPE type, int req,
                        const string& title, const string& text);

    // Handlers wishing to receive log events
    set<wxEvtHandler*> proxyListeners;
    set<wxEvtHandler*> httpListeners;
    set<wxEvtHandler*> filterListeners;
    
    // Mutex for $LOCK commands
    wxMutex filterLock;
    
    // String used in the test window
    string testString;
    
    // Log window
    CLogFrame* logFrame;
    
    // Main window
    CMainFrame* mainFrame;
    
    // Taskbar icon
    CTrayIcon* trayIcon;
    
protected:
    CLog();
    ~CLog();

private:
    // The singleton instance
    static CLog* instance;
};

#endif
