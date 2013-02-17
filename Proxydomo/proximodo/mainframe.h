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


#ifndef __mainframe__
#define __mainframe__

#include <wx/frame.h>
class CProxyEvent;
class CStatusEvent;
class CWindowContent;
class wxStatusBar;

/* This class creates the main Proximodo window, with toolbar and status bar.
 * The controls displayed within the window are created and managed by a
 * sizer attributed to the main window.
 */
class CMainFrame : public wxFrame {

public:    
    CMainFrame(const wxPoint& position);
    ~CMainFrame();

private:
    // Event handling functions
    void OnIconize(wxIconizeEvent& event);
    void OnCommand(wxCommandEvent& event);
    void OnProxyEvent(CProxyEvent& event);
    void OnStatusEvent(CStatusEvent& event);
    void OnMoveEvent(wxMoveEvent& event);

    // Variables
    wxStatusBar* statusbar;
    CWindowContent* content;  // destroyed with SetSizer(NULL) or
                              // by creating another CWindowContent

    // Functions
    void updateStatusBar();
    
    // Saved window position
    static int savedX, savedY;

    // IDs
    enum {
        // Buttons
        ID_SETTINGS = 1100,
        ID_MONITOR,
        ID_LOG,
        ID_FILTERS,
        ID_CONFIG,
        ID_HELP,
        ID_QUIT,
        // Menu items
        ID_TOOLSSETTINGS,
        ID_TOOLSMONITOR,
        ID_TOOLSCONFIG,
        ID_TOOLSFILTERS,
        ID_TOOLSLOG,
        ID_TOOLSSAVE,
        ID_TOOLSRELOAD,
        ID_TOOLSQUIT,
        ID_TOOLSICONIZE,
        ID_HELPABOUT,
        ID_HELPCONTENT,
        ID_HELPLICENSE,
        ID_HELPSYNTAX
    };

    // For event management
    DECLARE_EVENT_TABLE()
};

#endif

