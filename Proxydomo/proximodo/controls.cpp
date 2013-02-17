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


#include "util.h"
#include "controls.h"
#include "events.h"

using namespace std;

/* Custom text control
 */
BEGIN_EVENT_TABLE(pmTextCtrl, wxTextCtrl)
    EVT_KILL_FOCUS(pmTextCtrl::OnKillFocus)
    EVT_ENTER_WINDOW(pmTextCtrl::OnMouseEvent)
    EVT_LEAVE_WINDOW(pmTextCtrl::OnMouseEvent)
END_EVENT_TABLE()

pmTextCtrl::pmTextCtrl( wxWindow* parent, wxWindowID id, const wxString& value,
                        const wxPoint& pos, const wxSize& size, long style) :
        wxTextCtrl(parent, id, value, pos, size,
                   (style & wxTE_MULTILINE ? style : style | wxTE_PROCESS_ENTER)) {
}

void pmTextCtrl::OnKillFocus(wxFocusEvent& event) {
    wxCommandEvent cmd(wxEVT_COMMAND_TEXT_ENTER, event.GetId());
    GetEventHandler()->ProcessEvent(cmd);
    event.Skip();
}

void pmTextCtrl::OnMouseEvent(wxMouseEvent& event) {

    event.Skip();   // (let the base class receive the event too)
    if (helpText.empty()) return;
    CStatusEvent ev( event.Entering() ? W2S(helpText) : string("") );
    ProcessEvent(ev);
}


/* Custom combo box
 */
BEGIN_EVENT_TABLE(pmComboBox, wxComboBox)
    EVT_KILL_FOCUS(pmComboBox::OnKillFocus)
    EVT_ENTER_WINDOW(pmComboBox::OnMouseEvent)
    EVT_LEAVE_WINDOW(pmComboBox::OnMouseEvent)
    EVT_ERASE_BACKGROUND(pmComboBox::OnEraseEvent)
END_EVENT_TABLE()

pmComboBox::pmComboBox( wxWindow* parent, wxWindowID id, const wxString& value,
                        const wxPoint& pos, const wxSize& size,
                        const wxArrayString& choices, long style) :
        wxComboBox(parent, id, value, pos, size, choices, style | wxTE_PROCESS_ENTER) {
}

void pmComboBox::OnKillFocus(wxFocusEvent& event) {
    wxCommandEvent cmd(wxEVT_COMMAND_TEXT_ENTER, event.GetId());
    GetEventHandler()->ProcessEvent(cmd);
    event.Skip();
}

void pmComboBox::OnMouseEvent(wxMouseEvent& event) {

    event.Skip();   // (let the base class receive the event too)
    if (helpText.empty()) return;
    CStatusEvent ev( event.Entering() ? W2S(helpText) : string("") );
    ProcessEvent(ev);
}


/* Custom static box sizer
 */
pmStaticBoxSizer::pmStaticBoxSizer(wxStaticBox* box, int orient) :
            wxStaticBoxSizer(box,orient) {
}

pmStaticBoxSizer::~pmStaticBoxSizer() {
    wxStaticBox* box = GetStaticBox();
    wxWindow* window = box->GetParent();
    window->RemoveChild(box);
    box->Show(false);
    delete box;
}


/* Custom list control
 */
BEGIN_EVENT_TABLE(pmListCtrl, wxListCtrl)
    EVT_ENTER_WINDOW(pmListCtrl::OnMouseEvent)
    EVT_LEAVE_WINDOW(pmListCtrl::OnMouseEvent)
END_EVENT_TABLE()

pmListCtrl::pmListCtrl( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                        const wxSize& size, long style ) :
        wxListCtrl(parent, id, pos, size, style) {
}

void pmListCtrl::OnMouseEvent(wxMouseEvent& event) {

    event.Skip();   // (let the base class receive the event too)
    if (helpText.empty()) return;
    CStatusEvent ev( event.Entering() ? W2S(helpText) : string("") );
    ProcessEvent(ev);
}


/* Custom checkbox control
 */
BEGIN_EVENT_TABLE(pmCheckBox, wxCheckBox)
    EVT_ENTER_WINDOW(pmCheckBox::OnMouseEvent)
    EVT_LEAVE_WINDOW(pmCheckBox::OnMouseEvent)
    EVT_ERASE_BACKGROUND(pmCheckBox::OnEraseEvent)
END_EVENT_TABLE()

pmCheckBox::pmCheckBox( wxWindow* parent, wxWindowID id, const wxString& label,
                        const wxPoint& pos, const wxSize& size, long style ) :
        wxCheckBox(parent, id, label, pos, size, style) {
}

void pmCheckBox::OnMouseEvent(wxMouseEvent& event) {

    event.Skip();   // (let the base class receive the event too)
    if (helpText.empty()) return;
    CStatusEvent ev( event.Entering() ? W2S(helpText) : string("") );
    ProcessEvent(ev);
}


/* Custom button control
 */
BEGIN_EVENT_TABLE(pmButton, wxButton)
    EVT_ENTER_WINDOW(pmButton::OnMouseEvent)
    EVT_LEAVE_WINDOW(pmButton::OnMouseEvent)
END_EVENT_TABLE()

pmButton::pmButton( wxWindow* parent, wxWindowID id, const wxString& label,
                    const wxPoint& pos, const wxSize& size, long style ) :
        wxButton(parent, id, label, pos, size, style) {
}

void pmButton::OnMouseEvent(wxMouseEvent& event) {

    event.Skip();   // (let the base class receive the event too)
    if (helpText.empty()) return;
    CStatusEvent ev( event.Entering() ? W2S(helpText) : string("") );
    ProcessEvent(ev);
}


/* Custom bitmap button control
 */
BEGIN_EVENT_TABLE(pmBitmapButton, wxBitmapButton)
    EVT_ENTER_WINDOW(pmBitmapButton::OnMouseEvent)
    EVT_LEAVE_WINDOW(pmBitmapButton::OnMouseEvent)
    EVT_ERASE_BACKGROUND(pmBitmapButton::OnEraseEvent)
END_EVENT_TABLE()

pmBitmapButton::pmBitmapButton( wxWindow* parent, wxWindowID id,
                                const wxBitmap& bitmap, const wxPoint& pos,
                                const wxSize& size, long style ) :
        wxBitmapButton(parent, id, bitmap, pos, size, style) {
}

void pmBitmapButton::OnMouseEvent(wxMouseEvent& event) {

    event.Skip();   // (let the base class receive the event too)
    if (helpText.empty()) return;
    CStatusEvent ev( event.Entering() ? W2S(helpText) : string("") );
    ProcessEvent(ev);
}


/* Custom static text control
 */
BEGIN_EVENT_TABLE(pmStaticText, wxStaticText)
    EVT_ERASE_BACKGROUND(pmStaticText::OnEraseEvent)
END_EVENT_TABLE()

pmStaticText::pmStaticText( wxWindow* parent, wxWindowID id,
                            const wxString& label, const wxPoint& pos,
                            const wxSize& size, long style ) :
        wxStaticText(parent, id, label, pos, size, style) {
}


/* Custom tree control
 */
BEGIN_EVENT_TABLE(pmTreeCtrl, wxTreeCtrl)
    EVT_ENTER_WINDOW(pmTreeCtrl::OnMouseEvent)
    EVT_LEAVE_WINDOW(pmTreeCtrl::OnMouseEvent)
    EVT_LEFT_DOWN(pmTreeCtrl::OnMouseEvent)
END_EVENT_TABLE()

pmTreeCtrl::pmTreeCtrl( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                        const wxSize& size, long style ) :
        wxTreeCtrl(parent, id, pos, size, style) {
}

void pmTreeCtrl::OnMouseEvent(wxMouseEvent& event) {

    event.Skip();   // (let the base class receive the event too)
    if (event.GetEventType() == wxEVT_LEFT_DOWN) {
        int flags;
        wxTreeItemId id = HitTest(event.GetPosition(), flags);
        if (id.IsOk() && (flags & wxTREE_HITTEST_ONITEMICON)) {
            wxTreeEvent ev(wxEVT_COMMAND_TREE_STATE_IMAGE_CLICK, GetId());
            ev.SetItem(id);
            ProcessEvent(ev);
        }
    } else {
        if (helpText.empty()) return;
        CStatusEvent ev( event.Entering() ? W2S(helpText) : string("") );
        ProcessEvent(ev);
    }
}

