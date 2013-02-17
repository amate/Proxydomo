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


#include "platform.h"

#if defined(__WINDOWS__) || defined(__WIN32__)

#include <windows.h>

/* Indicates if a virtual key currently pressed.
 */
bool CPlatform::isKeyPressed(wxKeyCode keyCode) {

    int key = keyCode;
    if (key == WXK_CONTROL) key = VK_CONTROL;
    else if (key == WXK_SHIFT) key = VK_SHIFT;
    else if (key == WXK_MENU) key = VK_MENU;
    else if (key == WXK_TAB) key = VK_TAB;
    else if (key >= WXK_F1 && key <= WXK_F12) key += VK_F1 - WXK_F1;
    return ((GetAsyncKeyState(key) & 0x8000) != 0);
}

#else // Unknown platform

bool CPlatform::isKeyPressed(wxKeyCode keyCode) {

    return false;
}

#endif
