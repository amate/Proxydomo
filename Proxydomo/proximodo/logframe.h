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


#ifndef __logscreen__
#define __logscreen__

#include <wx/frame.h>
class pmButton;
class pmCheckBox;
class pmTextCtrl;
class CHttpEvent;
class CProxyEvent;
class CFilterEvent;
class CSettings;

/* This class creates a box sizer to be set as the main frame sizer.
 * It display the filter bank list and an editable configuration.
 */
class CLogFrame : public wxFrame {

public:
    CLogFrame();
    ~CLogFrame();

private:
    // GUI variables
    bool active;
    
    // reference to settings
    CSettings& settings;
    
    // Controls
	pmButton *startButton;
	pmCheckBox *filterCheckbox;
	pmCheckBox *httpCheckbox;
	pmCheckBox *postCheckbox;
	pmCheckBox *proxyCheckbox;
	pmCheckBox *logCheckbox;
	pmCheckBox *browserCheckbox;
	pmCheckBox *headersCheckbox;
    pmTextCtrl *logText;

    // Saved window position
    static int savedX, savedY, savedW, savedH, savedOpt;

    // Event handling function
    void OnCommand(wxCommandEvent& event);
    void OnHttpEvent(CHttpEvent& evt);
    void OnProxyEvent(CProxyEvent& evt);
    void OnFilterEvent(CFilterEvent& evt);

    // IDs
    enum {
        // Controls' ID
        ID_CLEARBUTTON = 1700,
        ID_STARTBUTTON
    };

    // Event table
    DECLARE_EVENT_TABLE()
};

#endif

