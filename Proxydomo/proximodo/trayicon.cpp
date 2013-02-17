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


#include "trayicon.h"
#include "images/icon32.xpm"
#include "settings.h"
#include "log.h"
#include "proxy.h"
#include "mainframe.h"
#include "util.h"
#include "const.h"
#include "logframe.h"
#include <wx/menu.h>
#include <wx/icon.h>
#include <wx/app.h>
#include <vector>
#include <map>
#include <string>

using namespace std;

BEGIN_EVENT_TABLE(CTrayIcon, wxTaskBarIcon)
    EVT_TASKBAR_LEFT_DOWN (CTrayIcon::OnClick)
    EVT_MENU_RANGE (ID_LOG, ID_LOG+200, CTrayIcon::OnCommand)
END_EVENT_TABLE()


/* Constructor
 */
CTrayIcon::CTrayIcon() : wxTaskBarIcon() {

    SetIcon(wxIcon(icon32_xpm), wxT(APP_NAME));
}


/* Destructor
 */
CTrayIcon::~CTrayIcon() {

    CLog::ref().trayIcon = NULL;
}


/* Opens the main frame (the user left click on the tray icon)
 */
void CTrayIcon::OnClick(wxTaskBarIconEvent& event) {

    if (!CLog::ref().mainFrame)
        CLog::ref().mainFrame = new CMainFrame(wxDefaultPosition);
    CUtil::show(CLog::ref().mainFrame);
}


/* Processes events from menu items
 */
void CTrayIcon::OnCommand(wxCommandEvent& event) {

    CSettings& settings = CSettings::ref();

    switch (event.GetId()) {
    case ID_LOG:
        if (!CLog::ref().logFrame)
            CLog::ref().logFrame = new CLogFrame();
        CUtil::show(CLog::ref().logFrame);
        break;

    case ID_EXIT:
        if (CLog::ref().trayIcon) delete CLog::ref().trayIcon;
        if (CLog::ref().logFrame) CLog::ref().logFrame->Destroy();
        if (CLog::ref().mainFrame) CLog::ref().mainFrame->Destroy();
        break;

    case ID_FILTERNONE:
        settings.filterIn = false;
        settings.filterOut = false;
        settings.filterText = false;
        settings.filterGif = false;
        break;
        
    case ID_FILTERALL:
        settings.filterIn = true;
        settings.filterOut = true;
        settings.filterText = true;
        settings.filterGif = true;
        break;

    case ID_FILTERIN:
        settings.filterIn = !settings.filterIn;
        break;

    case ID_FILTEROUT:
        settings.filterOut = !settings.filterOut;
        break;

    case ID_FILTERTEXT:
        settings.filterText = !settings.filterText;
        break;

    case ID_FILTERGIF:
        settings.filterGif = !settings.filterGif;
        break;

    case ID_USEPROXY:
        settings.useNextProxy = !settings.useNextProxy;
        CProxy::ref().refreshManagers();
        break;

    default: // (a configuration)
        {
            int num = event.GetId() - ID_CONFIG;
            if (num < 0 || num >= (int)settings.configs.size()) break;
            map<string,set<int> >::iterator it = settings.configs.begin();
            while (num--) it++;
            string name = it->first;
            if (name == settings.currentConfig) break;
            settings.currentConfig = name;
            CProxy::ref().refreshManagers();
        }
    }
}


/* Creates the popup menu
 */
wxMenu* CTrayIcon::CreatePopupMenu() {

    CSettings& settings = CSettings::ref();

    wxMenu* menuConfig = new wxMenu();
    menuConfig->Append(wxID_ANY, S2W(settings.currentConfig));
    menuConfig->AppendSeparator();
    int num = ID_CONFIG;
    for (map<string,set<int> >::iterator it = settings.configs.begin();
                it != settings.configs.end(); it++) {
        if (it->first != settings.currentConfig)
            menuConfig->Append(num, S2W(it->first));
        num++;
    }

    wxMenu* menuFilter = new wxMenu();
    menuFilter->Append(ID_FILTERALL,
        S2W(settings.getMessage("TRAY_FILTERALL")));
    menuFilter->Append(ID_FILTERNONE,
        S2W(settings.getMessage("TRAY_FILTERNONE")));
    menuFilter->AppendSeparator();
    menuFilter->AppendCheckItem(ID_FILTERIN,
        S2W(settings.getMessage("TRAY_FILTERIN")));
    menuFilter->AppendCheckItem(ID_FILTEROUT,
        S2W(settings.getMessage("TRAY_FILTEROUT")));
    menuFilter->AppendCheckItem(ID_FILTERTEXT,
        S2W(settings.getMessage("TRAY_FILTERTEXT")));
    menuFilter->AppendCheckItem(ID_FILTERGIF,
        S2W(settings.getMessage("TRAY_FILTERGIF")));
    if (settings.filterIn)   menuFilter->Check(ID_FILTERIN, true);
    if (settings.filterOut)  menuFilter->Check(ID_FILTEROUT, true);
    if (settings.filterText) menuFilter->Check(ID_FILTERTEXT, true);
    if (settings.filterGif)  menuFilter->Check(ID_FILTERGIF, true);

    wxMenu* menu = new wxMenu();
    menu->Append(wxID_ANY,
        S2W(settings.getMessage("TRAY_FILTER")), menuFilter);
    menu->Append(wxID_ANY,
        S2W(settings.getMessage("TRAY_CONFIG")), menuConfig);
    if (settings.useNextProxy || !settings.nextProxy.empty()) {
        menu->AppendCheckItem(ID_USEPROXY,
            S2W(settings.getMessage("TRAY_USEPROXY")));
        if (settings.useNextProxy) menu->Check(ID_USEPROXY, true);
    }
    menu->Append(ID_LOG,
        S2W(settings.getMessage("TRAY_LOG")));
    menu->AppendSeparator();
    menu->Append(ID_EXIT,
        S2W(settings.getMessage("TRAY_EXIT")));
    return menu;
}

