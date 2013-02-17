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


#ifndef __configscreen__
#define __configscreen__

#include "windowcontent.h"
#include "descriptor.h"
#include <wx/treectrl.h>
#include <string>
#include <map>
#include <set>
#include <vector>
class pmComboBox;
class pmTreeCtrl;
class pmTextCtrl;
class CEditScreen;

using namespace std;

/* This class creates a box sizer to be set as the main frame sizer.
 * It display the filter bank list and an editable configuration.
 */
class CConfigScreen : public CWindowContent {

public:
    CConfigScreen(wxFrame* frame);
    ~CConfigScreen();

private:
    // Variable management functions
    void revert(bool confirm);
    void apply (bool confirm);
    bool hasChanged();
    void checkSelection();
    void refreshEditWindow();
    void simplifySelection();
    string exportSelection();
    wxTreeItemId getSelectionId();
    int  populate        (wxTreeItemId id);
    void showStates      (wxTreeItemId id);
    void showParentStates(wxTreeItemId id);
    void unselectFolder  (wxTreeItemId id);
    void setStates       (wxTreeItemId id, int state);
    void deleteItem      (wxTreeItemId id, bool update);
    void exportItem      (wxTreeItemId id, int root, stringstream& out);
    void deleteSelection(bool update = true);
    string makeNewName(string oldname, string newname);
    void importFilters(const string& text, bool proxo = false);

    // Managed variables
    map<string, set<int> > configs;
    map<int, CFilterDescriptor> filters;
    map<int, CFolder> folders;

    // GUI variables
    string newActiveConfig;
    string editedConfigName;
    wxTreeItemId rootId;
    wxTreeItemId currentId;
    wxTreeItemId selectionId;
    wxArrayTreeItemIds dragNDrop;

    // Non-modal edit window
    CEditScreen* editWindow;
    CFilterDescriptor* blank;

    // Controls
	pmComboBox *nameEdit;
	pmTreeCtrl *tree;
	pmTextCtrl *commentText;
	wxMenu     *menuConfig;

    // Event handling functions
    void OnCommand(wxCommandEvent& event);
    void OnTreeEvent(wxTreeEvent& event);

    // IDs
    enum {
        // Controls' ID
        ID_COMMENTTEXT = 1600,
        ID_TREE,
        ID_NAMEEDIT,
        // Menu's ID
        ID_CONFIGNEW,
        ID_CONFIGDUPLICATE,
        ID_CONFIGDELETE,
        ID_CONFIGREVERT,
        ID_CONFIGAPPLY,
        ID_FOLDERSNEW,
        ID_FILTERSNEW,
        ID_FILTERSDELETE,
        ID_FILTERSEDIT,
        ID_FILTERSCHECK,
        ID_FILTERSEXPORT,
        ID_FILTERSIMPORT,
        ID_FILTERSPROXOMITRON,
        ID_FILTERSENCODE,
        ID_FILTERSDECODE,
        // Buttons' ID
        ID_ADDFOLDBUTTON,
        ID_ADDFILTBUTTON,
        ID_TRASHBUTTON,
        ID_EDITBUTTON,
        ID_REVERTBUTTON,
        ID_APPLYBUTTON
    };

    // Event table
    DECLARE_EVENT_TABLE()
};

#endif

