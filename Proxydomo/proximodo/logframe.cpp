//------------------------------------------------------------------
//
//this file is part of Proximodo
//Copyright (C) 2004-2005 Antony BOUCHER ( kuruden@users.sourceforge.net )
//                        Paul Rupe      ( prupe@users.sourceforge.net )
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


#include "logframe.h"
#include "images/icon32.xpm"
#include "util.h"
#include "log.h"
#include "const.h"
#include "settings.h"
#include "controls.h"
#include <wx/settings.h>
#include <wx/colour.h>
#include <wx/icon.h>
#include <string>
#include <sstream>

#define  LOG_COLOR_BACKGROUND  255, 255, 255
#define  LOG_COLOR_DEFAULT     140, 140, 140
#define  LOG_COLOR_FILTER      140, 140, 140
#define  LOG_COLOR_REQUEST     240, 100,   0
#define  LOG_COLOR_RESPONSE      0, 150,   0
#define  LOG_COLOR_PROXY         0,   0,   0
#define  LOG_COLOR_R           255,   0,   0
#define  LOG_COLOR_W             0,   0,   0
#define  LOG_COLOR_B             0,   0, 255
#define  LOG_COLOR_G             0, 150,   0
#define  LOG_COLOR_C             0, 150, 150
#define  LOG_COLOR_w           140, 140, 140
#define  LOG_COLOR_Y           230, 200,   0
#define  LOG_COLOR_V           150,   0, 150

using namespace std;

/* Events
 */
BEGIN_EVENT_TABLE(CLogFrame, wxFrame)
    EVT_BUTTON (ID_CLEARBUTTON, CLogFrame::OnCommand)
    EVT_BUTTON (ID_STARTBUTTON, CLogFrame::OnCommand)
    EVT_PROXY  (CLogFrame::OnProxyEvent)
    EVT_HTTP   (CLogFrame::OnHttpEvent)
    EVT_FILTER (CLogFrame::OnFilterEvent)
END_EVENT_TABLE()


/* Saved window position
 */
int CLogFrame::savedX = BIG_NUMBER,
    CLogFrame::savedY = 0,
    CLogFrame::savedW = 0,
    CLogFrame::savedH = 0,
    CLogFrame::savedOpt = 0;


/* Constructor
 */
CLogFrame::CLogFrame() 
       : wxFrame((wxFrame *)NULL, wxID_ANY,
                 S2W(CSettings::ref().getMessage("LOG_WINDOW_TITLE")),
                 wxDefaultPosition, wxDefaultSize,
                 wxDEFAULT_FRAME_STYLE |
                 wxTAB_TRAVERSAL |
                 wxCLIP_CHILDREN ),
         settings(CSettings::ref()) {

    SetIcon(wxIcon(icon32_xpm));
    CSettings& settings = CSettings::ref();
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    wxBoxSizer* vertSizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(vertSizer);

    logText =  new pmTextCtrl(this, wxID_ANY, wxT("") ,
        wxDefaultPosition, wxSize(500,250),
        wxTE_READONLY | wxTE_RICH |  wxTE_MULTILINE | wxTE_DONTWRAP );
    logText->SetBackgroundColour(wxColour(LOG_COLOR_BACKGROUND));
    logText->SetDefaultStyle(wxTextAttr(wxColour(LOG_COLOR_DEFAULT),
                                        wxColour(LOG_COLOR_BACKGROUND),
                                        wxFont(10, wxMODERN, wxNORMAL, wxBOLD)));
    vertSizer->Add(logText,1,wxGROW | wxALL,5);

    wxBoxSizer* lowerSizer = new wxBoxSizer(wxHORIZONTAL);
    vertSizer->Add(lowerSizer,0,wxALIGN_LEFT | wxALL,0);

    startButton =  new pmButton(this, ID_STARTBUTTON,
        S2W(settings.getMessage("LOG_BUTTON_STOP")) );
    lowerSizer->Add(startButton,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    pmButton* clearButton =  new pmButton(this, ID_CLEARBUTTON,
        S2W(settings.getMessage("LOG_BUTTON_CLEAR")) );
    lowerSizer->Add(clearButton,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    wxBoxSizer* ckbxSizer1 = new wxBoxSizer(wxVERTICAL);
    lowerSizer->Add(ckbxSizer1,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    httpCheckbox =  new pmCheckBox(this, wxID_ANY,
        S2W(settings.getMessage("LOG_CKB_HTTP")) );
    ckbxSizer1->Add(httpCheckbox,0,wxALIGN_LEFT | wxALL,2);

    postCheckbox =  new pmCheckBox(this, wxID_ANY,
        S2W(settings.getMessage("LOG_CKB_POST")) );
    ckbxSizer1->Add(postCheckbox,0,wxALIGN_LEFT | wxALL,2);

    browserCheckbox =  new pmCheckBox(this, wxID_ANY,
        S2W(settings.getMessage("LOG_CKB_BROWSER")) );
    ckbxSizer1->Add(browserCheckbox,0,wxALIGN_LEFT | wxALL,2);

    headersCheckbox =  new pmCheckBox(this, wxID_ANY,
        S2W(settings.getMessage("LOG_CKB_HEADERS")) );
    ckbxSizer1->Add(headersCheckbox,0,wxALIGN_LEFT | wxALL,2);

    wxBoxSizer* ckbxSizer2 = new wxBoxSizer(wxVERTICAL);
    lowerSizer->Add(ckbxSizer2,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    proxyCheckbox =  new pmCheckBox(this, wxID_ANY,
        S2W(settings.getMessage("LOG_CKB_PROXY")) );
    ckbxSizer2->Add(proxyCheckbox,0,wxALIGN_LEFT | wxALL,2);

    filterCheckbox =  new pmCheckBox(this, wxID_ANY,
        S2W(settings.getMessage("LOG_CKB_FILTER")) );
    ckbxSizer2->Add(filterCheckbox,0,wxALIGN_LEFT | wxALL,2);

    logCheckbox =  new pmCheckBox(this, wxID_ANY,
        S2W(settings.getMessage("LOG_CKB_LOG")) );
    ckbxSizer2->Add(logCheckbox,0,wxALIGN_LEFT | wxALL,2);

    active = true;
    httpCheckbox->SetValue(true);
    browserCheckbox->SetValue(true);
    headersCheckbox->SetValue(true);
    logCheckbox->SetValue(true);
    filterCheckbox->SetValue(true);
    
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);

    // Reload position, size and options
    if (savedX == BIG_NUMBER) {
        Centre(wxBOTH | wxCENTRE_ON_SCREEN);
    } else {
        Move(savedX, savedY);
        SetSize(savedW, savedH);
        headersCheckbox->SetValue((savedOpt & 64) != 0);
        logCheckbox->SetValue((savedOpt & 32) != 0);
        httpCheckbox->SetValue((savedOpt & 16) != 0);
        postCheckbox->SetValue((savedOpt & 8) != 0);
        proxyCheckbox->SetValue((savedOpt & 4) != 0);
        filterCheckbox->SetValue((savedOpt & 2) != 0);
        browserCheckbox->SetValue((savedOpt & 1) != 0);
    }

    CLog::ref().httpListeners.insert(this);
    CLog::ref().proxyListeners.insert(this);
    CLog::ref().filterListeners.insert(this);
}


/* Destructor
 */
CLogFrame::~CLogFrame() {

    // Record position, size and options
    GetPosition(&savedX, &savedY);
    GetSize(&savedW, &savedH);
    savedOpt =
        (headersCheckbox->GetValue() ? 64 : 0) +
        (logCheckbox->GetValue() ? 32 : 0) +
        (httpCheckbox->GetValue() ? 16 : 0) +
        (postCheckbox->GetValue() ? 8 : 0) +
        (proxyCheckbox->GetValue() ? 4 : 0) +
        (filterCheckbox->GetValue() ? 2 : 0) +
        (browserCheckbox->GetValue() ? 1 : 0);

    CLog::ref().httpListeners.erase(this);
    CLog::ref().proxyListeners.erase(this);
    CLog::ref().filterListeners.erase(this);
    CLog::ref().logFrame = NULL;
}


/* Event handling functions
 */
void CLogFrame::OnCommand(wxCommandEvent& event) {

    switch (event.GetId()) {
    case ID_STARTBUTTON:
        {
            if (active)
                startButton->SetLabel(S2W(settings.getMessage("LOG_BUTTON_START")));
            else
                startButton->SetLabel(S2W(settings.getMessage("LOG_BUTTON_STOP")));
            active = !active;
            break;
        }
    case ID_CLEARBUTTON:
        logText->Clear(); break;
        
    default:
        event.Skip();
    }
}


/* Log event listeners
 */
void CLogFrame::OnProxyEvent(CProxyEvent& evt) {

    // Exclude unwanted events
    if (!active || !proxyCheckbox->GetValue()) return;
    
    logText->SetDefaultStyle(wxTextAttr(wxColour(LOG_COLOR_PROXY)));

    stringstream port;
    port << evt.addr.Service();
    
    string mess;
    switch (evt.type) {
    case pmEVT_PROXY_TYPE_START:
        mess = "EVT_PROXY_START";   break;
        
    case pmEVT_PROXY_TYPE_STOP:
        mess = "EVT_PROXY_STOP";    break;
        
    case pmEVT_PROXY_TYPE_ACCEPT:
        mess = "EVT_PROXY_ACCEPT";  break;
        
    case pmEVT_PROXY_TYPE_REFUSE:
        mess = "EVT_PROXY_REFUSE";  break;
        
    case pmEVT_PROXY_TYPE_NEWREQ:
        mess = "EVT_PROXY_NEWREQ";  break;
        
    case pmEVT_PROXY_TYPE_ENDREQ:
        mess = "EVT_PROXY_ENDREQ";  break;
        
    case pmEVT_PROXY_TYPE_NEWSOCK:
        mess = "EVT_PROXY_NEWSOCK"; break;
        
    case pmEVT_PROXY_TYPE_ENDSOCK:
        mess = "EVT_PROXY_ENDSOCK"; break;
        
    default:
        return;
    }

    logText->AppendText(S2W(settings.getMessage(mess, port.str())));
    logText->AppendText(wxT("\n"));
    logText->ShowPosition(logText->GetInsertionPoint());
}

void CLogFrame::OnHttpEvent(CHttpEvent& evt) {

    // Exclude unwanted events
    if (   !active
        || !httpCheckbox->GetValue()
        || (!postCheckbox->GetValue()
            && evt.type == pmEVT_HTTP_TYPE_POSTOUT)
        || (browserCheckbox->GetValue()
            && (   evt.type == pmEVT_HTTP_TYPE_SENDOUT
                || evt.type == pmEVT_HTTP_TYPE_RECVIN))
        || (!browserCheckbox->GetValue()
            && (   evt.type == pmEVT_HTTP_TYPE_RECVOUT
                || evt.type == pmEVT_HTTP_TYPE_SENDIN))   )
        return;

    // Colors depends on Outgoing or Incoming
    if (evt.type == pmEVT_HTTP_TYPE_RECVIN || evt.type == pmEVT_HTTP_TYPE_SENDIN)
        logText->SetDefaultStyle(wxTextAttr(wxColour(LOG_COLOR_RESPONSE)));
    else
        logText->SetDefaultStyle(wxTextAttr(wxColour(LOG_COLOR_REQUEST)));

    stringstream port;
    port << evt.addr.Service();
    stringstream req;
    req << evt.req;

    string mess;
    switch (evt.type) {
    case pmEVT_HTTP_TYPE_RECVOUT:
        mess = "EVT_HTTP_RECVOUT";  break;

    case pmEVT_HTTP_TYPE_SENDOUT:
        mess = "EVT_HTTP_SENDOUT";  break;

    case pmEVT_HTTP_TYPE_RECVIN:
        mess = "EVT_HTTP_RECVIN";   break;

    case pmEVT_HTTP_TYPE_SENDIN:
        mess = "EVT_HTTP_SENDIN";   break;

    case pmEVT_HTTP_TYPE_POSTOUT:
        mess = "EVT_HTTP_POSTOUT";  break;

    default:
        return;
    }

    if (!headersCheckbox->GetValue()) {
        if (postCheckbox->GetValue() || proxyCheckbox->GetValue() ||
                filterCheckbox->GetValue() || logCheckbox->GetValue()) {
            logText->AppendText(S2W(settings.getMessage("EVT_HTTP_GENERIC", req.str(), port.str(), evt.text.substr(0,evt.text.find('\n')))));
        } else {
            logText->AppendText(S2W("#" + req.str() + ": " + evt.text.substr(0,evt.text.find('\n'))));
        }
    } else {
        logText->AppendText(S2W(settings.getMessage(mess, req.str(), port.str(), evt.text)));
        logText->AppendText(wxT("\n"));
    }
    logText->ShowPosition(logText->GetInsertionPoint());
}

void CLogFrame::OnFilterEvent(CFilterEvent& evt) {

    if (evt.type == pmEVT_FILTER_TYPE_LOGCOMMAND) {
    
        // If a Log event starts with ! the window must appear and show it
        if (evt.text.length() > 0 && evt.text[0] == '!') {

            evt.text.erase(0, 1);
            logCheckbox->SetValue(true);
            startButton->SetLabel(S2W(settings.getMessage("LOG_BUTTON_STOP")));
            active = true;
            CUtil::show(this);
        }

        // Exclude unwanted events
        if (!active || !logCheckbox->GetValue() || evt.text.length() < 2) return;

        stringstream req;
        req << evt.req;

        // The first line is always the same color
        logText->SetDefaultStyle(wxTextAttr(wxColour(LOG_COLOR_FILTER)));
        
        logText->AppendText(S2W(settings.getMessage("EVT_FILTER_LOG", req.str(), evt.title)));
        logText->AppendText(wxT("\n"));
        
        // The second line's color depends on the first character
        switch (evt.text[0]) {
        case 'R': logText->SetDefaultStyle(wxTextAttr(wxColour(LOG_COLOR_R))); break;
        case 'W': logText->SetDefaultStyle(wxTextAttr(wxColour(LOG_COLOR_W))); break;
        case 'B': logText->SetDefaultStyle(wxTextAttr(wxColour(LOG_COLOR_B))); break;
        case 'G': logText->SetDefaultStyle(wxTextAttr(wxColour(LOG_COLOR_G))); break;
        case 'C': logText->SetDefaultStyle(wxTextAttr(wxColour(LOG_COLOR_C))); break;
        case 'w': logText->SetDefaultStyle(wxTextAttr(wxColour(LOG_COLOR_w))); break;
        case 'Y': logText->SetDefaultStyle(wxTextAttr(wxColour(LOG_COLOR_Y))); break;
        case 'V': logText->SetDefaultStyle(wxTextAttr(wxColour(LOG_COLOR_V))); break;
        }
        logText->AppendText(S2W(evt.text.substr(1)));
        logText->AppendText(wxT("\n"));
        logText->ShowPosition(logText->GetInsertionPoint());
    
    } else {

        // Exclude unwanted events
        if (!active || !filterCheckbox->GetValue()) return;

        logText->SetDefaultStyle(wxTextAttr(wxColour(LOG_COLOR_FILTER)));

        stringstream req;
        req << evt.req;

        string mess;
        switch (evt.type) {
        case pmEVT_FILTER_TYPE_HEADERMATCH:
            mess = "EVT_FILTER_HMATCH";    break;

        case pmEVT_FILTER_TYPE_HEADERREPLACE:
            mess = "EVT_FILTER_HREPLACE";  break;

        case pmEVT_FILTER_TYPE_TEXTMATCH:
            mess = "EVT_FILTER_TMATCH";    break;

        case pmEVT_FILTER_TYPE_TEXTREPLACE:
            mess = "EVT_FILTER_TREPLACE";  break;

        default:
            return;
        }

        logText->AppendText(S2W(settings.getMessage(mess, req.str(), evt.title, evt.text)));
        logText->AppendText(wxT("\n"));
        logText->ShowPosition(logText->GetInsertionPoint());
    }
}
// vi:ts=4:sw=4:et
