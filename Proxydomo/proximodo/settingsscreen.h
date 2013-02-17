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


#ifndef __settingsscreen__
#define __settingsscreen__

#include "windowcontent.h"
#include <string>
#include <set>
#include <map>
class pmCheckBox;
class pmComboBox;
class pmTextCtrl;

using namespace std;

/* This class creates a box sizer to be set as the main frame sizer.
 * It displays all Proximodo settings, except filters and configurations.
 */
class CSettingsScreen : public CWindowContent {

public:
    CSettingsScreen(wxFrame* frame);
    ~CSettingsScreen();

private:
    // Variable management functions
    void revert(bool confirm);
    void apply(bool confirm);
    bool hasChanged();

    // Managed variables
    string         language;
    string         proxyPort;
    bool           enableUrlCmd;
    string         urlCmdPrefix;
    bool           useNextProxy;
    string         nextProxy;
    set<string>    proxies;
    bool           allowIPRange;
    unsigned long  minIPRange;
    unsigned long  maxIPRange;
    string         bypass;
    bool           showOnStartup;
    bool           startBrowser;
    string         browserPath;
    map<string,string> listNames;
    
    // GUI variables
    string currentListName;

    // Controls
    pmCheckBox *showGuiCheckbox;
    pmCheckBox *startBrowserCheckbox;
    pmCheckBox *allowIPCheckbox;
    pmCheckBox *useProxyCheckbox;
    pmCheckBox *urlCmdCheckbox;
    pmComboBox *languageDropDown;
    pmComboBox *nextProxyDropdown;
    pmComboBox *listDropDown;
    pmTextCtrl *listFileText;
    pmTextCtrl *bypassText;
    pmTextCtrl *browserPathText;
    pmTextCtrl *maxRangeText;
    pmTextCtrl *minRangeText;
    pmTextCtrl *portText;
    pmTextCtrl *urlCmdPrefixText;

    // Event handling function
    void OnCommand(wxCommandEvent& event);

    // IDs
    enum {
        ID_ALLOWIPCHECKBOX = 1300,
        ID_SHOWGUICHECKBOX,
        ID_STARTBROWSERCHECKBOX,
        ID_USEPROXYCHECKBOX,
        ID_URLCMDCHECKBOX,
        ID_APPLYBUTTON,
        ID_REVERTBUTTON,
        ID_NEXTPROXYBUTTON,
        ID_BYPASSTEXT,
        ID_MAXRANGETEXT,
        ID_MINRANGETEXT,
        ID_LISTFILETEXT,
        ID_BROWSERPATHTEXT,
        ID_PORTTEXT,
        ID_URLCMDPREFIXTEXT,
        ID_LANGUAGEDROPDOWN,
        ID_NEXTPROXYDROPDOWN,
        ID_LISTNAMEDROPDOWN,
        ID_NEWBUTTON,
        ID_OPENBUTTON,
        ID_BROWSERPATHBUTTON,
        ID_CHOOSEBUTTON
    };

    // Event table
    DECLARE_EVENT_TABLE()
};

#endif
// vi:ts=4:sw=4:et
