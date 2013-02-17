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


#include "settings.h"
#include "util.h"
#include "const.h"
#include "matcher.h"
#include "nodes.h"
#include <wx/file.h>
#include <wx/msgdlg.h>
#include <wx/mimetype.h>
#include <wx/textfile.h>
#include <sstream>

using namespace std;


/* Static members
 */
CSettings* CSettings::instance = NULL;
wxMutex CSettings::listsMutex(wxMUTEX_RECURSIVE);


/* Method to obtain the instance address
 */
CSettings& CSettings::ref() {

    if (!instance) {
        instance = new CSettings();
        instance->init();
    }
    return *instance;
}


/* Method to destroy safely the instance (better than delete)
 */
void CSettings::destroy() {

    if (instance) delete instance;
    instance = NULL;
}


/* Constructor
 */
CSettings::CSettings() {

    // Default values
    proxyPort     = "8080";
    enableUrlCmd  = false;
    urlCmdPrefix  = "";
    useNextProxy  = false;
    nextProxy     = "localhost:8088";
    allowIPRange  = false;
    minIPRange    = 0x00000000;
    maxIPRange    = 0x00000000;
    currentConfig = "default";
    filterIn      = true;
    filterOut     = true;
    filterText    = true;
    filterGif     = true;
    language      = DEFAULT_LANGUAGE;
    showOnStartup = true;
    startBrowser  = true;
    firstRun      = false;
    modified      = false;
}


/* Destructor
 */
CSettings::~CSettings() {

    save(true);
}


/* Load all files.
 */
void CSettings::init() {

    loadSettings();
    loadMessages(DEFAULT_LANGUAGE);
    loadMessages(language);
    loadFilters();
    loadLists();
}


/* Remove inexistant filter ids from configs
 */
void CSettings::cleanConfigs() {

    for (map<string, set<int> >::iterator itm = configs.begin();
                itm != configs.end(); itm++) {
        set<int> ids;
        for (set<int>::iterator its = itm->second.begin();
                    its != itm->second.end(); its++) {
            if (filters.find(*its) != filters.end()) {
                ids.insert(*its);
            }
        }
        itm->second = ids;
    }
}


/* Rebuild tree structure in folders
 */
void CSettings::cleanFolders() {

    folders[0] = CFolder(0, "root", 0);
    for (map<int, CFolder>::iterator it = folders.begin();
                it != folders.end(); it++) {
        it->second.children.clear();
        if (folders.find(it->second.parent) == folders.end()) {
            it->second.parent = 0;
        }
    }
    for (map<int, CFolder>::iterator it = folders.begin();
                it != folders.end(); it++) {
        if (it->first != 0) {
            folders[it->second.parent].children.insert(it->first);
        }
    }
}


/* Checks if settings have been saved, or asks the user.
 */
void CSettings::save(bool prompt) {

    if (prompt) {
        if (!modified) return;
        int ret = wxMessageBox(S2W(getMessage("SETTINGS_NOT_SAVED")),
                                                wxT(APP_NAME), wxYES_NO);
        if (ret == wxNO) return;
    }
    saveSettings();
    saveFilters();
    modified = false;
}


/* Loads everything except messages (we don't want GUI to be partly translated)
 */
void CSettings::load() {

    loadSettings();
    loadFilters();
    loadLists();
    modified = false;
}


/* Read a setting from a text file.
 * If the line is [title]\n only title (trimmed) is set.
 * If the line is label=value\n optionally followed by several _value\n
 * only label (trimmed and uppercased) and value (trimmed if needed) are set.
 * returns true if the line conformed one of those two formats.
 */
bool CSettings::readSetting(wxTextFile& f, string& title,
                            string& label, string& value, bool trimValue) {

    title = label = value = "";
    string line = string(f.GetLine(f.GetCurrentLine()).mb_str());
    f.GoToLine(f.GetCurrentLine() + 1);
    // Check for title
    if (line[0] == '[') {
        size_t end = line.find(']', 1);
        if (end == string::npos) return false;
        title = line.substr(1, end-1);
        CUtil::trim(title, " \t");
        return true;
    }
    // Check for label
    size_t eq = line.find('=');
    if (eq == string::npos) return false;
    label = line.substr(0, eq);
    CUtil::trim(label, " \t");
    CUtil::upper(label);
    // Get value
    value = line.substr(eq+1);
    // Add next _lines
    while (f.GetCurrentLine() < f.GetLineCount()) {
        line = string(f.GetLine(f.GetCurrentLine()).mb_str());
        if (line[0] != '_') break;
        value += "\n" + line.substr(1);
        f.GoToLine(f.GetCurrentLine() + 1);
    }
    if (trimValue) CUtil::trim(value);
    return true;
}


/* Add a line to a file
 */
void CSettings::addLine(wxTextFile& f, string s) {

    size_t pos1 = 0, pos2;
    while ((pos2 = s.find("\n", pos1)) != string::npos) {
        f.AddLine(wxString(((pos1?"_":"")+s.substr(pos1, pos2-pos1)).c_str(), wxConvUTF8));
        pos1 = pos2+1;
    }
    f.AddLine(wxString(((pos1?"_":"")+s.substr(pos1)).c_str(), wxConvUTF8));
}


/* Save settings
 */
void CSettings::saveSettings() {

    wxTextFile f(wxT(FILE_SETTINGS));
    if (!f.Create()) {
        f.Open();
        f.Clear();
    }
    
    addLine (f,"[Settings]");
    addLine (f, "ShowOnStartup = " + string(showOnStartup ? "yes" : "no"));
    addLine (f, "StartBrowser = " + string(startBrowser ? "yes" : "no"));
    addLine (f, "BrowserPath = " + browserPath);
    addLine (f, "Port = " + proxyPort);        // For file readability only
    addLine (f, "EnableUrlCommands = " + string(enableUrlCmd ? "yes" : "no"));
    addLine (f, "UrlCommandPrefix = " + urlCmdPrefix);
    addLine (f, "UseProxy = " + string(useNextProxy ? "yes" : "no"));
    addLine (f, "CurrentProxy = " + nextProxy);
    addLine (f, "AllowIPRange = " + string(allowIPRange ? "yes" : "no"));
    addLine (f, "MinIP = " + CUtil::toDotted(minIPRange));
    addLine (f, "MaxIP = " + CUtil::toDotted(maxIPRange));
    addLine (f, "FilterIn = " + string(filterIn ? "yes" : "no"));
    addLine (f, "FilterOut = " + string(filterOut ? "yes" : "no"));
    addLine (f, "FilterText = " + string(filterText ? "yes" : "no"));
    addLine (f, "FilterGif = " + string(filterGif ? "yes" : "no"));
    addLine (f, "Language = " + language);
    addLine (f, "CurrentConfig = " + currentConfig);
    addLine (f, "Bypass = " + bypass);
    addLine (f, "");
    addLine (f, "[Proxies]");
    for (set<string>::iterator it = proxies.begin();
                it != proxies.end(); it++) {
        addLine (f, "Proxy = " + *it);
    }
    addLine (f, "");
    addLine (f, "[Lists]");
    for (map<string,string>::iterator it = listNames.begin();
                it != listNames.end(); it++) {
        addLine (f, "List = " + CUtil::quote(it->first)
                       + ", " + CUtil::quote(it->second));
    }
    addLine (f, "");
    addLine (f, "[Folders]");
    for (map<int,CFolder>::iterator it = folders.begin();
                it != folders.end(); it++) {
        stringstream ss;
        ss << "Folder = " << CUtil::quote(it->second.name);
        ss << ", " << it->first;
        ss << ", " << it->second.parent;
        if (it->second.defaultFolder) ss << ", " << it->second.defaultFolder;
        if (it->first) addLine (f, ss.str());
    }
    addLine (f, "");
    addLine (f, "[Configurations]");
    for (map<string, set<int> >::iterator it1 = configs.begin();
                it1 != configs.end(); it1++) {
        stringstream ss;
        ss << "Config = " << CUtil::quote(it1->first);
        for (set<int>::iterator it2 = it1->second.begin();
                it2 != it1->second.end(); it2++) {
            ss << ", " << *it2;
        }
        addLine (f, ss.str());
    }
    f.Write();
    f.Close();
}


/* Load settings
 */
void CSettings::loadSettings() {

    configs.clear();
    proxies.clear();
    listNames.clear();

    wxTextFile f(wxT(FILE_SETTINGS));
    if (!f.Open()) return;
    while (f.GetCurrentLine() < f.GetLineCount()) {
        string title, label, value;
        readSetting(f, title, label, value, true);
        
        if      (label == "PORT")
            proxyPort = value;
        else if (label == "ENABLEURLCOMMANDS")
            enableUrlCmd = (toupper(value[0])=='Y');
        else if (label == "URLCOMMANDPREFIX")
            urlCmdPrefix = value;
        else if (label == "SHOWONSTARTUP")
            showOnStartup = (toupper(value[0])=='Y');
        else if (label == "STARTBROWSER")
            startBrowser = (toupper(value[0])=='Y');
        else if (label == "FIRSTRUN")
            firstRun = true;
        else if (label == "USEPROXY")
            useNextProxy = (toupper(value[0])=='Y');
        else if (label == "CURRENTPROXY")
            nextProxy = value;
        else if (label == "BROWSERPATH")
            browserPath = value;
        else if (label == "ALLOWIPRANGE")
            allowIPRange = (toupper(value[0])=='Y');
        else if (label == "FILTERIN")
            filterIn = (toupper(value[0])=='Y');
        else if (label == "FILTEROUT")
            filterOut = (toupper(value[0])=='Y');
        else if (label == "FILTERTEXT")
            filterText = (toupper(value[0])=='Y');
        else if (label == "FILTERGIF")
            filterGif = (toupper(value[0])=='Y');
        else if (label == "MINIP")
            minIPRange = CUtil::fromDotted(value);
        else if (label == "MAXIP")
            maxIPRange = CUtil::fromDotted(value);
        else if (label == "LANGUAGE")
            language = value;
        else if (label == "PROXY")
            proxies.insert(value);
        else if (label == "BYPASS") {
            if (CMatcher::testPattern(value)) bypass = value;
        }
        else if (label == "CURRENTCONFIG") {
            currentConfig = value;
            configs[value];
        }
        else if (label == "LIST") {
            string name, path;
            int comma = CUtil::getQuoted(value, name, -1, ',');
            if (name.empty()) continue;
            comma = CUtil::getQuoted(value, path, comma, ',');
            if (path.empty()) continue;
            listNames[name] = path;
        }
        else if (label == "CONFIG") {
            string name, param;
            int comma = CUtil::getQuoted(value, name, -1, ',');
            if (name.empty()) continue;
            set<int>& ids = configs[name];
            while (comma >= 0) {
                comma = CUtil::getQuoted(value, param, comma, ',');
                stringstream ss(param);
                int id = -1;
                ss >> id;
                ids.insert(id);
            }
        }
        else if (label == "FOLDER") {
            string s1, s2, s3, s4;
            stringstream ss2, ss3, ss4;
            int id2 = 0, id3 = 0, id4 = 0;
            int comma = -1;
            comma = CUtil::getQuoted(value, s1, comma, ',');
            if (s1.empty()) continue;
            comma = CUtil::getQuoted(value, s2, comma, ',');
            ss2 << s2; ss2 >> id2;
            if (id2 == 0) continue;
            comma = CUtil::getQuoted(value, s3, comma, ',');
            ss3 << s3; ss3 >> id3;
            comma = CUtil::getQuoted(value, s4, comma, ',');
            ss4 << s4; ss4 >> id4;
            folders[id2] = CFolder(id2, s1, id3, id4);
        }
    }
    
    // If showOnStartup and startBrowser are both false, starting
    // Proximodo will do nothing visible.
    // *** The user might want this! since they explicitly chose
    //     it in settings screen (Kuruden)
    
    cleanFolders();
    f.Close();
    
    // Define browser executable path on first run
    if (firstRun && browserPath.empty()) {
        wxFileType* type = wxTheMimeTypesManager->GetFileTypeFromMimeType(wxT("text/html"));
        if (type) {
            wxString command = type->GetOpenCommand(wxString(wxT("")));
            browserPath = CUtil::getExeName(string(command.mb_str()));
        }
    }
}


/* Load localized messages for use in GUI.
 * It overloads messages currently in memory but keeps those that are
 * not present in the file (so that default messages appear for
 * non-translated ones).
 * Note that all message labels are in uppercase.
 */
void CSettings::loadMessages(string language) {

    wxTextFile f(S2W(language+".lng"));
    if (!f.Open()) return;
    while (f.GetCurrentLine() < f.GetLineCount()) {
        string title, label, value;
        readSetting(f, title, label, value, false);
        if (!label.empty()) messages[label] = value;
    }
    f.Close();
}


/* This function retrieves a message with the label, and replaces
 * parameters by those given, if needed.
 * Places in message where to insert label is indicated by %1 %2 and %3
 */
string CSettings::getMessage(string name,
                string param1, string param2, string param3, string param4) {

    if (messages.find(name) == messages.end()) return name;
    string mess = messages[name];
    size_t pos = 0;
    while ((pos = mess.find('%', pos)) != string::npos && pos+1 < mess.size()) {
        switch (mess[pos+1]) {
            case '1': mess.replace(pos, 2, param1); pos += param1.size(); break;
            case '2': mess.replace(pos, 2, param2); pos += param2.size(); break;
            case '3': mess.replace(pos, 2, param3); pos += param3.size(); break;
            case '4': mess.replace(pos, 2, param4); pos += param4.size(); break;
        }
    }
    return mess;
}

string CSettings::getMessage(string name, int number) {

    stringstream ss;
    ss << number;
    return getMessage(name, ss.str());
}


/* Load all files of patterns.
 */
void CSettings::loadLists() {

    wxMutexLocker lock(listsMutex);
    lists.clear();
    for (map<string,string>::iterator it = listNames.begin();
                it != listNames.end(); it++) {
        loadList(it->first);
    }
}


/* Load a file of patterns.
 * Blank lines, lines starting with # and invalid patterns are ignored.
 */
void CSettings::loadList(string name) {

    string fn(listNames[name]);
    for (int i = 0; fn[i]; i++)   // Replace \ with / in paths on Unix
        if (fn[i] == wxFILE_SEP_PATH_UNIX || fn[i] == wxFILE_SEP_PATH_DOS)
            fn[i] = wxFILE_SEP_PATH;
    wxTextFile f(S2W(fn));
    deque<string> patterns;
    if (f.Open()) {
        f.AddLine(wxT(""));  // (so that we don't need post-loop processing)
        string pattern;
        while (f.GetCurrentLine() < f.GetLineCount()) {
            string line = string(f.GetLine(f.GetCurrentLine()).mb_str());
            f.GoToLine(f.GetCurrentLine() + 1);

            if (!line.empty() && (line[0] == ' ' || line[0] == '\t')) {
                pattern += line;
            } else {
                CUtil::trim(pattern);
                if (!pattern.empty() && CMatcher::testPattern(pattern))
                    patterns.push_back(pattern);
                if (line.empty() || line[0] == '#') {
                    pattern.clear();
                } else {
                    pattern = line;
                }
            }
        }
        f.Close();
    }
    lists[name] = patterns;
}


/* Add a line to a list.
 * The insertion is done in memory (if valid pattern) and on HDD.
 */
void CSettings::addListLine(string name, string line) {

    // We do nothing if the line is empty
    if (CUtil::trim(line).empty() || line == "~") return;
    
    // we must lock access to 'lists'
    wxMutexLocker lock(listsMutex);

    // We add the line to the file if the list exists
    if (listNames.find(name) != listNames.end()) {

        /* do we really want this?
        // multiline patterns are converted to single line, to avoid
        // the danger of having the 2nd+ line not beginning by a space
        line = CUtil::replaceAll(line, "\r", "");
        line = CUtil::replaceAll(line, "\n", "");
        */

        // Append line to file
        wxFile f(S2W(listNames[name]), wxFile::write_append);
        if (f.IsOpened()) {
            f.Write(wxString(line.c_str(), wxConvUTF8)+wxT("\r\n"));
            f.Close();
        }

        // If line is a valid pattern, keep it in memory
        if (line[0] != '#' && CMatcher::testPattern(line)) {
            lists[name].push_back(line);
            CNode_List::notifyPatternPushBack(name, line);
        }

    } else {

        // This is a virtual list, always add the line
        deque<string>& list = lists[name];
        list.push_back(line);
        CNode_List::notifyPatternPushBack(name, line);
        
        // we will limit its size
        if (list.size() > VIRTUAL_LIST_MAXSIZE) {
            CNode_List::notifyPatternPopFront(name, list.front());
            list.pop_front();
        }
    }
}


/* Save filters
 */
void CSettings::saveFilters() {

    wxFile f(wxT(FILE_FILTERS), wxFile::write);
    if (f.IsOpened()) {
        f.Write(S2W(CFilterDescriptor::exportFilters(folders, filters)));
    }
}


/* Load filters
 */
void CSettings::loadFilters() {

    // Load file content into memory
    stringstream text;
    wxFile f(wxT(FILE_FILTERS));
    if (f.IsOpened()) {
        while (!f.Eof()) {
            char buf[1024];
            size_t n = f.Read(buf, 1024);
            text << string(buf, n);
        }
    }

    // Decode filters into the filter bank
    filters.clear();
    CFilterDescriptor::importFilters(text.str(), folders, filters);

    // Apply language's default filters' descriptions
    for (map<int,CFilterDescriptor>::iterator it = filters.begin();
                it != filters.end(); it++) {

        // Change titles, comments and categories of (unmodified)
        // default filters according to language file
        if (it->second.defaultFilter) {
            stringstream prefix;
            prefix << "FILTER_" << it->second.defaultFilter << "_";
            string mess, text;
            mess = prefix.str() + "TITLE";
            text = getMessage(mess);
            if (text != mess) it->second.title = text;
            mess = prefix.str() + "COMMENT";
            text = getMessage(mess);
            if (text != mess) it->second.comment = text;
        }
    }
    
    // Apply language's default folders' names
    for (map<int,CFolder>::iterator it = folders.begin();
                it != folders.end(); it++) {
        if (it->second.defaultFolder) {
            stringstream mess;
            mess << "FOLDER_" << it->second.defaultFolder << "_NAME";
            string text = getMessage(mess.str());
            if (text != mess.str()) it->second.name = text;
        }
    }

    cleanConfigs();
}
// vi:ts=4:sw=4:et
