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


#include "log.h"
#include "const.h"
#include "trayicon.h"
#include "logframe.h"
#include "mainframe.h"

/* Functions to log event
 */
void CLog::logProxyEvent(pmEVT_PROXY_TYPE type, const wxIPV4address& addr) {

    if (proxyListeners.empty()) return;
    CProxyEvent event(type, addr);
    for (set<wxEvtHandler*>::iterator it = proxyListeners.begin();
            it != proxyListeners.end(); it++)
        (*it)->AddPendingEvent(event);
}

void CLog::logHttpEvent(pmEVT_HTTP_TYPE type, const wxIPV4address& addr,
                          int req, const string& text) {

    if (httpListeners.empty()) return;
    CHttpEvent event(type, addr, req, text);
    for (set<wxEvtHandler*>::iterator it = httpListeners.begin();
            it != httpListeners.end(); it++)
        (*it)->AddPendingEvent(event);
}

void CLog::logFilterEvent(pmEVT_FILTER_TYPE type, int req,
                            const string& title, const string& text) {

    if (filterListeners.empty()) return;
    CFilterEvent event(type, req, title, text);
    for (set<wxEvtHandler*>::iterator it = filterListeners.begin();
            it != filterListeners.end(); it++)
        (*it)->AddPendingEvent(event);
}


/* Singleton methods
 */
CLog* CLog::instance = NULL;

CLog& CLog::ref() {
    if (!instance) instance = new CLog();
    return *instance;
}

void CLog::destroy() {
    if (instance) delete instance;
    instance = NULL;
}

CLog::CLog() {
    numRequests = numActiveRequests = numOpenSockets = 0;
    mainFrame = NULL;
    logFrame = NULL;
    trayIcon = NULL;
}

CLog::~CLog() {
    if (trayIcon) delete trayIcon;
    if (logFrame) logFrame->Destroy();
    if (mainFrame) mainFrame->Destroy();
}
