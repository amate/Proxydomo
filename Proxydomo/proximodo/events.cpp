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


#include "events.h"

/* Proxy event ------------------------------------
 */

DEFINE_EVENT_TYPE(pmEVT_TYPE_PROXY)

CProxyEvent::CProxyEvent() : wxEvent(wxID_ANY, pmEVT_TYPE_PROXY) {
}

CProxyEvent::CProxyEvent(const pmEVT_PROXY_TYPE type,
                         const wxIPV4address& addr) :
        wxEvent(wxID_ANY, pmEVT_TYPE_PROXY),
        type(type), addr(addr) {
}

wxEvent* CProxyEvent::Clone() const {
    return new CProxyEvent(*this);
}


/* HTTP event ------------------------------------
 */

DEFINE_EVENT_TYPE(pmEVT_TYPE_HTTP)

CHttpEvent::CHttpEvent() : wxEvent(wxID_ANY, pmEVT_TYPE_HTTP) {
}

CHttpEvent::CHttpEvent(const pmEVT_HTTP_TYPE type,
                       const wxIPV4address& addr,
                       const int req,
                       const string& text) :
        wxEvent(wxID_ANY, pmEVT_TYPE_HTTP),
        type(type), addr(addr), req(req), text(text) {
}

wxEvent* CHttpEvent::Clone() const {
    return new CHttpEvent(*this);
}


/* Filtering event ------------------------------------
 */

DEFINE_EVENT_TYPE(pmEVT_TYPE_FILTER)

CFilterEvent::CFilterEvent() : wxEvent(wxID_ANY, pmEVT_TYPE_FILTER) {
}

CFilterEvent::CFilterEvent(const pmEVT_FILTER_TYPE type,
                           const int req,
                           const string& title,
                           const string& text) :
        wxEvent(wxID_ANY, pmEVT_TYPE_FILTER),
        type(type), req(req), title(title), text(text) {
}

wxEvent* CFilterEvent::Clone() const {
    return new CFilterEvent(*this);
}


/* Statusbar update event ------------------------------------
 */

DEFINE_EVENT_TYPE(pmEVT_TYPE_STATUS)

CStatusEvent::CStatusEvent() : wxEvent(wxID_ANY, pmEVT_TYPE_STATUS) {
}

CStatusEvent::CStatusEvent(const string& text, const int field) :
        wxEvent(wxID_ANY, pmEVT_TYPE_STATUS),
        text(text), field(field) {
    m_propagationLevel = wxEVENT_PROPAGATE_MAX;
}

wxEvent* CStatusEvent::Clone() const {
    return new CStatusEvent(*this);
}

