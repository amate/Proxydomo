//------------------------------------------------------------------
//
//this file is part of Proximodo
//Copyright (C) 2004-2005 Antony BOUCHER ( kuruden@users.sourceforge.net )
//                        Paul Rupe      ( prupe@users.sourceforge.net )
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


#ifndef __platform__
#define __platform__

#include <wx/event.h>

/* This file contains platform-specific functions.
 * Theoretically, to port Proximodo on a platform where wxWidgets and
 * C++ STL support are available, only this file would have to
 * be adapted.
 */

class CPlatform {

public:
    static bool isKeyPressed(wxKeyCode keyCode);
};

#if defined(__WINDOWS__) || defined(__WIN32__)
#define   PLATFORM_SERVERFLAGS   (wxSOCKET_NONE)
#define   PLATFORM_CLOSE_ON_MINIMIZE
#else
#define   PLATFORM_SERVERFLAGS   (wxSOCKET_REUSEADDR)
#endif

#endif
// vi:ts=4:sw=4:et
