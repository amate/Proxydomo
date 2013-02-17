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


#include <wx/app.h>
#include <wx/snglinst.h>
#include "proxy.h"
#include "const.h"
#include "settings.h"
#include "mainframe.h"
#include "util.h"
#include "log.h"
#include "trayicon.h"

class ProximodoApp : public wxApp {

public:
    virtual bool OnInit();
    virtual int OnExit();

private:
    wxSingleInstanceChecker* sic;
};


IMPLEMENT_APP(ProximodoApp)


bool ProximodoApp::OnInit() {

    // Initialize sockets
    wxSocketBase::Initialize();

    // Check that there is no other Proximodo running
    const wxString name = wxT(APP_NAME);
    sic = new wxSingleInstanceChecker(name);
    if (sic->IsAnotherRunning()) // Verifies this user is not running Proximodo
    {
        // If configured, still open the browser
        if (CSettings::ref().startBrowser
                && !CSettings::ref().browserPath.empty()) {
            wxExecute(S2W(CSettings::ref().browserPath), wxEXEC_ASYNC);
        }
        return false;
    }
    
    // Start proxy server
    CProxy::ref().openProxyPort();

    // Create and display main frame
    if (CSettings::ref().showOnStartup) {
        CLog::ref().mainFrame = new CMainFrame(wxDefaultPosition);
        CLog::ref().mainFrame->Show();
    }
    
    // Open welcome html page on first run
    if (CSettings::ref().firstRun) {
        CSettings::ref().firstRun = false;
        CSettings::ref().save();
        CUtil::openBrowser(CSettings::ref().getMessage("HELP_PAGE_WELCOME"));
    } else if (CSettings::ref().startBrowser
                && !CSettings::ref().browserPath.empty()) {
        // Open browser
        wxExecute(S2W(CSettings::ref().browserPath), wxEXEC_ASYNC);
    }

    // Create tray icon
    CLog::ref().trayIcon = new CTrayIcon();

    return true;
}


int ProximodoApp::OnExit() {

    CProxy::destroy();
    CLog::destroy();
    CSettings::destroy();
    delete sic;
    return 0;
}
