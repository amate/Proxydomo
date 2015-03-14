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


#include "expander.h"
#include "url.h"
#include "memory.h"
//#include "const.h"
#include "util.h"
//#include "settings.h"
#include "..\Matcher.h"
//#include "log.h"
#include "..\Log.h"
//#include "logframe.h"
#include "filter.h"
#include "..\filterowner.h"
//#include <wx/msgdlg.h>
//#include <wx/textdlg.h>
//#include <wx/datetime.h>
//#include <wx/filefn.h>
#include <map>
#include <vector>
#include <sstream>
#include <set>
#include <wctype.h>
#include "..\CodeConvert.h"
#include "..\Settings.h"
#include "..\AppConst.h"

using namespace std;
using namespace Proxydomo;
using namespace CodeConvert;

/* Decode the pattern.
 * In case a conditional command returns false, the decoding is stopped.
 *
 * Note: The filter using expand() MUST unlock the mutex if it is still
 * locked. It cannot be done from inside CExpander (nor CMatcher) because
 * of possible recursive calls.
 */
std::wstring CExpander::expand(const std::wstring& pattern, CFilter& filter) {

    std::wostringstream output;       // stream for the return string
    std::wstring command;             // decoded command name
    std::wstring content;             // decoded command content

    // Iterator to stacked memories
    auto stackedMem = filter.memoryStack.begin();

    size_t size = pattern.size();
    size_t index = 0;
    
    while (index < size) {

        // We can drop the pattern directly to the
        // output up to a special character
        size_t pos = pattern.find_first_of( L"$\\\r" , index);
        if (pos == std::wstring::npos) {
            // No more special chars, drop the pattern to the end
            output << pattern.substr(index);
            index = size;
            continue;
        }
            
        // Send previous characters to output
        output << pattern.substr(index, pos - index);
        index = pos;

		if (pattern[pos] == L'\r') {
			ATLASSERT(pattern[pos + 1] == '\n');
			index += 2;
			continue;

		} else if (pattern[pos] == L'\\') {

            // ensure there is something after the backslash
            if (pos == size-1) {
                // the pattern is terminated by \ so do as for a normal character
                output << L'\\';
                index++;
                continue;
            }
            
            // this is an escaped code
            UChar c = pattern[index + 1];
            index += 2;
            switch (c) {
            case L'#':
                // get one more memorized string from the stack
                if (stackedMem != filter.memoryStack.end()) {
                    if (stackedMem->isByValue()) {
                        output << expand(stackedMem->getValue(), filter);
                    } else {
                        output << stackedMem->getValue();
                    }
                    stackedMem++;
                }
                break;
            case L'@':
                // get all remaining memorized strings from the stack
                while (stackedMem != filter.memoryStack.end()) {
                    if (stackedMem->isByValue()) {
                        output << expand(stackedMem->getValue(), filter);
                    } else {
                        output << stackedMem->getValue();
                    }
                    stackedMem++;
                }
                break;
            case L'k':
                filter.owner.killed = true;
                break;
            // simple escaped codes
            case L't': output << L'\t'; break;
            case L'r': output << L'\r'; break;
            case L'n': output << L'\n'; break;
            case L'u': output << UTF16fromUTF8(filter.owner.url.getUrl()); break;
            case L'h': output << UTF16fromUTF8(filter.owner.url.getHost()); break;
            case L'p': output << UTF16fromUTF8(filter.owner.url.getPath()); break;
            case L'q': output << UTF16fromUTF8(filter.owner.url.getQuery()); break;
            case L'a': output << UTF16fromUTF8(filter.owner.url.getAnchor()); break;
            //case 'd': output << wxT("file:///");
            //          output << CUtil::replaceAll(W2S(wxGetCwd()), "\\", "/");
                  break;

			case L'x':
				output << UTF16fromUTF8(CSettings::s_urlCommandPrefix);
				break;

            default :
                if (CUtil::digit(c)) {
                    // code \0-9 we get the corresponding memorized string
                    CMemory& mem = filter.memoryTable[c - L'0'];
                    if (mem.isByValue()) {
                        output << expand(mem.getValue(), filter);
                    } else {
                        output << mem.getValue();
                    }
                } else {
                    // escaped special codes, we drop them to the output
                    output << c; break;
                }
            }

        } else {    // '$'

            // Decode the command name, i.e all following uppercase letters
            command.clear();
			for (pos = index + 1; pos < size && iswupper(pattern[pos]); pos++)
                command += pattern[pos];
                
            // Does it look like a command?
            if (command.size() > 0 && pos < size && pattern[pos] == '(') {

                // Decode content
                content.clear();
                int level = 1;
                size_t end = pos + 1;
                while (   end<size
                       && (   level>1
                           || pattern[end]!=L')'
                           || pattern[end-1]==L'\\' ) ) {
                    if (pattern[end] == L'(' && pattern[end-1] != L'\\') {
                        level++;
                    } else if (pattern[end] == L')' && pattern[end-1] != L'\\') {
                        level--;
                    }
                    end++;
                }
                content = pattern.substr(pos+1, end-pos-1);
                if (end<size) end++;
                index = end;
                
                // PENDING: Commands
                // Each command is accessed in its wxT("else if") block.
                // For each command, the call and use of results can be slightly different.
                // For example, the _result_ of $GET() is parsed before feeding the output.
                // Note that commands that are not CGenerator-dependent (ex. $ESC) should
                // be placed in another class.
                // This parse function will have to be modified to take into account
                // commands with several comma-separated parameters.
                //   output << f_COMMAND(parse(pos+1, pos));

                if (command == L"GET") {

                    CUtil::trim(content);
                    CUtil::lower(content);
                    output << filter.owner.variables[content];

                } else if (command == L"SET") {

                    size_t eq = content.find(L'=');
                    if (eq == string::npos) continue;
                    std::wstring name = content.substr(0, eq);
                    std::wstring value = content.substr(eq+1);
                    CUtil::trim(name);
                    CUtil::lower(name);
                    if (name[0] == L'\\') name.erase(0,1);
                    if (name == L"#") {
                        filter.memoryStack.push_back(CMemory(value));
                    } else if (name.size() == 1 && CUtil::digit(name[0])) {
                        filter.memoryTable[name[0] - L'0'] = CMemory(value);
                    } else {
                        filter.owner.variables[name] = expand(value, filter);
                    }

                } else if (command == L"SETPROXY") {
                    CUtil::trim(content);
					auto it = CSettings::s_setRemoteProxy.find(UTF8fromUTF16(content));
					if (it != CSettings::s_setRemoteProxy.end()) {
						filter.owner.contactHost = *it;
						filter.owner.useSettingsProxy = false;
					}
                } else if (command == L"USEPROXY") {
                    CUtil::trim(content);
                    CUtil::lower(content);
                    if (content == L"true") {
                        filter.owner.useSettingsProxy = true;
                    } else if (content == L"false") {
                        filter.owner.useSettingsProxy = false;
                    }
                } else if (command == L"ALERT") {
                    //wxMessageBox(S2W(expand(content, filter)), wxT(APP_NAME));
					std::wstring text = CExpander::expand(content, filter);
					MessageBox(NULL, text.c_str(), APP_NAME, MB_OK);

                } else if (command == L"CONFIRM") {

                    //int answer = wxMessageBox(S2W(expand(content, filter)),
                    //                            wxT(APP_NAME), wxYES_NO);
                    //if (answer == wxNO) index = size;

                } else if (command == L"ESC") {

                    std::wstring value = expand(content, filter);
                    output << CUtil::ESC(value);
                    
                } else if (command == L"UESC") {

                    std::wstring value = expand(content, filter);
                    output << CUtil::UESC(value);

                } else if (command == L"WESC") {

                    std::wstring value = expand(content, filter);
                    output << CUtil::WESC(value);

                } else if (command == L"STOP") {

                    filter.bypassed = true;

                } else if (command == L"JUMP") {

                    filter.owner.rdirToHost = UTF8fromUTF16(expand(content, filter));
                    CUtil::trim(filter.owner.rdirToHost);
                    filter.owner.rdirMode = 0;

                } else if (command == L"RDIR") {

                    filter.owner.rdirToHost = UTF8fromUTF16(expand(content, filter));
                    CUtil::trim(filter.owner.rdirToHost);
                    filter.owner.rdirMode = 1;

                } else if (command == L"FILTER") {

                    CUtil::trim(content);
                    CUtil::lower(content);
                    if (content == L"true") {
                        filter.owner.bypassBody = false;
                        filter.owner.bypassBodyForced = true;
                    } else if (content == L"false") {
                        filter.owner.bypassBody = true;
                        filter.owner.bypassBodyForced = true;
                    }

                } else if (command == L"FILE") {

                    output << UTF16fromUTF8(CUtil::getFile(UTF8fromUTF16(expand(content, filter))));

                } else if (command == L"LOG") {

                    std::wstring log = expand(content, filter);
					CLog::FilterEvent(kLogFilterLogCommand, filter.owner.requestNumber, UTF8fromUTF16(filter.title), UTF8fromUTF16(log));

                } else if (command == L"LOCK") {

                    if (!filter.locked) {
                        //CLog::ref().filterLock.Lock();
                        filter.locked = true;
                    }

                } else if (command == L"UNLOCK") {

                    if (filter.locked) {
                        //CLog::ref().filterLock.Unlock();
                        filter.locked = false;
                    }

                } else if (command == L"KEYCHK") {

                    if (!CUtil::keyCheck(CUtil::upper(content))) index = size;

                } else if (command == L"ADDLST") {

                    size_t comma = content.find(',');
                    if (comma != string::npos) {
                        std::wstring name = content.substr(0, comma);
                        std::wstring value = content.substr(comma + 1);
                        CUtil::trim(name);
                        //CSettings::ref().addListLine(name, expand(value, filter));
                    }

                } else if (command == L"ADDLSTBOX") {

                    //size_t comma = content.find(',');
                    //if (comma != string::npos) {
                    //    string name = content.substr(0, comma);
                    //    CUtil::trim(name);
                    //    string title = APP_NAME;
                    //    string value = content.substr(comma + 1);
                    //    comma = CUtil::findUnescaped(value, ',');
                    //    if (comma != string::npos) {
                    //        title = value.substr(0, comma);
                    //        value = value.substr(comma + 1);
                    //    }
                    //    title = expand(title, filter);
                    //    value = expand(value, filter);
                    //    string message = CSettings::ref().getMessage("ADDLSTBOX_MESSAGE", name);
                    //    wxTextEntryDialog dlg(NULL, S2W(message), S2W(title), S2W(value));
                    //    if (dlg.ShowModal() == wxID_OK)
                    //        CSettings::ref().addListLine(name, W2S(dlg.GetValue()));
                    //}

                } else if (command == L"URL") {

                    const wchar_t *tStart, *tStop, *tEnd, *tReached;
                    const std::wstring url = UTF16fromUTF8(filter.owner.url.getUrl());
                    tStart = url.c_str();
                    tStop = tStart + url.size();
                    // Here we use the static matching function
                    bool ret = CMatcher::match(content, filter,
                                               tStart, tStop, tEnd, tReached);
                    if (!ret) index = size;

                } else if (command == L"RESP") {

                    const wchar_t *tStart, *tStop, *tEnd, *tReached;
                    std::wstring& resp = UTF16fromUTF8(filter.owner.responseCode);
                    tStart = resp.c_str();
                    tStop = tStart + resp.size();
                    bool ret = CMatcher::match(content, filter,
                                               tStart, tStop, tEnd, tReached);
                    if (!ret) index = size;

                } else if (command == L"TST") {

                    const wchar_t *tStart, *tStop, *tEnd, *tReached;
                    size_t eq, lev;
                    for (eq = 0, lev = 0; eq < size; eq++) {
                        // Left parameter can be some text containing ()
                        if (content[eq] == L'(' && (!eq || content[eq-1]!=L'\\'))
                            lev++;
                        else if (content[eq] == L')' && (!eq || content[eq-1]!=L'\\'))
                            lev--;
                        else if (content[eq] == L'=' && !lev)
                            break;
                    }

                    if (eq == size) { eq = 0; content = L'='; }
                    std::wstring value = content.substr(eq + 1);
                    std::wstring name = content.substr(0, eq);
                    CUtil::trim(name);
                    if (name[0] != L'(') CUtil::lower(name);
                    if (name[0] == L'\\') name.erase(0,1);
                    std::wstring toMatch;
                    if (name == L"#") {
                        if (!filter.memoryStack.empty())
                            toMatch = filter.memoryStack.back().getValue();
                    } else if (name.size() == 1 && CUtil::digit(name[0])) {
                        toMatch = filter.memoryTable[name[0]-L'0'].getValue();
                    } else if (name[0] == L'(' && name[name.size()-1] == L')') {
                        name = name.substr(1, name.size()-2);
                        toMatch = expand(name, filter);
                    } else {
                        toMatch = filter.owner.variables[name];
                    }

                    tStart = toMatch.c_str();
                    tStop = tStart + toMatch.size();
                    bool ret = !toMatch.empty() &&
                               CMatcher::match(value, filter,
                                               tStart, tStop, tEnd, tReached);
                    if (!ret || tEnd != tStop) index = size;

                } else if (command == L"IHDR") {

                    const wchar_t *tStart, *tStop, *tEnd, *tReached;
                    size_t colon = content.find(L':');
                    if (colon == string::npos) { pos=0; content = L":"; }
                    std::wstring pattern = content.substr(colon + 1);
                    std::wstring name = content.substr(0, colon);
                    CUtil::trim(name);
                    CUtil::lower(name);
					std::wstring value = UTF16fromUTF8(filter.owner.GetInHeader(UTF8fromUTF16(name)));
                    tStart = value.c_str();
                    tStop = tStart + value.size();
                    if (!CMatcher::match(pattern, filter,
                                         tStart, tStop, tEnd, tReached))
                        index = size;

                } else if (command == L"OHDR") {

                    const wchar_t *tStart, *tStop, *tEnd, *tReached;
                    size_t colon = content.find(L':');
                    if (colon == string::npos) { pos=0; content = L":"; }
                    std::wstring pattern = content.substr(colon + 1);
                    std::wstring name = content.substr(0, colon);
                    CUtil::trim(name);
                    CUtil::lower(name);
					std::wstring value = UTF16fromUTF8(filter.owner.GetOutHeader(UTF8fromUTF16(name)));
                    tStart = value.c_str();
                    tStop = tStart + value.size();
                    if (!CMatcher::match(pattern, filter,
                                         tStart, tStop, tEnd, tReached))
                        index = size;

                } else if (command == L"DTM") {
#if 0
                    string day[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
                    stringstream ss;
                    wxDateTime now = wxDateTime::UNow();
                    size_t i = 0;
                    while (i < content.size()) {
                        char c = content[i];
                        switch (c) {
                        case 'Y' : ss << now.GetYear(); break;
                        case 'M' : ss << CUtil::pad(now.GetMonth()+1,2); break;
                        case 'D' : ss << CUtil::pad(now.GetDay(),2); break;
                        case 'H' : ss << CUtil::pad(now.GetHour(),2); break;
                        case 'm' : ss << CUtil::pad(now.GetMinute(),2); break;
                        case 's' : ss << CUtil::pad(now.GetSecond(),2); break;
                        case 't' : ss << CUtil::pad(now.GetMillisecond(),3); break;
                        case 'h' : ss << CUtil::pad(((now.GetHour()+11)%12+1),2); break;
                        case 'a' : ss << (now.GetDay()<12?wxT("am"):wxT("pm")); break;
                        case 'c' : ss << filter.owner.reqNumber; break;
                        case 'w' : ss << day[now.GetWeekDay()]; break;
                        case 'T' : content.replace(i, 1, "H:m:s"); continue;
                        case 'U' : content.replace(i, 1, "M/D/Y"); continue;
                        case 'E' : content.replace(i, 1, "D/M/Y"); continue;
                        case 'd' : content.replace(i, 1, "Y-M-D"); continue;
                        case 'I' : ss << W2S(now.Format(wxT("%a, %d %b %Y %H:%M:%S GMT"), wxDateTime::GMT0)); break;
                        case '\\' : if (i+1 < content.size()) { ss << content[i+1]; i++; } break;
                        default  : ss << c;
                        }
                        i++;
                    }
                    output << ss.str();
#endif
                }
            } else {
                // not a command, consume the $ as a normal character
                output << '$';
                index++;
            }
        }
    }
    // End of pattern, return the decoded string
    return output.str();
}

