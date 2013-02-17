//------------------------------------------------------------------
//
//this file is part of Proximodo
//Copyright (C) 2004 Antony BOUCHER ( kuruden@users.sourceforge.net )
//              2005 Paul Rupe      ( prupe@users.sourceforge.net )
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


#ifndef __testframe__
#define __testframe__

#include <wx/frame.h>
class CFilterDescriptor;
class pmTextCtrl;

/* This is a test window, for testing a filter upon some sample html code.
 */
class CTestFrame : public wxFrame {

public:
    CTestFrame(CFilterDescriptor* desc);
    ~CTestFrame();
    void setCurrent(CFilterDescriptor* desc);

private:
    // Event functions
    void OnClose(wxCloseEvent& event);
    void OnCommand(wxCommandEvent& event);

    // Filter to test
    CFilterDescriptor* current;

    // Controls
    pmTextCtrl *resultMemo;
    pmTextCtrl *testMemo;

    // Saved window position
    static int savedX, savedY, savedW, savedH;

    // IDs
    enum {
        // Buttons
        ID_TEST = 1500,
        ID_PROFILE,
        ID_TESTTEXT
    };

    // For event management
    DECLARE_EVENT_TABLE()
};

#endif
