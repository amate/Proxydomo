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


#ifndef __controls__
#define __controls__

#include <wx/combobox.h>
#include <wx/treectrl.h>
#include <wx/checkbox.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <string>

using namespace std;

/* This class adds the Kill Focus event catching functionality to text controls.
 * Wx validators are not exactly what we want, they are only invoked at dialog
 * dismissal, while we want the user to know immediately if something is wrong.
 * Default wxTextCtrl can detect a new value when the user pressed Enter,
 * but not when they change field.
 * This controls also implements event generation for displaying a help wxString
 * in parent's statusbar.
 */
class pmTextCtrl : public wxTextCtrl {

public:
    pmTextCtrl( wxWindow* parent, wxWindowID id,
                const wxString& value = wxT(""),
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize, long style = 0 );
    virtual ~pmTextCtrl() { }
    void OnKillFocus(wxFocusEvent& event);
    void OnMouseEvent(wxMouseEvent& event);
    virtual void SetHelpText(const wxString& text) { helpText = text; }
    virtual wxString GetHelpText() const { return helpText; }

private:
    wxString helpText;
    DECLARE_EVENT_TABLE()
};


/* This class adds the Kill Focus event catching functionality to combo boxes,
 * and statusbar help wxString.
 */
class pmComboBox : public wxComboBox {

public:
    pmComboBox( wxWindow* parent, wxWindowID id, const wxString& value,
                const wxPoint& pos, const wxSize& size,
                const wxArrayString& choices, long style = 0 );
    virtual ~pmComboBox() { }
    void OnKillFocus(wxFocusEvent& event);
    void OnMouseEvent(wxMouseEvent& event);
    void OnEraseEvent(wxEraseEvent& event) { }
    virtual void SetHelpText(const wxString& text) { helpText = text; }
    virtual wxString GetHelpText() const { return helpText; }

private:
    wxString helpText;
    DECLARE_EVENT_TABLE()
};


/* This class corrects a bug(?) in the wxStaticBoxSizer, which does not
 * remove the static box from the parent at destruction
 */
class pmStaticBoxSizer : public wxStaticBoxSizer {

public:
    pmStaticBoxSizer(wxStaticBox* box, int orient);
    virtual ~pmStaticBoxSizer();
};


/* This class adds the status help wxString capability to the list control.
 */
class pmListCtrl : public wxListCtrl {

public:
    pmListCtrl( wxWindow* parent, wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize, long style = wxLC_ICON );
    virtual ~pmListCtrl() { }
    void OnMouseEvent(wxMouseEvent& event);
    virtual void SetHelpText(const wxString& text) { helpText = text; }
    virtual wxString GetHelpText() const { return helpText; }

private:
    wxString helpText;
    DECLARE_EVENT_TABLE()
};


/* This class adds the status help wxString capability to the checkbox control.
 */
class pmCheckBox : public wxCheckBox {

public:
    pmCheckBox( wxWindow* parent, wxWindowID id, const wxString& label,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize, long style = 0 );
    virtual ~pmCheckBox() { }
    void OnMouseEvent(wxMouseEvent& event);
    virtual void SetHelpText(const wxString& text) { helpText = text; }
    virtual wxString GetHelpText() const { return helpText; }
    void OnEraseEvent(wxEraseEvent& event) { }

private:
    wxString helpText;
    DECLARE_EVENT_TABLE()
};


/* This class adds the status help wxString capability to the button control.
 */
class pmButton : public wxButton {

public:
    pmButton( wxWindow* parent, wxWindowID id,
              const wxString& label = wxEmptyString,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize, long style = 0 );
    virtual ~pmButton() { }
    void OnMouseEvent(wxMouseEvent& event);
    virtual void SetHelpText(const wxString& text) { helpText = text; }
    virtual wxString GetHelpText() const { return helpText; }

private:
    wxString helpText;
    DECLARE_EVENT_TABLE()
};


/* This class adds the status help wxString capability to the bitmap button control.
 */
class pmBitmapButton : public wxBitmapButton {

public:
    pmBitmapButton( wxWindow* parent, wxWindowID id, const wxBitmap& bitmap,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize,
                    long style = wxBU_AUTODRAW );
    virtual ~pmBitmapButton() { }
    void OnMouseEvent(wxMouseEvent& event);
    virtual void SetHelpText(const wxString& text) { helpText = text; }
    virtual wxString GetHelpText() const { return helpText; }
    void OnEraseEvent(wxEraseEvent& event) { }

private:
    wxString helpText;
    DECLARE_EVENT_TABLE()
};


/* This class supersedes the static text, to prevent painting background.
 */
class pmStaticText : public wxStaticText {

public:
    pmStaticText( wxWindow* parent, wxWindowID id, const wxString& label,
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize, long style = 0 );
    virtual ~pmStaticText() { }
    void OnEraseEvent(wxEraseEvent& event) { }

private:
    DECLARE_EVENT_TABLE()
};


/* This class adds status help and icon click detection to the tree control
 */
class pmTreeCtrl : public wxTreeCtrl {

public:
    pmTreeCtrl( wxWindow* parent, wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxTR_HAS_BUTTONS );
    virtual ~pmTreeCtrl() { }
    void OnMouseEvent(wxMouseEvent& event);
    virtual void SetHelpText(const wxString& text) { helpText = text; }
    virtual wxString GetHelpText() const { return helpText; }

private:
    wxString helpText;
    DECLARE_EVENT_TABLE()
};

#endif
