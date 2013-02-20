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


#ifndef __zlibbuffer__
#define __zlibbuffer__

#define ZLIB_WINAPI
#include <zlib.h>       // (zlib 1.2.5)
#include <string>
#include <sstream>

using namespace std;

/* This class is a simple container for a zlib stream. One can feed it
 * with compressed (resp. uncompressed) data then immediately read
 * uncompressed (resp. compressed) data.
 * The same object can be used for compression/decompression, depending
 * on the value of 'shrink' when calling the reset() method.
 * Inflating automatically detects gzip/deflate input.
 * Deflating will generate gzip/deflate formatted data depending on 'modeGzip'.
 */
class CZlibBuffer {

public:
    CZlibBuffer();
    ~CZlibBuffer();
    bool reset(bool shrink, bool modeGzip = false);
    void feed(string data);
    void dump();
    void read(string& data);
    void free();

private:
    bool modeGzip;
    bool shrink;
    bool freed;
    int  err;
    string buffer;
    z_stream stream;
    stringstream output;
};

#endif

