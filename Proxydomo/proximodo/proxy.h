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


#ifndef __proxy__
#define __proxy__

#include <wx/event.h>
#include <vector>
class CRequestManager;
class CManagerThread;
class wxSocketEvent;
class wxSocketServer;

using namespace std;

class CProxy : public wxEvtHandler {

public:
    // Singleton methods
    static CProxy& ref();
    static void destroy();
    
    // Test the port configured in settings, 
    // using testPort(const string& port).
    bool testPort();
    
    // Tests a port for server usage.
    // If the port is the one we are using, returns true.
    // Otherwise check that:
    // 1. The port is not in use (i.e. you cannot connect to it), and
    // 2. You can open a server on the port.
    // If both tests pass, returns true, otherwise false.
    bool testPort(const string& port);
    
    // Tests a remote proxy.
    bool testRemoteProxy(string hostport);

    // Attempts opening server socket.
    // If the proxy is already started, it remains so.
    // Then starts accepting connections.
    // Returns true if port is open.
    bool openProxyPort();
    
    // Closes server socket.
    // Currents connections from browser stay open
    // till they close themselves.
    void closeProxyPort();
    
    // Set whether the proxy should accept connections
    void acceptConnections(bool accept);
    bool acceptConnections();
    
    // Aborts all current connections.
    // If you need to stop accepting new connections,
    // use closeProxyPort first.
    void abortConnections();
    
    // Marks all current managers as obsolete (!valid).
    // New connections will be processed by new managers.
    void refreshManagers();
    
protected:
    CProxy();
    ~CProxy();

private:
    // Singleton instance
    static CProxy* instance;

    // Called by server socket's event manager
    void OnServerEvent(wxSocketEvent& event);

    // Proxy server socket
    wxSocketServer *server;
    
    // Accepting status
    bool accepting;
    
    // Current managers
    vector<CRequestManager*> managers;
    
    // Manager threads
    vector<CManagerThread*> threads;
    
    // For event management
    DECLARE_EVENT_TABLE()
};

#endif
