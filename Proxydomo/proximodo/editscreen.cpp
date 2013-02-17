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


#include "editscreen.h"
#include "matcher.h"
#include "settings.h"
#include "const.h"
#include "util.h"
#include "controls.h"
#include "testframe.h"
#include "images/icon32.xpm"
#include <wx/msgdlg.h>
#include <wx/icon.h>
#include <wx/sysopt.h>
#include <wx/settings.h>
#include <wx/menu.h>
#include <set>
#include <list>
#include <sstream>
#include <algorithm>

using namespace std;

/* Events
 */
BEGIN_EVENT_TABLE(CEditScreen, wxFrame)
    EVT_CLOSE      (CEditScreen::OnClose)
    EVT_CHECKBOX   (ID_MULTICHECKBOX, CEditScreen::OnCommand)
    EVT_COMBOBOX   (ID_TYPECOMBOBOX,  CEditScreen::OnCommand)
    EVT_TEXT_ENTER (ID_TITLEEDIT,     CEditScreen::OnCommand)
    EVT_TEXT_ENTER (ID_AUTHOREDIT,    CEditScreen::OnCommand)
    EVT_TEXT_ENTER (ID_COMMENTEDIT,   CEditScreen::OnCommand)
    EVT_TEXT_ENTER (ID_VERSIONEDIT,   CEditScreen::OnCommand)
    EVT_TEXT_ENTER (ID_URLEDIT,       CEditScreen::OnCommand)
    EVT_TEXT_ENTER (ID_WIDTHEDIT,     CEditScreen::OnCommand)
    EVT_TEXT_ENTER (ID_PRIORITYEDIT,  CEditScreen::OnCommand)
    EVT_TEXT_ENTER (ID_BOUNDSEDIT,    CEditScreen::OnCommand)
    EVT_TEXT_ENTER (ID_HEADEREDIT,    CEditScreen::OnCommand)
    EVT_TEXT_ENTER (ID_MATCHMEMO,     CEditScreen::OnCommand)
    EVT_TEXT_ENTER (ID_REPLACEMEMO,   CEditScreen::OnCommand)
    EVT_MENU_RANGE (ID_FILTERSENCODE, ID_HELPSYNTAX, CEditScreen::OnCommand)
    EVT_COMMAND    (wxID_ANY,  wxEVT_PM_RAISE_EVENT, CEditScreen::OnCommand)
END_EVENT_TABLE()

DEFINE_EVENT_TYPE(wxEVT_PM_RAISE_EVENT)


/* Saved window position
 */
int CEditScreen::savedX = BIG_NUMBER,
    CEditScreen::savedY = 0,
    CEditScreen::savedW = 0,
    CEditScreen::savedH = 0;


/* Constructor
 */
CEditScreen::CEditScreen(CFilterDescriptor* desc) :
        wxFrame((wxFrame *)NULL, wxID_ANY,
                S2W(CSettings::ref().getMessage("EDIT_WINDOW_TITLE")),
                wxDefaultPosition, wxDefaultSize,
                wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL |
                wxCLIP_CHILDREN) {

    SetIcon(wxIcon(icon32_xpm));
    CSettings& settings = CSettings::ref();
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    // Controls creation
    wxBoxSizer* editSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(editSizer);

    wxFlexGridSizer* descSizer = new wxFlexGridSizer(5,2,5,5);
    descSizer->AddGrowableCol(1);
    editSizer->Add(descSizer,0,wxGROW | wxALL,5);

    pmStaticText* titleLabel =  new pmStaticText(this, wxID_ANY ,
        S2W(settings.getMessage("LB_EDIT_TITLE"))  );
    descSizer->Add(titleLabel,0,wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,0);

    titleEdit =  new pmTextCtrl(this, ID_TITLEEDIT, wxT("") ,
        wxDefaultPosition, wxDefaultSize);
    titleEdit->SetHelpText(S2W(settings.getMessage("EDIT_TITLE_TIP")));
    descSizer->Add(titleEdit,1,wxGROW | wxALIGN_CENTER_VERTICAL | wxALL,0);

    pmStaticText* authorLabel =  new pmStaticText(this, wxID_ANY ,
        S2W(settings.getMessage("LB_EDIT_AUTHOR"))  );
    descSizer->Add(authorLabel,0,wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,0);

    wxBoxSizer* authorSizer = new wxBoxSizer(wxHORIZONTAL);
    descSizer->Add(authorSizer,0,wxGROW | wxALL,0);

    authorEdit =  new pmTextCtrl(this, ID_AUTHOREDIT, wxT("") ,
        wxDefaultPosition, wxDefaultSize );
    authorEdit->SetHelpText(S2W(settings.getMessage("EDIT_AUTHOR_TIP")));
    authorSizer->Add(authorEdit,1,wxGROW | wxALIGN_CENTER_VERTICAL | wxALL,0);

    pmStaticText* versionLabel =  new pmStaticText(this, wxID_ANY ,
        S2W(settings.getMessage("LB_EDIT_VERSION"))  );
    authorSizer->Add(versionLabel,0,wxALIGN_CENTER_VERTICAL | wxLEFT,10);

    versionEdit =  new pmTextCtrl(this, ID_VERSIONEDIT, wxT("") ,
        wxDefaultPosition, wxSize(60,21) );
    versionEdit->SetHelpText(S2W(settings.getMessage("EDIT_VERSION_TIP")));
    authorSizer->Add(versionEdit,0,wxALIGN_CENTER_VERTICAL | wxLEFT,5);

    pmStaticText* commentLabel =  new pmStaticText(this, wxID_ANY ,
        S2W(settings.getMessage("LB_EDIT_COMMENT"))  );
    descSizer->Add(commentLabel,0,wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,0);

    commentEdit =  new pmTextCtrl(this, ID_COMMENTEDIT, wxT("") ,
        wxDefaultPosition, wxSize(50,45), wxTE_MULTILINE );
    commentEdit->SetHelpText(S2W(settings.getMessage("EDIT_COMMENT_TIP")));
    descSizer->Add(commentEdit,1,wxGROW | wxALIGN_CENTER_VERTICAL | wxALL,0);

    pmStaticText* priorityLabel =  new pmStaticText(this, wxID_ANY ,
        S2W(settings.getMessage("LB_EDIT_PRIORITY"))  );
    descSizer->Add(priorityLabel,0,wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,0);

    wxBoxSizer* prioritySizer = new wxBoxSizer(wxHORIZONTAL);
    descSizer->Add(prioritySizer,0,wxGROW | wxALL,0);

    priorityEdit =  new pmTextCtrl(this, ID_PRIORITYEDIT, wxT("") ,
        wxDefaultPosition, wxSize(40,21) );
    priorityEdit->SetHelpText(S2W(settings.getMessage("EDIT_PRIORITY_TIP")));
    prioritySizer->Add(priorityEdit,0,wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,0);

    widthLabel =  new pmStaticText(this, wxID_ANY ,
        S2W(settings.getMessage("LB_EDIT_WIDTH"))  );
    prioritySizer->Add(widthLabel,0,wxALIGN_CENTER_VERTICAL | wxLEFT,10);

    widthEdit =  new pmTextCtrl(this, ID_WIDTHEDIT, wxT("") ,
        wxDefaultPosition, wxSize(40,21) );
    widthEdit->SetHelpText(S2W(settings.getMessage("EDIT_WIDTH_TIP")));
    prioritySizer->Add(widthEdit,0,wxALIGN_CENTER_VERTICAL | wxLEFT,5);

    multiCheckBox =  new pmCheckBox(this, ID_MULTICHECKBOX,
        S2W(settings.getMessage("LB_EDIT_MULTIMATCH")),
        wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT  );
    multiCheckBox->SetHelpText(S2W(settings.getMessage("EDIT_MULTIMATCH_TIP")));
    prioritySizer->Add(multiCheckBox,0,wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT,10);

    pmStaticText* typeLabel =  new pmStaticText(this, wxID_ANY ,
        S2W(settings.getMessage("LB_EDIT_TYPE"))  );
    descSizer->Add(typeLabel,0,wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,0);

    wxBoxSizer* typeSizer = new wxBoxSizer(wxHORIZONTAL);
    descSizer->Add(typeSizer,0,wxGROW | wxALL,0);

    wxArrayString choices;
    typeComboBox =  new pmComboBox(this, ID_TYPECOMBOBOX , wxT("") ,
        wxDefaultPosition, wxSize(120,21),
        choices, wxCB_DROPDOWN | wxCB_READONLY );
    typeComboBox->SetHelpText(S2W(settings.getMessage("EDIT_TYPE_TIP")));
    typeComboBox->Append(S2W(settings.getMessage("LB_EDIT_TYPEOUT")));
    typeComboBox->Append(S2W(settings.getMessage("LB_EDIT_TYPEIN")));
    typeComboBox->Append(S2W(settings.getMessage("LB_EDIT_TYPETEXT")));
    typeSizer->Add(typeComboBox,0,wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,0);

    headerLabel =  new pmStaticText(this, wxID_ANY ,
        S2W(settings.getMessage("LB_EDIT_HEADERNAME"))  );
    typeSizer->Add(headerLabel,0,wxALIGN_CENTER_VERTICAL | wxLEFT, 10);

    headerEdit =  new pmTextCtrl(this, ID_HEADEREDIT, wxT("") ,
        wxDefaultPosition, wxSize(150,21) );
    headerEdit->SetHelpText(S2W(settings.getMessage("EDIT_HEADERNAME_TIP")));
    headerEdit->SetFont(wxFont(10, wxSWISS ,wxNORMAL,wxNORMAL,FALSE,wxT("Courier")));
    typeSizer->Add(headerEdit,0,wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

    wxFlexGridSizer* boundsSizer = new wxFlexGridSizer(2,2,5,5);
    boundsSizer->AddGrowableCol(1);
    editSizer->Add(boundsSizer,0,wxGROW | wxALL,5);

    boundsLabel =  new pmStaticText(this, wxID_ANY ,
        S2W(settings.getMessage("LB_EDIT_BOUNDS"))  );
    boundsSizer->Add(boundsLabel,0,wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,0);

    boundsEdit =  new pmTextCtrl(this, ID_BOUNDSEDIT, wxT("") ,
        wxDefaultPosition, wxDefaultSize );
    boundsEdit->SetHelpText(S2W(settings.getMessage("EDIT_BOUNDS_TIP")));
    boundsEdit->SetFont(wxFont(10, wxSWISS ,wxNORMAL,wxNORMAL,FALSE,wxT("Courier")));
    boundsSizer->Add(boundsEdit,1, wxALIGN_CENTER_VERTICAL | wxGROW | wxALL,0);

    pmStaticText* urlLabel =  new pmStaticText(this, wxID_ANY ,
        S2W(settings.getMessage("LB_EDIT_URL"))  );
    boundsSizer->Add(urlLabel,0,wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,0);

    urlEdit =  new pmTextCtrl(this, ID_URLEDIT, wxT("") ,
        wxDefaultPosition, wxDefaultSize );
    urlEdit->SetHelpText(S2W(settings.getMessage("EDIT_URL_TIP")));
    urlEdit->SetFont(wxFont(10, wxSWISS ,wxNORMAL,wxNORMAL,FALSE,wxT("Courier")));
    boundsSizer->Add(urlEdit,1, wxALIGN_CENTER_VERTICAL | wxGROW | wxALL,0);

    pmStaticText* matchLabel =  new pmStaticText(this, wxID_ANY ,
        S2W(settings.getMessage("LB_EDIT_MATCH"))  );
    editSizer->Add(matchLabel,0,wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT,5);

    matchMemo =  new pmTextCtrl(this, ID_MATCHMEMO, wxT("") ,
        wxDefaultPosition, wxSize(200,55)  ,
        wxTE_MULTILINE | wxTE_DONTWRAP | wxTE_PROCESS_TAB);
    matchMemo->SetHelpText(S2W(settings.getMessage("EDIT_MATCH_TIP")));
    matchMemo->SetFont(wxFont(10, wxSWISS ,wxNORMAL,wxNORMAL,FALSE,wxT("Courier")));
    editSizer->Add(matchMemo,3,wxGROW | wxALL,5);

    pmStaticText* replaceLabel =  new pmStaticText(this, wxID_ANY ,
        S2W(settings.getMessage("LB_EDIT_REPLACE"))  );
    editSizer->Add(replaceLabel,0,wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT,5);

    replaceMemo =  new pmTextCtrl(this, ID_REPLACEMEMO, wxT("") ,
        wxDefaultPosition, wxSize(200,55)  ,
        wxTE_MULTILINE | wxTE_DONTWRAP | wxTE_PROCESS_TAB);
    replaceMemo->SetHelpText(S2W(settings.getMessage("EDIT_REPLACE_TIP")));
    replaceMemo->SetFont(wxFont(10, wxSWISS ,wxNORMAL,wxNORMAL,FALSE,wxT("Courier")));
    editSizer->Add(replaceMemo,2,wxGROW | wxALL,5);

    // Menu creation
    wxMenuBar* menubar = new wxMenuBar();
    wxMenu* menu = new wxMenu();
    menu->Append(ID_FILTERSTEST,
        S2W(settings.getMessage("MENU_FILTERSTEST")),
        S2W(settings.getMessage("MENU_FILTERSTEST_TIP")));
    menu->Append(ID_FILTERSENCODE,
        S2W(settings.getMessage("MENU_FILTERSENCODE")),
        S2W(settings.getMessage("MENU_FILTERSENCODE_TIP")));
    menu->Append(ID_FILTERSDECODE,
        S2W(settings.getMessage("MENU_FILTERSDECODE")),
        S2W(settings.getMessage("MENU_FILTERSDECODE_TIP")));
    menubar->Append(menu,
        S2W(settings.getMessage("MENU_FILTERS")));
    menu = new wxMenu();
    menu->Append(ID_HELPFILTERS,
        S2W(settings.getMessage("MENU_HELPFILTERS")),
        S2W(settings.getMessage("MENU_HELPFILTERS_TIP")));
    menu->Append(ID_HELPSYNTAX,
        S2W(settings.getMessage("MENU_HELPSYNTAX")),
        S2W(settings.getMessage("MENU_HELPSYNTAX_TIP")));
    menubar->Append(menu,
        S2W(settings.getMessage("MENU_HELP")));
    SetMenuBar(menubar);

    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);

    // Reload position and size
    if (savedX == BIG_NUMBER) {
        Centre(wxBOTH | wxCENTRE_ON_SCREEN);
    } else {
        Move(savedX, savedY);
        SetSize(savedW, savedH);
    }

    current = desc;
    testWindow = new CTestFrame(current);
}


/* Destructor
 */
CEditScreen::~CEditScreen() {

    // Record position and size
    GetPosition(&savedX, &savedY);
    GetSize(&savedW, &savedH);

    delete testWindow;
}


/* Hide the window when clicking on the [X] button
 * The window is destroyed by the config screen on its own destruction
 */
void CEditScreen::OnClose(wxCloseEvent& event) {
    testWindow->Hide();
    Hide();
}


/* Display filter properties in controls
 */
void CEditScreen::setCurrent(CFilterDescriptor* desc) {

    current = desc;
    testWindow->setCurrent(current);
    
    stringstream ss;
    ss << current->windowWidth;
    widthEdit->SetValue(SS2W(ss));
    ss.str("");
    ss << current->priority;
    priorityEdit->SetValue(SS2W(ss));
    
    multiCheckBox->SetValue(current->multipleMatches);
    authorEdit   ->SetValue(S2W(current->author));
    boundsEdit   ->SetValue(S2W(current->boundsPattern));
    commentEdit  ->SetValue(S2W(current->comment));
    headerEdit   ->SetValue(S2W(current->headerName));
    titleEdit    ->SetValue(S2W(current->title));
    urlEdit      ->SetValue(S2W(current->urlPattern));
    versionEdit  ->SetValue(S2W(current->version));
    matchMemo    ->SetValue(S2W(current->matchPattern));
    replaceMemo  ->SetValue(S2W(current->replacePattern));
    
    if (current->filterType == CFilterDescriptor::HEADOUT) {
        typeComboBox->SetValue(typeComboBox->GetString(0));
    } else if (current->filterType == CFilterDescriptor::HEADIN) {
        typeComboBox->SetValue(typeComboBox->GetString(1));
    } else {
        typeComboBox->SetValue(typeComboBox->GetString(2));
    }
    enableFields();
}


/* Enables fields wrt. filter type
 */
void CEditScreen::enableFields() {

    bool canEdit = (current->id != -1);
    wxWindowList& children = GetChildren();
    for (wxWindowListNode* child = children.GetFirst(); child; child = child->GetNext())
        child->GetData()->Enable(canEdit);
    if (!canEdit) return;

    bool isText = (current->filterType == CFilterDescriptor::TEXT);
    boundsEdit->Enable(isText);
    widthEdit->Enable(isText);
    multiCheckBox->Enable(isText);
    headerEdit->Enable(!isText);
    boundsLabel->Enable(isText);
    widthLabel->Enable(isText);
    headerLabel->Enable(!isText);
}


/* Fields events handling function
 */
void CEditScreen::OnCommand(wxCommandEvent& event) {

    if (event.GetEventType() == wxEVT_PM_RAISE_EVENT) {
        // at some point in the config screen, we want to wxT("post") a raise event.
        // wxWidgets does not have such event, so we use a custom event type.
        Raise();
        event.Skip();
        return;
    }
    
    switch (event.GetId()) {
    case ID_MULTICHECKBOX:
        {
            current->multipleMatches = multiCheckBox->GetValue();
            break;
        }
    case ID_AUTHOREDIT:
        {
            string value = W2S(authorEdit->GetValue());
            current->author = CUtil::trim(value);
            break;
        }
    case ID_VERSIONEDIT:
        {
            string value = W2S(versionEdit->GetValue());
            current->version = CUtil::trim(value);
            break;
        }
    case ID_HEADEREDIT:
        {
            string value = W2S(headerEdit->GetValue());
            current->headerName = CUtil::trim(value);
            current->testValidity();
            break;
        }
    case ID_COMMENTEDIT:
        {
            string value = W2S(commentEdit->GetValue());
            CUtil::trim(value);
            if (value != current->comment)
                current->defaultFilter = 0;
            current->comment = value;
            break;
        }
    case ID_TITLEEDIT:
        {
            string value = W2S(titleEdit->GetValue());
            CUtil::trim(value);
            if (value != current->title)
                current->defaultFilter = 0;
            current->title = value;
            current->testValidity();
            break;
        }
    case ID_WIDTHEDIT:
        {
            stringstream ss(W2S(widthEdit->GetValue()));
            int n = 0;
            ss >> n;
            if (n < 1) {
                n = 256;
                widthEdit->SetValue(wxT("256"));
            }
            current->windowWidth = n;
            current->testValidity();
            break;
        }
    case ID_PRIORITYEDIT:
        {
            stringstream ss(W2S(priorityEdit->GetValue()));
            int n = 0;
            ss >> n;
            if (n < 1) {
                n = 256;
                priorityEdit->SetValue(wxT("256"));
            }
            current->priority = n;
            break;
        }
    case ID_TYPECOMBOBOX:
        {
            int row = typeComboBox->FindString(typeComboBox->GetValue());
            if (row == 0) {
                current->filterType = CFilterDescriptor::HEADOUT;
            } else if (row == 1) {
                current->filterType = CFilterDescriptor::HEADIN;
            } else {
                current->filterType = CFilterDescriptor::TEXT;
            }
            enableFields();
            break;
        }
    case ID_URLEDIT:
        {
            string value = W2S(urlEdit->GetValue());
            if (value != current->urlPattern) {
                current->urlPattern = value;
                string errmsg;
                if (!CMatcher::testPattern(current->urlPattern, errmsg))
                    wxMessageBox(S2W(errmsg), wxT(APP_NAME));
            }
            current->testValidity();
            break;
        }
    case ID_BOUNDSEDIT:
        {
            string value = W2S(boundsEdit->GetValue());
            if (value != current->boundsPattern) {
                current->boundsPattern = value;
                string errmsg;
                if (!CMatcher::testPattern(current->boundsPattern, errmsg))
                    wxMessageBox(S2W(errmsg), wxT(APP_NAME));
            }
            current->testValidity();
            break;
        }
    case ID_MATCHMEMO:
        {
            string value = W2S(matchMemo->GetValue());
            if (value != current->matchPattern) {
                current->matchPattern = value;
                string errmsg;
                if (!CMatcher::testPattern(current->matchPattern, errmsg))
                    wxMessageBox(S2W(errmsg), wxT(APP_NAME));
            }
            current->testValidity();
            break;
        }
    case ID_REPLACEMEMO:
        {
            string value = W2S(replaceMemo->GetValue());
            current->replacePattern = value;
            break;
        }
    case ID_FILTERSENCODE:
        {
            CUtil::setClipboard(CUtil::encodeBASE64(CUtil::getClipboard()));
            break;
        }
    case ID_FILTERSDECODE:
        {
            CUtil::setClipboard(CUtil::decodeBASE64(CUtil::getClipboard()));
            break;
        }
    case ID_HELPFILTERS:
        {
            CUtil::openBrowser(CSettings::ref().getMessage("HELP_PAGE_FILTERS"));
            break;
        }
    case ID_HELPSYNTAX:
        {
            CUtil::openBrowser(CSettings::ref().getMessage("HELP_PAGE_SYNTAX"));
            break;
        }
    case ID_FILTERSTEST:
        {
            testWindow->Show();
            testWindow->Raise();
            break;
        }
    default:
        event.Skip();
    }
}

