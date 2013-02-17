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


#ifndef __windowcontent__
#define __windowcontent__

#include <wx/sizer.h>
#include <wx/event.h>
class CSettings;

/* This class creates a box sizer to be set as the main frame sizer.
 * It is meant to be inherited by all screens that can display within the
 * main frame.
 */
class CWindowContent : public wxEvtHandler, public wxBoxSizer {

public:
    virtual void revert(bool confirm) { }
    virtual void apply(bool confirm) { }
    virtual bool hasChanged() { return false; }
    virtual ~CWindowContent();

protected:
    CWindowContent(wxFrame* frame, int orient = wxHORIZONTAL);
    void makeSizer();
    void commitText();

    wxFrame* frame;          // The parent frame
    CSettings& settings;
};

#endif

