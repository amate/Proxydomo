//------------------------------------------------------------------
//
//this file is part of Proximodo
//Copyright (C) 2004-2005 Antony BOUCHER ( kuruden@users.sourceforge.net )
//                        Paul Rupe ( prupe@users.sourceforge.net )
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

#include "textbuffer.h"
#include "textfilter.h"
#include "matcher.h"
#include "filterowner.h"
#include "util.h"
#include "settings.h"
#include "log.h"
#include "descriptor.h"
#include <sstream>
#include <algorithm>

using namespace std;

CTextBuffer::CTextBuffer(CFilterOwner& owner, CDataReceptor* output) :
                                        owner(owner), output(output) {

    // Retrieve filter descriptions to embed
    CSettings& se = CSettings::ref();
    vector<CFilterDescriptor> filters;
    set<int> ids = se.configs[se.currentConfig];
    for (set<int>::iterator it = ids.begin(); it != ids.end(); it++) {
        map<int,CFilterDescriptor>::iterator itm = se.filters.find(*it);
        if (itm != se.filters.end()
                && itm->second.filterType == CFilterDescriptor::TEXT) {
            filters.push_back(itm->second);
        }
    }

    // Sort filters by decreasing priority
    sort(filters.begin(), filters.end());

    // Text filters must be created and chained in reverse order;
    for (vector<CFilterDescriptor>::reverse_iterator it = filters.rbegin();
                it != filters.rend(); it++) {
        try {
            if (it->errorMsg.empty() && it->filterType == CFilterDescriptor::TEXT) {
                TEXTfilters.push_back(new CTextFilter(owner, *it));
            }
        } catch (parsing_exception) {
            // Invalid filters are just ignored
        }
    }

    currentFilter = TEXTfilters.begin();
}


CTextBuffer::~CTextBuffer() {

    CUtil::deleteVector<CTextFilter>(TEXTfilters);
}


void CTextBuffer::dataReset() {

    buffer.clear();
    currentFilter = TEXTfilters.begin();
    
    for (vector<CTextFilter*>::iterator it = TEXTfilters.begin();
                it != TEXTfilters.end(); it++) {
        (*it)->reset();
    }
    output->dataReset();
}


void CTextBuffer::dataDump() {

    if (owner.killed) return;
    for (vector<CTextFilter*>::iterator it = TEXTfilters.begin();
                it != TEXTfilters.end(); it++) {
        (*it)->endReached();
    }
    dataFeed("");
    output->dataDump();
    owner.killed = true;
}


void CTextBuffer::escapeOutput(stringstream& out, const char *data,
                               size_t len) {
    if (owner.url.getDebug()) {
        string buf;
        CUtil::htmlEscape(buf, string(data, len));
        out << buf;
    } else {
        out << string(data, len);
    }
}


void CTextBuffer::dataFeed(const string& data) {

    if (owner.killed) return;
    buffer += data;
    stringstream out;
    
    const char* bufStart = buffer.data();
    const char* bufEnd   = bufStart + buffer.size();
    const char* index    = bufStart;
    const char* done     = bufStart;
    
    // test every filter at every position
    for (; index <= bufEnd; currentFilter = TEXTfilters.begin(), index++) {
        for (; currentFilter != TEXTfilters.end(); currentFilter++) {

            // don't need to test the filter if character is not a good candidate
            if ((*currentFilter)->bypassed ||
                (index < bufEnd && !(*currentFilter)->okayChars[(unsigned char)(*index)]))
                continue;

            // try and match
            int ret = (*currentFilter)->match(index, bufEnd);

            if (ret < 0) {

                if (owner.killed) {

                    // filter would like more data, but killed the stream anyway
                    escapeOutput(out, done, (size_t)(index - done));
                    buffer.clear();
                    output->dataFeed(out.str());
                    output->dataDump();
                    return;         // no more processing
                    
                } else {

                    // filter does not have enough data to provide an accurate result
                    escapeOutput(out, done, (size_t)(index - done));
                    buffer.erase(0, (size_t)(index - bufStart));
                    output->dataFeed(out.str());
                    return;         // when more data arrives, same filter, same position
                }

            } else if (ret > 0) {

                string replaceText = (*currentFilter)->getReplaceText();

                // log match events
                string occurrence(index, (size_t)((*currentFilter)->endOfMatched - index));
                CLog::ref().logFilterEvent(pmEVT_FILTER_TYPE_TEXTMATCH,
                                           owner.reqNumber,
                                           (*currentFilter)->title, occurrence);
                CLog::ref().logFilterEvent(pmEVT_FILTER_TYPE_TEXTREPLACE,
                                           owner.reqNumber,
                                           (*currentFilter)->title, replaceText);

                escapeOutput(out, done, (size_t)(index - done));
                if (owner.url.getDebug()) {
                    string buf = "<div class=\"match\">\n"
                        "<div class=\"filter\">Match: ";
                    CUtil::htmlEscape(buf, (*currentFilter)->title);
                    buf += "</div>\n<div class=\"in\">";
                    CUtil::htmlEscape(buf, occurrence);
                    buf += "</div>\n<div class=\"repl\">";
                    CUtil::htmlEscape(buf, replaceText);
                    buf += "</div>\n</div>\n";
                    out << buf;
                }
                if (owner.killed) {

                    // filter matched and killed the stream
                    if (!owner.url.getDebug())
                        out << replaceText;
                    buffer.clear();
                    output->dataFeed(out.str());
                    output->dataDump();
                    return;         // no more processing

                } else if ((*currentFilter)->multipleMatches) {

                    // filter matched, insert replace text in buffer
                    buffer.replace(0, (size_t)((*currentFilter)->endOfMatched - bufStart),
                                   replaceText);
                    bufStart = buffer.data();
                    bufEnd   = bufStart + buffer.size();
                    index    = bufStart;
                    done     = bufStart;
                    continue;       // next filter, same position
                    
                } else {

                    // filter matched, output replace text.
                    if (!owner.url.getDebug())
                        out << replaceText;
                    done = (*currentFilter)->endOfMatched;
                    // If the occurrence is length 0, we'll try next filters
                    // then move 1 byte, to avoid infinite loop on the filter.
                    if (done == index) continue;
                    index = done - 1;
                    break;          // first filter, after the occurrence
                }

            } else if (owner.killed) {

                // filter did not match but killed the stream
                escapeOutput(out, done, (size_t)(index - done));
                buffer.clear();
                output->dataFeed(out.str());
                output->dataDump();
                return;             // no more processing
            }
        }
    }
    
    // we processed all the available data.
    // Add unmatching data left and send everything.
    escapeOutput(out, done, (size_t)(bufEnd - done));
    buffer.clear();
    output->dataFeed(out.str());
}
// vi:ts=4:sw=4:et
