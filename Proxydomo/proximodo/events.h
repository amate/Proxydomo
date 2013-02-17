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


#ifndef __events__
#define __events__

#include <wx/event.h>
#include <wx/socket.h>
#include <string>

using namespace std;

/* Event classes.
 * The proxy, the request managers and the filters can notify CLog, the
 * log system, when they do something worth reporting. CLog will then
 * post this event to all event handlers recorded in its sets.
 * The advantage is that distributed events are spooled, to avoid 
 * slowing down the sender.
 * pmEVT_TYPE_STATUS is an event for writing text in the frame's statusbar.
 * Currently used for displaying wxWindow's GetHelpText.
 */

/* Event types for wx event system
 */
BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(pmEVT_TYPE_PROXY,  wxEVT_USER_FIRST+1)
    DECLARE_EVENT_TYPE(pmEVT_TYPE_HTTP,   wxEVT_USER_FIRST+2)
    DECLARE_EVENT_TYPE(pmEVT_TYPE_FILTER, wxEVT_USER_FIRST+3)
    DECLARE_EVENT_TYPE(pmEVT_TYPE_STATUS, wxEVT_USER_FIRST+4)
END_DECLARE_EVENT_TYPES()


/* Event sub-types for proxy events
 */
enum pmEVT_PROXY_TYPE {
    pmEVT_PROXY_TYPE_START,         // Proxy server port opened
    pmEVT_PROXY_TYPE_STOP,          // Proxy server port closed
    pmEVT_PROXY_TYPE_ACCEPT,        // A connexion has been accepted
    pmEVT_PROXY_TYPE_REFUSE,        // A connexion has been refused
    pmEVT_PROXY_TYPE_NEWREQ,        // A new request is being processed
    pmEVT_PROXY_TYPE_ENDREQ,        // A request has ended (response obtained)
    pmEVT_PROXY_TYPE_NEWSOCK,       // A new browser socket opened
    pmEVT_PROXY_TYPE_ENDSOCK        // A browser socket closed
};


/* Event sub-types for HTTP events
 */
enum pmEVT_HTTP_TYPE {
    pmEVT_HTTP_TYPE_RECVOUT,
    pmEVT_HTTP_TYPE_SENDOUT,
    pmEVT_HTTP_TYPE_POSTOUT,
    pmEVT_HTTP_TYPE_RECVIN,
    pmEVT_HTTP_TYPE_SENDIN
};


/* Event sub-types for filtering events
 */
enum pmEVT_FILTER_TYPE {
    pmEVT_FILTER_TYPE_HEADERMATCH,
    pmEVT_FILTER_TYPE_HEADERREPLACE,
    pmEVT_FILTER_TYPE_TEXTMATCH,
    pmEVT_FILTER_TYPE_TEXTREPLACE,
    pmEVT_FILTER_TYPE_LOGCOMMAND
};


/* Proxy event
 */
class CProxyEvent : public wxEvent {

public:
    CProxyEvent();
    CProxyEvent(const pmEVT_PROXY_TYPE type,
                const wxIPV4address& addr);
    virtual wxEvent* Clone() const;
    pmEVT_PROXY_TYPE type;
    wxIPV4address addr;
};


/* HTTP event
 */
class CHttpEvent : public wxEvent {

public:
    CHttpEvent();
    CHttpEvent(const pmEVT_HTTP_TYPE type,
               const wxIPV4address& addr,
               const int req,
               const string& text);
    virtual wxEvent* Clone() const;
    pmEVT_HTTP_TYPE type;
    wxIPV4address addr;
    int req;
    string text;
};


/* Filtering event
 */
class CFilterEvent : public wxEvent {

public:
    CFilterEvent();
    CFilterEvent(const pmEVT_FILTER_TYPE type,
                 const int req,
                 const string& title,
                 const string& text);
    virtual wxEvent* Clone() const;
    pmEVT_FILTER_TYPE type;
    int req;
    string title;
    string text;
};


/* Status event
 */
class CStatusEvent : public wxEvent {

public:
    CStatusEvent();
    CStatusEvent(const string& text, const int field = 0);
    virtual wxEvent* Clone() const;
    string text;
    int field;
};


/* Event processing functions definition
 */
typedef void (wxEvtHandler::*pmProxyHandlerFunction )(CProxyEvent& );
typedef void (wxEvtHandler::*pmHttpHandlerFunction  )(CHttpEvent&  );
typedef void (wxEvtHandler::*pmFilterHandlerFunction)(CFilterEvent&);
typedef void (wxEvtHandler::*pmStatusHandlerFunction)(CStatusEvent&);


/* Event table macros
 */
#define EVT_PROXY(func) \
    DECLARE_EVENT_TABLE_ENTRY( pmEVT_TYPE_PROXY, wxID_ANY, wxID_ANY, \
        (wxObjectEventFunction) (wxEventFunction)  (pmProxyHandlerFunction) & func , \
        (wxObject *) NULL ),

#define EVT_HTTP(func) \
    DECLARE_EVENT_TABLE_ENTRY( pmEVT_TYPE_HTTP, wxID_ANY, wxID_ANY, \
        (wxObjectEventFunction) (wxEventFunction)  (pmHttpHandlerFunction) & func , \
        (wxObject *) NULL ),

#define EVT_FILTER(func) \
    DECLARE_EVENT_TABLE_ENTRY( pmEVT_TYPE_FILTER, wxID_ANY, wxID_ANY, \
        (wxObjectEventFunction) (wxEventFunction)  (pmFilterHandlerFunction) & func , \
        (wxObject *) NULL ),

#define EVT_STATUS(func) \
    DECLARE_EVENT_TABLE_ENTRY( pmEVT_TYPE_STATUS, wxID_ANY, wxID_ANY, \
        (wxObjectEventFunction) (wxEventFunction)  (pmStatusHandlerFunction) & func , \
        (wxObject *) NULL ),

#endif
