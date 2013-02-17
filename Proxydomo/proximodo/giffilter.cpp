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


#include "giffilter.h"
#include <sstream>

using namespace std;

/* Contructor
 */
CGifFilter::CGifFilter(CDataReceptor* out) : outReceptor(out) {

    doneFirstImage = false;
}


/* Reset processing state
 */
void CGifFilter::dataReset() {

    buffer.clear();
    doneFirstImage = false;
    outReceptor->dataReset();
}


/* process buffer to the end (end of data stream)
 */
void CGifFilter::dataDump() {

    outReceptor->dataFeed(buffer);
    outReceptor->dataDump();
    buffer.clear();
}


/* Receive new data
 */
void CGifFilter::dataFeed(const string& data) {

    buffer += data;
    stringstream out;
    
    while (true) {
        size_t size = buffer.size();
        
        // Header
        if (size >= 6 && ( buffer.substr(0, 6) == "GIF87a" ||
                           buffer.substr(0, 6) == "GIF89a" ) ) {

            if (size < 13) break;
            size_t blocksize = ((unsigned char)(buffer[10]) & 0x01 ? 781 : 13);
            if (size < blocksize) break;
            out << buffer.substr(0, blocksize);
            buffer.erase(0, blocksize);

        // Footer
        } else if (size >= 1 && (unsigned char)buffer[0] == 0x3B) {

            out << buffer.substr(0, 1);
            buffer.erase(0, 1);
            
        // Graphic Control Extension Block
        } else if (size >= 2 && (unsigned char)buffer[0] == 0x21
                             && (unsigned char)buffer[1] == 0xF9) {

            if (size < 8) break;
            if (!doneFirstImage) {
                out << buffer.substr(0, 8);
            }
            buffer.erase(0, 8);

        // Comment/Plain Text/Application Extension Block
        } else if (size >= 2 && (unsigned char)buffer[0] == 0x21
                             && ( (unsigned char)buffer[1] == 0xFE ||
                                  (unsigned char)buffer[1] == 0x01 ||
                                  (unsigned char)buffer[1] == 0xFF ) ) {

            size_t pos = 2;
            if ((unsigned char)buffer[1] == 0x01) pos = 15;
            if ((unsigned char)buffer[1] == 0xFF) pos = 14;
            while (pos < size && buffer[pos])
                pos += (unsigned char)buffer[pos] + 1;
            if (pos >= size) break;
            pos++;
            out << buffer.substr(0, pos);
            buffer.erase(0, pos);

        // Image Block
        } else if (size >= 1 && (unsigned char)buffer[0] == 0x2C) {

            if (size < 10) break;
            size_t pos = ((unsigned char)(buffer[10]) & 0x01 ? 779 : 11);
            while (pos < size && buffer[pos])
                pos += (unsigned char)buffer[pos] + 1;
            if (pos >= size) break;
            pos++;
            if (!doneFirstImage) {
                out << buffer.substr(0, pos);
                doneFirstImage = true;
            }
            buffer.erase(0, pos);

        // No more data to read
        } else {
            break;
        }
    }
    
    outReceptor->dataFeed(out.str());
}

