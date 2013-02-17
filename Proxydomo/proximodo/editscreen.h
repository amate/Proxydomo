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


#ifndef __editscreen__
#define __editscreen__

#include <wx/frame.h>
#include <string>
#include <map>
class pmCheckBox;
class pmComboBox;
class pmStaticText;
class pmTextCtrl;
class pmListCtrl;
class CTestFrame;
class CFilterDescriptor;

using namespace std;

/* This is the non-modal filter edition window.
 */
class CEditScreen : public wxFrame {

public:
    CEditScreen(CFilterDescriptor* desc);
    ~CEditScreen();
    void setCurrent(CFilterDescriptor* desc);
    
private:
    // Edited filter
    CFilterDescriptor* current;
    
    void enableFields();
    void OnClose(wxCloseEvent& event);
    CTestFrame* testWindow;

    // Controls
    pmCheckBox *multiCheckBox;
    pmComboBox *typeComboBox;
    pmStaticText *boundsLabel;
    pmStaticText *headerLabel;
    pmStaticText *widthLabel;
    pmTextCtrl *authorEdit;
    pmTextCtrl *boundsEdit;
    pmTextCtrl *commentEdit;
    pmTextCtrl *headerEdit;
    pmTextCtrl *titleEdit;
    pmTextCtrl *urlEdit;
    pmTextCtrl *versionEdit;
    pmTextCtrl *widthEdit;
    pmTextCtrl *priorityEdit;
    pmTextCtrl *matchMemo;
    pmTextCtrl *replaceMemo;
    pmListCtrl *listCtrl;

    // Saved window position
    static int savedX, savedY, savedW, savedH;

    // Event handling function
    void OnCommand(wxCommandEvent& event);

    // IDs
    enum {
        // Controls' ID
        ID_TITLEEDIT = 1400,
        ID_AUTHOREDIT,
        ID_COMMENTEDIT,
        ID_VERSIONEDIT,
        ID_PRIORITYEDIT,
        ID_URLEDIT,
        ID_WIDTHEDIT,
        ID_BOUNDSEDIT,
        ID_HEADEREDIT,
        ID_MATCHMEMO,
        ID_REPLACEMEMO,
        ID_MULTICHECKBOX,
        ID_TYPECOMBOBOX,
        ID_LISTCTRL,
        // Menu items
        ID_FILTERSENCODE,
        ID_FILTERSDECODE,
        ID_FILTERSTEST,
        ID_HELPFILTERS,
        ID_HELPSYNTAX
    };

    // Event table
    DECLARE_EVENT_TABLE()
};

DECLARE_EVENT_TYPE(wxEVT_PM_RAISE_EVENT, -1)

#endif

