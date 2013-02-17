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


#ifndef __const__
#define __const__

// This file only contains #define constants

// Application name
#define APP_NAME "Proximodo"

// Proximodo version
#define APP_VERSION "0.2.5"

// Constant used for "+infinity"
#define BIG_NUMBER 0x7FFFFFFF

// Size of text buffer over which CTextFilter will wait for window size
#define CTF_THRESHOLD1 8
// Size of text buffer for CTextFilter to start and process it
#define CTF_THRESHOLD2 64

// Size of socket read buffer
#define CRM_READSIZE 2048

// File names for loading settings
#define FILE_SETTINGS "settings.txt"

// File names for loading settings
#define FILE_FILTERS "filters.txt"

// Default language
#define DEFAULT_LANGUAGE "english"

// Zlib stream buffer (this is the size of chunks provided to deflate,
// not compression window size)
#define ZLIB_BLOCK 4096

// Line terminations
#define CRLF "\r\n"
#define CR   '\r'
#define LF   '\n'

// Maximum size of virtual lists
#define VIRTUAL_LIST_MAXSIZE  1000

// Default socket expiration delay (in seconds)
#define SOCKET_EXPIRATION   10

#endif
