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


#ifndef __managerthread__
#define __managerthread__

#include <wx/thread.h>
class CRequestManager;
class wxSocketBase;

/* This class embeds a thread that runs a request manager on a browser socket.
 * So that socket "events" are really processed _in_ the thread, the request
 * manager runs in a loop and polls sockets for new data to read, or buffers
 * for new data to send. We don't use wx socket events, because these are
 * collected by a single event manager (wx threads don't have their own
 * event manager), so, though they can be sent to our request manager object,
 * they will execute in the same thread as the event manager, i.e in the main
 * one. That is not what we want: for example, a filter waiting for a user
 * click in a $CONFIRM dialog box blocks its whole thread, but as we do want
 * other simultaneously loading pages to continue going through (and especially
 * the GUI to continue working independently), each request processing must run
 * in a dedicated thread.
 */
class CManagerThread : public wxThread {

public:
    CManagerThread(wxSocketBase* sock, CRequestManager* manager);
    ~CManagerThread() { }
    virtual ExitCode Entry();
    
private:
    wxSocketBase* sock;
    CRequestManager* manager;
};

#endif
