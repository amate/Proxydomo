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


#include "settingsscreen.h"
#include "mainframe.h"
#include "settings.h"
#include "util.h"
#include "const.h"
#include "proxy.h"
#include "controls.h"
#include "matcher.h"
#include "images/btn_add20.xpm"
#include "images/btn_view20.xpm"
#include "images/btn_file20.xpm"
#include "images/btn_trash20.xpm"
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/event.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/event.h>
#include <wx/textfile.h>
#include <map>
#include <algorithm>
#include <sstream>

using namespace std;

/* Events
 */
BEGIN_EVENT_TABLE(CSettingsScreen, CWindowContent)
    EVT_BUTTON     (ID_APPLYBUTTON,          CSettingsScreen::OnCommand)
    EVT_BUTTON     (ID_REVERTBUTTON,         CSettingsScreen::OnCommand)
    EVT_BUTTON     (ID_NEXTPROXYBUTTON,      CSettingsScreen::OnCommand)
    EVT_BUTTON     (ID_NEWBUTTON,            CSettingsScreen::OnCommand)
    EVT_BUTTON     (ID_OPENBUTTON,           CSettingsScreen::OnCommand)
    EVT_BUTTON     (ID_BROWSERPATHBUTTON,    CSettingsScreen::OnCommand)
    EVT_BUTTON     (ID_CHOOSEBUTTON,         CSettingsScreen::OnCommand)
    EVT_CHECKBOX   (ID_ALLOWIPCHECKBOX,      CSettingsScreen::OnCommand)
    EVT_CHECKBOX   (ID_STARTBROWSERCHECKBOX, CSettingsScreen::OnCommand)
    EVT_CHECKBOX   (ID_SHOWGUICHECKBOX,      CSettingsScreen::OnCommand)
    EVT_CHECKBOX   (ID_USEPROXYCHECKBOX,     CSettingsScreen::OnCommand)
    EVT_CHECKBOX   (ID_URLCMDCHECKBOX,       CSettingsScreen::OnCommand)
    EVT_COMBOBOX   (ID_LANGUAGEDROPDOWN,     CSettingsScreen::OnCommand)
    EVT_COMBOBOX   (ID_NEXTPROXYDROPDOWN,    CSettingsScreen::OnCommand)
    EVT_COMBOBOX   (ID_LISTNAMEDROPDOWN,     CSettingsScreen::OnCommand)
    EVT_TEXT_ENTER (ID_LISTNAMEDROPDOWN,     CSettingsScreen::OnCommand)
    EVT_TEXT_ENTER (ID_NEXTPROXYDROPDOWN,    CSettingsScreen::OnCommand)
    EVT_TEXT_ENTER (ID_BYPASSTEXT,           CSettingsScreen::OnCommand)
    EVT_TEXT_ENTER (ID_MAXRANGETEXT,         CSettingsScreen::OnCommand)
    EVT_TEXT_ENTER (ID_MINRANGETEXT,         CSettingsScreen::OnCommand)
    EVT_TEXT_ENTER (ID_BROWSERPATHTEXT,      CSettingsScreen::OnCommand)
    EVT_TEXT_ENTER (ID_PORTTEXT,             CSettingsScreen::OnCommand)
    EVT_TEXT_ENTER (ID_LISTFILETEXT,         CSettingsScreen::OnCommand)
    EVT_TEXT_ENTER (ID_URLCMDPREFIXTEXT,     CSettingsScreen::OnCommand)
END_EVENT_TABLE()


/* Constructor
 */
CSettingsScreen::CSettingsScreen(wxFrame* frame) : CWindowContent(frame) {

    wxBoxSizer* leftBox = new wxBoxSizer(wxVERTICAL);
    Add(leftBox,1,wxGROW | wxALL,0);

    wxBoxSizer* rightBox = new wxBoxSizer(wxVERTICAL);
    Add(rightBox,1,wxGROW | wxALL,0);

    // Filtering options

    wxStaticBox* filterStaticBox_StaticBoxObj = new wxStaticBox(frame,wxID_ANY,
        S2W(settings.getMessage("LB_SETTINGS_FILTERING")));
    pmStaticBoxSizer* filterStaticBox = new pmStaticBoxSizer(
        filterStaticBox_StaticBoxObj,wxHORIZONTAL);
    leftBox->Add(filterStaticBox,0,wxGROW | wxALL,5);

    wxBoxSizer* filterBox = new wxBoxSizer(wxVERTICAL);
    filterStaticBox->Add(filterBox,1,wxALIGN_TOP | wxALL,0);

    wxBoxSizer* bypassBox = new wxBoxSizer(wxHORIZONTAL);
    filterBox->Add(bypassBox,0,wxGROW | wxALL,0);

    pmStaticText* bypassLabel =  new pmStaticText(frame, wxID_ANY ,
        S2W(settings.getMessage("LB_SETTINGS_BYPASS")));
    bypassBox->Add(bypassLabel,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    bypassText =  new pmTextCtrl(frame, ID_BYPASSTEXT,
        wxT("") , wxDefaultPosition, wxDefaultSize);
    bypassText->SetHelpText(S2W(settings.getMessage("SETTINGS_BYPASS_TIP")));
    bypassBox->Add(bypassText,1,wxALIGN_CENTER_VERTICAL | wxALL,5);

    // GUI options

    wxStaticBox* guiStaticBox_StaticBoxObj = new wxStaticBox(frame,wxID_ANY,
        S2W(settings.getMessage("LB_SETTINGS_GUI")));
    pmStaticBoxSizer* guiStaticBox = new pmStaticBoxSizer(
        guiStaticBox_StaticBoxObj,wxHORIZONTAL);
    rightBox->Add(guiStaticBox,0,wxGROW | wxALL,5);

    wxBoxSizer* guiBox = new wxBoxSizer(wxVERTICAL);
    guiStaticBox->Add(guiBox,1,wxALIGN_TOP | wxALL,0);

    wxBoxSizer* languageBox = new wxBoxSizer(wxHORIZONTAL);
    guiBox->Add(languageBox,0,wxGROW | wxALL,0);

    pmStaticText* languageLabel =  new pmStaticText(frame, wxID_ANY ,
        S2W(settings.getMessage("LB_SETTINGS_LANGUAGE")));
    languageBox->Add(languageLabel,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    wxArrayString choices;
    languageDropDown =  new pmComboBox(frame, ID_LANGUAGEDROPDOWN ,
        wxT("") , wxDefaultPosition, wxDefaultSize, choices,
        wxCB_DROPDOWN | wxCB_READONLY | wxCB_SORT );
    languageDropDown->SetHelpText(S2W(settings.getMessage("SETTINGS_LANGUAGE_TIP")));
    languageBox->Add(languageDropDown,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    showGuiCheckbox =  new pmCheckBox(frame, ID_SHOWGUICHECKBOX,
        S2W(settings.getMessage("LB_SETTINGS_SHOWGUI")));
    showGuiCheckbox->SetHelpText(S2W(settings.getMessage("SETTINGS_SHOWGUI_TIP")));
    guiBox->Add(showGuiCheckbox,0,wxALIGN_LEFT | wxALL,5);

    startBrowserCheckbox =  new pmCheckBox(frame, ID_STARTBROWSERCHECKBOX,
        S2W(settings.getMessage("LB_SETTINGS_STARTBROWSER")));
    startBrowserCheckbox->SetHelpText(S2W(settings.getMessage("SETTINGS_STARTBROWSER_TIP")));
    guiBox->Add(startBrowserCheckbox,0,wxALIGN_LEFT | wxALL,5);

    wxBoxSizer* browserPathBox = new wxBoxSizer(wxHORIZONTAL);
    guiBox->Add(browserPathBox,0,wxGROW | wxALL,0);

    browserPathText =  new pmTextCtrl(frame, ID_BROWSERPATHTEXT,
        wxT("") , wxDefaultPosition, wxDefaultSize);
    browserPathText->SetHelpText(S2W(settings.getMessage("SETTINGS_BROWSERPATH_TIP")));
    browserPathBox->Add(browserPathText,1,wxALIGN_CENTER_VERTICAL | wxALL,5);

    pmBitmapButton* browserButton =  new pmBitmapButton(frame, ID_BROWSERPATHBUTTON,
        wxBitmap(btn_file20_xpm) );
    browserButton->SetHelpText(S2W(settings.getMessage("SETTINGS_BROWSERCHOOSE_TIP")));
    browserPathBox->Add(browserButton,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    // Proxy options

    wxStaticBox* proxyStaticBox_StaticBoxObj = new wxStaticBox(frame,wxID_ANY,
        S2W(settings.getMessage("LB_SETTINGS_PROXY")));
    pmStaticBoxSizer* proxyStaticBox = new pmStaticBoxSizer(
        proxyStaticBox_StaticBoxObj,wxHORIZONTAL);
    leftBox->Add(proxyStaticBox,1,wxGROW | wxALL,5);

    wxBoxSizer* proxyBox = new wxBoxSizer(wxVERTICAL);
    proxyStaticBox->Add(proxyBox,1,wxALIGN_TOP | wxALL,0);

    wxBoxSizer* portBox = new wxBoxSizer(wxHORIZONTAL);
    proxyBox->Add(portBox,0,wxALIGN_LEFT | wxALL,0);

    pmStaticText* portLabel =  new pmStaticText(frame, wxID_ANY ,
        S2W(settings.getMessage("LB_SETTINGS_PORT")));
    portBox->Add(portLabel,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    urlCmdCheckbox = new pmCheckBox(frame, ID_URLCMDCHECKBOX,
        S2W(settings.getMessage("LB_SETTINGS_URLCMD")));
    urlCmdCheckbox->SetHelpText(S2W(settings.getMessage("SETTINGS_URLCMD_TIP")));
    proxyBox->Add(urlCmdCheckbox,0,wxALIGN_LEFT | wxALL,5);

    wxBoxSizer* urlCmdBox = new wxBoxSizer(wxHORIZONTAL);
    proxyBox->Add(urlCmdBox,0,wxALIGN_LEFT | wxALL,0);

    pmStaticText* urlCmdLabel =  new pmStaticText(frame, wxID_ANY ,
        S2W(settings.getMessage("LB_SETTINGS_URLCMD_PREFIX")));
    urlCmdBox->Add(urlCmdLabel,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    urlCmdPrefixText = new pmTextCtrl(frame, ID_URLCMDPREFIXTEXT,
        wxT("") , wxDefaultPosition, wxSize(100, 21));
    urlCmdPrefixText->SetHelpText(S2W(settings.getMessage("SETTINGS_URLCMD_PREFIX_TIP")));
    urlCmdBox->Add(urlCmdPrefixText,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    portText = new pmTextCtrl(frame, ID_PORTTEXT,
        wxT("") , wxDefaultPosition, wxSize(50, 21));
    portText->SetHelpText(S2W(settings.getMessage("SETTINGS_PORT_TIP")));
    portBox->Add(portText,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    useProxyCheckbox = new pmCheckBox(frame, ID_USEPROXYCHECKBOX,
        S2W(settings.getMessage("LB_SETTINGS_USEPROXY")));
    useProxyCheckbox->SetHelpText(S2W(settings.getMessage("SETTINGS_USEPROXY_TIP")));
    proxyBox->Add(useProxyCheckbox,0,wxALIGN_LEFT | wxALL,5);

    wxBoxSizer* nextProxyBox = new wxBoxSizer(wxHORIZONTAL);
    proxyBox->Add(nextProxyBox,0,wxGROW | wxALL,0);

    pmStaticText* proxyLabel =  new pmStaticText(frame, wxID_ANY ,
        S2W(settings.getMessage("LB_SETTINGS_NEXTPROXY")));
    nextProxyBox->Add(proxyLabel,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    nextProxyDropdown =  new pmComboBox(frame, ID_NEXTPROXYDROPDOWN ,
        wxT("") , wxDefaultPosition, wxDefaultSize,
        choices, wxCB_DROPDOWN | wxCB_SORT  );
    nextProxyDropdown->SetHelpText(S2W(settings.getMessage("SETTINGS_NEXTPROXY_TIP")));
    nextProxyBox->Add(nextProxyDropdown,1,wxALIGN_CENTER_VERTICAL | wxALL,5);

    pmBitmapButton* nextProxyButton =  new pmBitmapButton(frame, ID_NEXTPROXYBUTTON,
        wxBitmap(btn_trash20_xpm) );
    nextProxyButton->SetHelpText(S2W(settings.getMessage("SETTINGS_REMOVEPROXY_TIP")));
    nextProxyBox->Add(nextProxyButton,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    allowIPCheckbox =  new pmCheckBox(frame, ID_ALLOWIPCHECKBOX,
        S2W(settings.getMessage("LB_SETTINGS_ALLOWIP")));
    allowIPCheckbox->SetHelpText(S2W(settings.getMessage("SETTINGS_ALLOWIP_TIP")));
    proxyBox->Add(allowIPCheckbox,0,wxALIGN_LEFT | wxALL,5);

    wxFlexGridSizer* rangeBox = new wxFlexGridSizer(2,2,5,5);
    proxyBox->Add(rangeBox,0,wxALIGN_LEFT | wxALL,5);

    pmStaticText* minRangeLabel =  new pmStaticText(frame, wxID_ANY ,
        S2W(settings.getMessage("LB_SETTINGS_MINRANGE")));
    rangeBox->Add(minRangeLabel,0,wxALIGN_LEFT |
        wxALIGN_CENTER_VERTICAL| wxALL,0);

    minRangeText =  new pmTextCtrl(frame, ID_MINRANGETEXT,
        wxT("") , wxDefaultPosition, wxDefaultSize);
    minRangeText->SetHelpText(S2W(settings.getMessage("SETTINGS_MINRANGE_TIP")));
    rangeBox->Add(minRangeText,0,wxALIGN_CENTER_HORIZONTAL |
        wxALIGN_CENTER_VERTICAL | wxALL,0);

    pmStaticText* maxRangeLabel =  new pmStaticText(frame, wxID_ANY ,
        S2W(settings.getMessage("LB_SETTINGS_MAXRANGE")));
    rangeBox->Add(maxRangeLabel,0,wxALIGN_LEFT |
        wxALIGN_CENTER_VERTICAL| wxALL,0);

    maxRangeText =  new pmTextCtrl(frame, ID_MAXRANGETEXT,
        wxT("") , wxDefaultPosition, wxDefaultSize);
    maxRangeText->SetHelpText(S2W(settings.getMessage("SETTINGS_MAXRANGE_TIP")));
    rangeBox->Add(maxRangeText,0,wxALIGN_CENTER_HORIZONTAL |
        wxALIGN_CENTER_VERTICAL | wxALL,0);

    // List options

    wxStaticBox* listStaticBox_StaticBoxObj = new wxStaticBox(frame,wxID_ANY,
        S2W(settings.getMessage("LB_SETTINGS_LIST")));
    pmStaticBoxSizer* listStaticBox = new pmStaticBoxSizer(
        listStaticBox_StaticBoxObj,wxHORIZONTAL);
    rightBox->Add(listStaticBox,1,wxGROW | wxALL,5);

    wxBoxSizer* listBox = new wxBoxSizer(wxVERTICAL);
    listStaticBox->Add(listBox,1,wxALIGN_TOP | wxALL,0);

    wxBoxSizer* listNameBox = new wxBoxSizer(wxHORIZONTAL);
    listBox->Add(listNameBox,0,wxGROW | wxALL,0);

    pmStaticText* listNameLabel =  new pmStaticText(frame, wxID_ANY ,
        S2W(settings.getMessage("LB_SETTINGS_LISTNAME")));
    listNameBox->Add(listNameLabel,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    listDropDown =  new pmComboBox(frame, ID_LISTNAMEDROPDOWN ,
        wxT("") , wxDefaultPosition, wxSize(70, 21),
        choices, wxCB_DROPDOWN | wxCB_SORT  );
    listDropDown->SetHelpText(S2W(settings.getMessage("SETTINGS_LISTNAME_TIP")));
    listNameBox->Add(listDropDown,1,wxALIGN_CENTER_VERTICAL | wxALL,5);

    pmBitmapButton* newButton =  new pmBitmapButton(frame, ID_NEWBUTTON,
        wxBitmap(btn_add20_xpm) );
    newButton->SetHelpText(S2W(settings.getMessage("SETTINGS_NEWLIST_TIP")));
    listNameBox->Add(newButton,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    pmBitmapButton* openButton =  new pmBitmapButton(frame, ID_OPENBUTTON,
        wxBitmap(btn_view20_xpm) );
    openButton->SetHelpText(S2W(settings.getMessage("SETTINGS_OPENLIST_TIP")));
    listNameBox->Add(openButton,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    wxBoxSizer* listFileBox = new wxBoxSizer(wxHORIZONTAL);
    listBox->Add(listFileBox,0,wxGROW | wxALL,0);

    pmStaticText* listFileLabel =  new pmStaticText(frame, wxID_ANY ,
        S2W(settings.getMessage("LB_SETTINGS_LISTFILE")));
    listFileBox->Add(listFileLabel,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    listFileText =  new pmTextCtrl(frame, ID_LISTFILETEXT,
        wxT("") , wxDefaultPosition, wxDefaultSize);
    listFileText->SetHelpText(S2W(settings.getMessage("SETTINGS_LISTFILE_TIP")));
    listFileBox->Add(listFileText,1,wxALIGN_CENTER_VERTICAL | wxALL,5);

    pmBitmapButton* chooseButton =  new pmBitmapButton(frame, ID_CHOOSEBUTTON,
        wxBitmap(btn_file20_xpm) );
    chooseButton->SetHelpText(S2W(settings.getMessage("SETTINGS_FILECHOOSE_TIP")));
    listFileBox->Add(chooseButton,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    // More buttons

    wxBoxSizer* buttonBox = new wxBoxSizer(wxHORIZONTAL);
    rightBox->Add(buttonBox,0,wxALIGN_CENTER_HORIZONTAL | wxALL,0);

    pmButton* applyButton =  new pmButton(frame, ID_APPLYBUTTON,
        S2W(settings.getMessage("LB_SETTINGS_APPLY")));
    applyButton->SetHelpText(S2W(settings.getMessage("SETTINGS_APPLY_TIP")));
    buttonBox->Add(applyButton,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    pmButton* revertButton =  new pmButton(frame, ID_REVERTBUTTON,
        S2W(settings.getMessage("LB_SETTINGS_REVERT")));
    revertButton->SetHelpText(S2W(settings.getMessage("SETTINGS_REVERT_TIP")));
    buttonBox->Add(revertButton,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    // Ready

    makeSizer();
}


/* Destructor
 */
CSettingsScreen::~CSettingsScreen() {

    apply(true);
}


/* Test if local variables differ from settings
 */
bool CSettingsScreen::hasChanged() {

    commitText();
    return (language      != settings.language      ||
            proxyPort     != settings.proxyPort     ||
            enableUrlCmd  != settings.enableUrlCmd  ||
            urlCmdPrefix  != settings.urlCmdPrefix  ||
            useNextProxy  != settings.useNextProxy  ||
            nextProxy     != settings.nextProxy     ||
            allowIPRange  != settings.allowIPRange  ||
            minIPRange    != settings.minIPRange    ||
            maxIPRange    != settings.maxIPRange    ||
            bypass        != settings.bypass        ||
            proxies       != settings.proxies       ||
            showOnStartup != settings.showOnStartup ||
            startBrowser  != settings.startBrowser  ||
            browserPath   != settings.browserPath   ||
            listNames     != settings.listNames     );
}


/* (Re)loads settings into local variables
 */
void CSettingsScreen::revert(bool confirm) {

    // Ask user before proceeding
    if (confirm && hasChanged()) {
        int ret = wxMessageBox(S2W(settings.getMessage("REVERT_SETTINGS")),
                                                wxT(APP_NAME), wxYES_NO);
        if (ret == wxNO) return;
    }

    // Load variables
    language      = settings.language      ;
    proxyPort     = settings.proxyPort     ;
    enableUrlCmd  = settings.enableUrlCmd  ;
    urlCmdPrefix  = settings.urlCmdPrefix  ;
    useNextProxy  = settings.useNextProxy  ;
    nextProxy     = settings.nextProxy     ;
    allowIPRange  = settings.allowIPRange  ;
    minIPRange    = settings.minIPRange    ;
    maxIPRange    = settings.maxIPRange    ;
    bypass        = settings.bypass        ;
    proxies       = settings.proxies       ;
    showOnStartup = settings.showOnStartup ;
    startBrowser  = settings.startBrowser  ;
    browserPath   = settings.browserPath   ;
    listNames     = settings.listNames     ;

    // Populate controls
    allowIPCheckbox->SetValue(allowIPRange);
    startBrowserCheckbox->SetValue(startBrowser);
    showGuiCheckbox->SetValue(showOnStartup);
    useProxyCheckbox->SetValue(useNextProxy);
    urlCmdCheckbox->SetValue(enableUrlCmd);
    bypassText->SetValue(S2W(bypass));
    browserPathText->SetValue(S2W(browserPath));
    maxRangeText->SetValue(S2W(CUtil::toDotted(maxIPRange)));
    minRangeText->SetValue(S2W(CUtil::toDotted(minIPRange)));
    portText->SetValue(S2W(proxyPort));
    urlCmdPrefixText->SetValue(S2W(urlCmdPrefix));

    // Populate proxy list
    nextProxyDropdown->Clear();
    for (set<string>::iterator it = proxies.begin();
            it != proxies.end(); it++) {
        nextProxyDropdown->Append(S2W(*it));
    }
    nextProxyDropdown->SetValue(S2W(nextProxy));

    // Populate language list
    languageDropDown->Clear();
    wxString f = wxFindFirstFile(wxT("*.lng"));
    while ( !f.IsEmpty() ) {
        languageDropDown->Append(f.SubString(2, f.Length() - 5));
        f = wxFindNextFile();
    }
    languageDropDown->SetValue(S2W(language));
    
    // Populate lists list
    listDropDown->Clear();
    for (map<string,string>::iterator it = listNames.begin();
            it != listNames.end(); it++) {
        listDropDown->Append(S2W(it->first));
    }
    currentListName.clear();
    listDropDown->SetValue(wxT(""));
    listFileText->SetValue(wxT(""));

    allowIPCheckbox->SetFocus();
}


/* Copies local variables to settings. The proxy is also restarted.
 */
void CSettingsScreen::apply(bool confirm) {

    if (!hasChanged()) return;
    if (confirm) {
        // Ask user before proceeding
        int ret = wxMessageBox(S2W(settings.getMessage("APPLY_SETTINGS")),
                               wxT(APP_NAME), wxYES_NO);
        if (ret == wxNO) return;
    }
    // Stop proxy
    CProxy::ref().closeProxyPort();
    CProxy::ref().refreshManagers();
    // Apply variables
    settings.useNextProxy  = useNextProxy  ;
    settings.language      = language      ;
    settings.proxyPort     = proxyPort     ;
    settings.enableUrlCmd  = enableUrlCmd  ;
    settings.urlCmdPrefix  = urlCmdPrefix  ;
    settings.nextProxy     = nextProxy     ;
    settings.allowIPRange  = allowIPRange  ;
    settings.minIPRange    = minIPRange    ;
    settings.maxIPRange    = maxIPRange    ;
    settings.bypass        = bypass        ;
    settings.proxies       = proxies       ;
    settings.showOnStartup = showOnStartup ;
    settings.startBrowser  = startBrowser  ;
    settings.browserPath   = browserPath   ;
    settings.listNames     = listNames     ;
    settings.modified      = true;
    // Remove lists with no filename
    map<string,string>::iterator it = settings.listNames.begin();
    while (it != settings.listNames.end()) {
        if (it->second.empty()) {
            settings.listNames.erase(it);
            it = settings.listNames.begin();
        } else it++;
    }
    // Reload lists
    settings.loadLists();
    // Restart proxy
    CProxy::ref().openProxyPort();
}


/* Event handling function
 */
void CSettingsScreen::OnCommand(wxCommandEvent& event) {
    string value;
    
    switch (event.GetId()) {
    case ID_ALLOWIPCHECKBOX:
        allowIPRange = allowIPCheckbox->GetValue(); break;

    case ID_USEPROXYCHECKBOX:
        if (nextProxyDropdown->GetValue().IsEmpty())
            useProxyCheckbox->SetValue(false);
        useNextProxy = useProxyCheckbox->GetValue(); break;

    case ID_URLCMDCHECKBOX:
        enableUrlCmd = urlCmdCheckbox->GetValue(); break;

    case ID_SHOWGUICHECKBOX:
        showOnStartup = showGuiCheckbox->GetValue(); break;

    case ID_STARTBROWSERCHECKBOX:
        startBrowser = startBrowserCheckbox->GetValue(); break;

    case ID_APPLYBUTTON:
        settings.proxyPort.clear(); // (to force apply, in case list files were edited)
        apply(false); break;

    case ID_REVERTBUTTON:
        revert(true); break;

    case ID_LANGUAGEDROPDOWN:
        language = W2S(languageDropDown->GetValue()); break;
        
    case ID_BROWSERPATHTEXT:
        browserPath = W2S(browserPathText->GetValue()); break;

    case ID_BYPASSTEXT:
        {
            string newValue = W2S(bypassText->GetValue());
            string errmsg;
            if (CMatcher::testPattern(newValue, errmsg)) {
                bypass = newValue;
            } else {
                wxMessageBox(S2W(errmsg), wxT(APP_NAME));
            }
            break;
        }
    case ID_MAXRANGETEXT:
        {
            maxIPRange = CUtil::fromDotted(W2S(maxRangeText->GetValue()));
            maxRangeText->SetValue(S2W(CUtil::toDotted(maxIPRange)));
            break;
        }
    case ID_MINRANGETEXT:
        {
            minIPRange = CUtil::fromDotted(W2S(minRangeText->GetValue()));
            minRangeText->SetValue(S2W(CUtil::toDotted(minIPRange)));
            break;
        }
    case ID_PORTTEXT: 
        {
            string newValue = W2S(portText->GetValue());
            stringstream ss(newValue);
            unsigned short num = 0;
            ss >> num;
            if (num > 0) {
                proxyPort = newValue;
                if (!CProxy::ref().testPort(proxyPort)) {
                    wxMessageBox(S2W(settings.getMessage("PORT_UNAVAILABLE")),
                                 wxT(APP_NAME));
                }
            }
            portText->SetValue(S2W(proxyPort));
            break;
        }
    case ID_URLCMDPREFIXTEXT:
        {
            urlCmdPrefix = W2S(urlCmdPrefixText->GetValue());
            break;
        }
    case ID_NEXTPROXYBUTTON:
        {
            set<string>::iterator it = proxies.find(nextProxy);
            if (it != proxies.end())
                proxies.erase(it);
            int pos = nextProxyDropdown->FindString(S2W(nextProxy));
            if (pos != wxNOT_FOUND)
                nextProxyDropdown->Delete(pos);
            if (proxies.empty()) {
                nextProxy = "";
            } else {
                nextProxy = *(proxies.begin());
            }
            nextProxyDropdown->SetValue(S2W(nextProxy));
            break;
        }
    case ID_NEXTPROXYDROPDOWN:
        {
            nextProxy = W2S(nextProxyDropdown->GetValue());
            CUtil::trim(nextProxy);
            if (!nextProxy.empty()) {
                if (nextProxy.find(':') == string::npos) {
                    nextProxy += ":8080";
                    nextProxyDropdown->SetValue(S2W(nextProxy));
                }
                proxies.insert(nextProxy);
                if (nextProxyDropdown->FindString(S2W(nextProxy)) == wxNOT_FOUND) {
                    nextProxyDropdown->Append(S2W(nextProxy));
                    if (!CProxy::ref().testRemoteProxy(nextProxy)) {
                        wxMessageBox(S2W(settings.getMessage("BAD_REMOTE_PROXY")),
                                     wxT(APP_NAME));
                    }
                }
            } else {
                useProxyCheckbox->SetValue(useNextProxy = false);
            }
            break;
        }
    case ID_NEWBUTTON:
        {
            currentListName.clear();
            listDropDown->SetValue(wxT(""));
            listFileText->SetValue(wxT(""));
            listDropDown->SetFocus();
            break;
        }
    case ID_OPENBUTTON:
        {
            if (!currentListName.empty())
                CUtil::openNotepad(listNames[currentListName]);
            break;
        }
    case ID_LISTNAMEDROPDOWN:
        {
            if (event.GetEventType() == wxEVT_COMMAND_TEXT_ENTER) {

                string newListName = W2S(listDropDown->GetValue());
                CUtil::trim(newListName);
                if (newListName != currentListName) {
                    if (!currentListName.empty()) {
                        // Delete/Rename list
                        listNames.erase(currentListName);
                        listDropDown->Delete(
                            listDropDown->FindString(S2W(currentListName)));
                    }
                    if (!newListName.empty()) {
                        // Create/Rename list
                        if (!listFileText->GetValue().empty()) {
                            listNames[newListName] = W2S(listFileText->GetValue());
                            CUtil::trim(listNames[newListName]);
                        }
                        listDropDown->Append(S2W(newListName));
                    } else {
                        listFileText->SetValue(wxT(""));
                    }
                    currentListName = newListName;
                    listDropDown->SetValue(S2W(currentListName));
                }
            } else if (event.GetEventType() == wxEVT_COMMAND_COMBOBOX_SELECTED) {

                currentListName = W2S(event.GetString());
                if (!currentListName.empty())
                    listFileText->SetValue(S2W(listNames[currentListName]));
            }
            break;
        }
    case ID_CHOOSEBUTTON:
        {
            wxFileDialog fd(frame,
                  S2W(settings.getMessage("OPEN_LISTFILE_QUESTION")),
                  wxT("Lists"), // This is the directory name
                  wxT(""), wxT("Text files (*.txt)|*.txt|All files (*.*)|*.*"), wxOPEN);
            if (fd.ShowModal() != wxID_OK) break;
            listFileText->SetValue(fd.GetPath());
            // No break here, we'll do following tests
        }
    case ID_LISTFILETEXT:
        {
            wxFileName fn(listFileText->GetValue());

            fn.MakeAbsolute();
            string fullpath = W2S(fn.GetFullPath());
            
            fn.MakeRelativeTo(wxFileName::GetCwd());
            if (fullpath.size() < fn.GetFullPath().Length())
                fn.MakeAbsolute();
            string path = W2S(fn.GetFullPath());

            if (!currentListName.empty())
                listNames[currentListName] = path;
            listFileText->SetValue(S2W(path));
            
            if (!path.empty() && !fn.FileExists()) {
                int answer = wxMessageBox(
                              S2W(settings.getMessage("ASK_CREATE_NEWLIST")),
                              wxT(APP_NAME), wxYES_NO);
                if (answer == wxYES) {
                    wxTextFile f(S2W(path));
                    f.Create();
                    f.Close();
                }
            }
            break;
        }
    case ID_BROWSERPATHBUTTON:
        {
            wxFileDialog fd(frame,
                  S2W(settings.getMessage("OPEN_BROWSER_QUESTION")),
                  wxT("."), // This is the directory name
                  wxT(""), wxT("All files (*.*)|*.*"), wxOPEN);
            if (fd.ShowModal() != wxID_OK) break;
            wxString path = fd.GetPath();
            if (path.Find(wxT(" ")) >= 0)
                path = (wxString)wxT("\"") + path + wxT("\"");
            browserPathText->SetValue(path);
            browserPath = W2S(path);
            break;
        }
    default:
        event.Skip();
    }
}
// vi:ts=4:sw=4:et
