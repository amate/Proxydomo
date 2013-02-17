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


#ifndef __trayicon__
#define __trayicon__

#include <wx/taskbar.h>

/* This class implements Proximodo's tray icon
 */
class CTrayIcon : public wxTaskBarIcon {

public:
    CTrayIcon();
    ~CTrayIcon();
    virtual wxMenu* CreatePopupMenu();

private:
    // IDs
    enum {
        // Menu items
        ID_LOG = 1200,
        ID_EXIT,
        ID_USEPROXY,
        ID_FILTERNONE,
        ID_FILTERALL,
        ID_FILTERTEXT,
        ID_FILTERGIF,
        ID_FILTERIN,
        ID_FILTEROUT,
        ID_CONFIG        // This value and over are for configurations
    };

    // Event processing functions
    void OnCommand(wxCommandEvent& event);
    void OnClick(wxTaskBarIconEvent& event);

    // For event management
    DECLARE_EVENT_TABLE()
};

#endif
