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


#include "welcomescreen.h"
#include "mainframe.h"
#include "proxy.h"
#include "log.h"
#include "util.h"
#include "settings.h"
#include "controls.h"
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/event.h>
#include <sstream>

using namespace std;

/* Events
 */
BEGIN_EVENT_TABLE(CWelcomeScreen, CWindowContent)
    EVT_CHECKBOX   (ID_FILTERINCHECKBOX,     CWelcomeScreen::OnCommand)
    EVT_CHECKBOX   (ID_FILTEROUTCHECKBOX,    CWelcomeScreen::OnCommand)
    EVT_CHECKBOX   (ID_FILTERTEXTCHECKBOX,   CWelcomeScreen::OnCommand)
    EVT_CHECKBOX   (ID_FILTERGIFCHECKBOX,    CWelcomeScreen::OnCommand)
    EVT_COMBOBOX   (ID_CONFIGDROPDOWN,       CWelcomeScreen::OnCommand)
    EVT_BUTTON     (ID_ABORTBUTTON,          CWelcomeScreen::OnCommand)
    EVT_PROXY      (CWelcomeScreen::OnProxyEvent)
END_EVENT_TABLE()


/* Constructor
 */
CWelcomeScreen::CWelcomeScreen(wxFrame* frame) : CWindowContent(frame, wxVERTICAL) {

    // Sizers

    wxBoxSizer* bottomBox = new wxBoxSizer(wxHORIZONTAL);
    Add(bottomBox, 1, wxGROW | wxALL, 0);

    wxBoxSizer* leftBox = new wxBoxSizer(wxVERTICAL);
    bottomBox->Add(leftBox, 1, wxGROW | wxALL, 0);

    wxBoxSizer* rightBox = new wxBoxSizer(wxVERTICAL);
    bottomBox->Add(rightBox, 1, wxGROW | wxALL, 0);

    // Config drop-down

    wxBoxSizer* configBox = new wxBoxSizer(wxHORIZONTAL);
    Add(configBox, 0, wxGROW | wxALL, 0);

    pmStaticText* configLabel =  new pmStaticText(frame, wxID_ANY,
        S2W(settings.getMessage("LB_SETTINGS_ACTIVE")));
    configBox->Add(configLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

    wxArrayString choices;
    configDropdown =  new pmComboBox(frame, ID_CONFIGDROPDOWN,
        wxT(""), wxDefaultPosition, wxDefaultSize, choices,
        wxCB_DROPDOWN | wxCB_READONLY | wxCB_SORT);
    configDropdown->SetHelpText(S2W(settings.getMessage("SETTINGS_CONFIG_TIP")));
    configBox->Add(configDropdown, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);

    // Filtering settings

    wxStaticBox* filterStaticBox_StaticBoxObj = new wxStaticBox(frame, wxID_ANY,
        S2W(settings.getMessage("LB_SETTINGS_FILTERING")));
    pmStaticBoxSizer* filterStaticBox = new pmStaticBoxSizer(
        filterStaticBox_StaticBoxObj, wxHORIZONTAL);
    leftBox->Add(filterStaticBox, 1, wxGROW | wxALL, 5);

    wxBoxSizer* filterBox = new wxBoxSizer(wxVERTICAL);
    filterStaticBox->Add(filterBox, 1, wxALIGN_TOP | wxALL, 0);

    filterOutCheckbox =  new pmCheckBox(frame, ID_FILTEROUTCHECKBOX,
        S2W(settings.getMessage("LB_SETTINGS_OUTGOING")));
    filterOutCheckbox->SetHelpText(S2W(settings.getMessage("SETTINGS_OUTGOING_TIP")));
    filterBox->Add(filterOutCheckbox, 0, wxALIGN_LEFT | wxALL, 5);

    filterInCheckbox =  new pmCheckBox(frame, ID_FILTERINCHECKBOX,
        S2W(settings.getMessage("LB_SETTINGS_INCOMING")));
    filterInCheckbox->SetHelpText(S2W(settings.getMessage("SETTINGS_INCOMING_TIP")));
    filterBox->Add(filterInCheckbox, 0, wxALIGN_LEFT | wxALL, 5);

    filterTextCheckbox =  new pmCheckBox(frame, ID_FILTERTEXTCHECKBOX,
        S2W(settings.getMessage("LB_SETTINGS_TEXT")));
    filterTextCheckbox->SetHelpText(S2W(settings.getMessage("SETTINGS_TEXT_TIP")));
    filterBox->Add(filterTextCheckbox, 0, wxALIGN_LEFT | wxALL, 5);

    filterGifCheckbox =  new pmCheckBox(frame, ID_FILTERGIFCHECKBOX,
        S2W(settings.getMessage("LB_SETTINGS_GIF")));
    filterGifCheckbox->SetHelpText(S2W(settings.getMessage("SETTINGS_GIF_TIP")));
    filterBox->Add(filterGifCheckbox, 0, wxALIGN_LEFT | wxALL, 5);

    // Statistics

    wxStaticBox* statsStaticBox_StaticBoxObj = new wxStaticBox(frame, wxID_ANY,
        S2W(settings.getMessage("LB_MONITOR_STATISTICS")));
    pmStaticBoxSizer* statsStaticBox = new pmStaticBoxSizer(
        statsStaticBox_StaticBoxObj, wxHORIZONTAL);
    rightBox->Add(statsStaticBox, 1, wxGROW | wxALL, 5);

    wxBoxSizer* statsBox = new wxBoxSizer(wxVERTICAL);
    statsStaticBox->Add(statsBox, 1, wxALIGN_TOP | wxALL, 0);

    wxFlexGridSizer* statsGrid = new wxFlexGridSizer(2, 2, 5, 5);
    statsBox->Add(statsGrid, 0, wxALIGN_LEFT | wxALL, 5);

    pmStaticText* openReqLabel =  new pmStaticText(frame, wxID_ANY,
        S2W(settings.getMessage("LB_MONITOR_OPENREQ")));
    statsGrid->Add(openReqLabel, 0, wxALIGN_LEFT |
        wxALIGN_CENTER_VERTICAL| wxALL, 0);

    openReqValue =  new pmStaticText(frame, wxID_ANY, wxT(""));
    statsGrid->Add(openReqValue, 0, wxALIGN_LEFT |
        wxALIGN_CENTER_VERTICAL| wxALL, 0);

    pmStaticText* openCnxLabel =  new pmStaticText(frame, wxID_ANY,
        S2W(settings.getMessage("LB_MONITOR_OPENCNX")));
    statsGrid->Add(openCnxLabel, 0, wxALIGN_LEFT |
        wxALIGN_CENTER_VERTICAL| wxALL, 0);

    openCnxValue =  new pmStaticText(frame, wxID_ANY, wxT(""));
    statsGrid->Add(openCnxValue, 0, wxALIGN_LEFT |
        wxALIGN_CENTER_VERTICAL| wxALL, 0);
    updateStatus();

    pmButton* abortButton =  new pmButton(frame, ID_ABORTBUTTON,
        S2W(settings.getMessage("LB_MONITOR_ABORT")));
    abortButton->SetHelpText(S2W(settings.getMessage("MONITOR_ABORT_TIP")));
    statsBox->Add(abortButton, 0, wxALIGN_LEFT | wxALL, 5);

    // Ready

    makeSizer();
    CLog::ref().proxyListeners.insert(this);
}


/* Destructor
 */
CWelcomeScreen::~CWelcomeScreen() {

    CLog::ref().proxyListeners.erase(this);
}


/* Update checkbox display (may have been changed via tray icon)
 */
void CWelcomeScreen::revert(bool confirm) {

    // Populate controls
    filterInCheckbox->SetValue(settings.filterIn);
    filterOutCheckbox->SetValue(settings.filterOut);
    filterTextCheckbox->SetValue(settings.filterText);
    filterGifCheckbox->SetValue(settings.filterGif);

    // Populate configuration list
    configDropdown->Clear();
    for (map<string, set<int> >::iterator it = settings.configs.begin();
             it != settings.configs.end(); it++) {
        configDropdown->Append(S2W(it->first));
    }
    configDropdown->SetValue(S2W(settings.currentConfig));
}


/* Event handling function
 */
void CWelcomeScreen::OnCommand(wxCommandEvent& event) {
    string value;

    switch (event.GetId()) {
    case ID_ABORTBUTTON:
        CProxy::ref().abortConnections(); break;

    case ID_FILTERINCHECKBOX:
        settings.filterIn = filterInCheckbox->GetValue(); break;

    case ID_FILTEROUTCHECKBOX:
        settings.filterOut = filterOutCheckbox->GetValue(); break;

    case ID_FILTERTEXTCHECKBOX:
        settings.filterText = filterTextCheckbox->GetValue(); break;

    case ID_FILTERGIFCHECKBOX:
        settings.filterGif = filterGifCheckbox->GetValue(); break;

    case ID_CONFIGDROPDOWN:
        settings.currentConfig = W2S(configDropdown->GetValue());
        CProxy::ref().refreshManagers();
        break;

    default:
        event.Skip();
    }
}


void CWelcomeScreen::updateStatus() {

    wxString s1, s2;
    s1 << CLog::ref().numOpenSockets;
    s2 << CLog::ref().numActiveRequests;
    openCnxValue->SetLabel(s1);
    openReqValue->SetLabel(s2);
}

/* Proxy event listener
 */
void CWelcomeScreen::OnProxyEvent(CProxyEvent& evt) {

    updateStatus();
}
