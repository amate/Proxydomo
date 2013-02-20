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

#include "zlibbuffer.h"
//#include "const.h"


#define ZLIB_BLOCK 8192

#pragma comment(lib, "zlibstat.lib")

using namespace std;

/* Constructor
 */
CZlibBuffer::CZlibBuffer()
{
    freed = true;
}


/* Destructor
 */
CZlibBuffer::~CZlibBuffer() {
    free();
}


/* Free zlib stream's internal state
 */
void CZlibBuffer::free() {

    if (!freed) {
        if (shrink) {
            deflateEnd(&stream);
        } else {
            inflateEnd(&stream);
        }
        freed = true;
    }
}


/* Reset zlib stream and make it a deflating/inflating stream
 */
bool CZlibBuffer::reset(bool shrink, bool modeGzip) {
	
    free();
    this->shrink = shrink;
    this->modeGzip = modeGzip;
    buffer.clear();
    output.str("");
	SecureZeroMemory(&stream, sizeof(stream));
    stream.zalloc = Z_NULL;
    stream.zfree  = Z_NULL;
    stream.opaque = Z_NULL;
    if (shrink) {
        if (modeGzip) {
            err = deflateInit2(&stream, Z_BEST_SPEED, Z_DEFLATED,
                               31, 8, Z_DEFAULT_STRATEGY);
        } else {
            err = deflateInit(&stream, Z_BEST_SPEED);
        }
    } else {
		if (modeGzip) {
			err = inflateInit2(&stream, 47);
		} else {
			err = inflateInit(&stream);
		}
    }
    if (err != Z_OK) return false;
    freed = false;
    return true;
}


/* Provide data to the container.
 * As much data as possible is immediately processed.
 */
void CZlibBuffer::feed(string data) {

    if (err != Z_OK || freed) 
		return;

    buffer += data;
    size_t size = buffer.size();
    size_t remaining = size;
    char buf1[ZLIB_BLOCK], buf2[ZLIB_BLOCK];
	while ((err == Z_OK || err == Z_BUF_ERROR) && remaining > 0) {
        stream.next_in = (Byte*)buf1;
        stream.avail_in = (remaining > ZLIB_BLOCK ? ZLIB_BLOCK : remaining);
		memcpy_s(buf1, ZLIB_BLOCK, &buffer[size - remaining], stream.avail_in);
        //for (size_t i=0; i<stream.avail_in; i++) {
        //    buf1[i] = buffer[size - remaining + i];
        //}
        do {
            stream.next_out = (Byte*)buf2;
            stream.avail_out = (uInt)ZLIB_BLOCK;
            if (shrink) {
                err = deflate(&stream, Z_NO_FLUSH);
            } else {
                err = inflate(&stream, Z_NO_FLUSH);
            }
            output << string(buf2, ZLIB_BLOCK - stream.avail_out);
        } while (err == Z_OK && stream.avail_out < ZLIB_BLOCK / 10);
        if (stream.next_in == (Byte*)buf1) 
			break;
        remaining -= ((char*)stream.next_in - buf1);
    }
    buffer = buffer.substr(size - remaining);
}


/* Tells the zlib stream that there will be no more input, and ask it
 * for the remaining output bytes.
 */
void CZlibBuffer::dump() {

    if (shrink) {
        size_t size = buffer.size();
        char* buf1 = new char[size];
        char buf2[ZLIB_BLOCK];
        stream.next_in = (Byte*)buf1;
        stream.avail_in = size;
        for (size_t i=0; i<size; i++) buf1[i] = buffer[i];
        do {
            stream.next_out = (Byte*)buf2;
            stream.avail_out = (uInt)ZLIB_BLOCK;
            err = deflate(&stream, Z_FINISH);
            output << string(buf2, ZLIB_BLOCK - stream.avail_out);
        } while (err == Z_OK);
        free();
        delete[] buf1;
    }
}


/* Get processed data. It is removed from the container.
 */
void CZlibBuffer::read(string& data) {

    data = output.str();
    output.str("");
}

