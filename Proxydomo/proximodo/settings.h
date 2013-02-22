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


#ifndef __settings__
#define __settings__

//#include "descriptor.h"
#include <wx/thread.h>
#include <set>
#include <map>
#include <string>
#include <vector>
#include <deque>
class wxTextFile;

using namespace std;

/* This class contains all settings, filters and messages. It is implemented as
 * a singleton. When the instance is created, it loads settings (settings.txt),
 * messages (english.lng then <language>.lng), filters (filters.txt) and lists
 * (<listname>.txt).
 *
 * All GUI messages, even menu items, should be recorded in the <language>.lng .
 * When a message in not found, the english message is used instead.
 *
 * Lists are loaded in memory, but each time a line in added to one, the
 * insertion is done both in memory and on HDD. There is no saveFile() method,
 * because we don't want to mess the user's comments and so on. If the GUI lets
 * the user edit a file, it will be opened, edited and saved as a plain text
 * before calling loadList().
 *
 * Note that depending on changes in settings, the proxy may be started /
 * stopped / refreshed:
 * - $ADDLST, addListLine(), loadList(), removing a list: nothing to do (because
 *   modifications are taken into account dynamically by the request managers)
 * - Changing proxies/next proxy: n.t.d
 * - IPRange changes: n.t.d
 * - Changing language: close and reopen main window, n.t.d about proxy
 * - Exiting application: closeProxyPort(), abortConnections()
 * - loadSettings(): closeProxyPort(), refreshManagers(), openProxyPort()
 * - Changing proxyPort: closeProxyPort(), openProxyPort()
 * - loadFilters(), changing currentConfig / configs / filters used in current
 *   config: refreshManagers()
 */
class CSettings {

public:
    // Singleton methods
    static CSettings& ref();
    static void destroy();
    
    string         currentConfig;  // active configuration
    string         proxyPort;      // proxy server's listening port
    bool           enableUrlCmd;   // allow URL commands (dbug.., bweb.., etc.)
    string         urlCmdPrefix;   // prefix for URL commands
    bool           useNextProxy;   // does Proximodo connects through another proxy
    string         nextProxy;      // proxy through which Proximodo connects to internet
    set<string>    proxies;        // list of known proxies. ':nnnn' is mandatory
    bool           allowIPRange;   // allows non-local client connections
    unsigned long  minIPRange;     // first allowed client IP
    unsigned long  maxIPRange;     // last allowed client IP
    string         bypass;         // bypass-URL pattern
    bool           filterIn;       // is incoming header filtering active
    bool           filterOut;      // is outgoing header filtering active
    bool           filterText;     // is text filtering active
    bool           filterGif;      // is GIF image freezing active
    bool           showOnStartup;  // Indicates if the GUI is shown on startup
    bool           startBrowser;   // Indicates whether the default browser should be run at startup
    string         language;       // GUI language
    bool           firstRun;       // only true on first Proximodo run
    bool           settingsChanged;// set to true when settings are modified
    bool           modified;       // set to true when settings/filters are modified
    string         browserPath;    // path and name of browser's executable

    map<int, CFilterDescriptor>  filters;    // filter name, description
    map<int, CFolder>            folders;    // folder id, folder data
    map<string, set<int> >       configs;    // config name, set of filter titles
    map<string, string>          listNames;  // list name, file path
    map<string, deque<string> >  lists;      // list name, content

    // Locks 'lists' for read/write operations
    static wxMutex listsMutex;
    
    // Remove inexistant filter ids from config
    void cleanConfigs();
    
    // Rebuild tree structure in folders
    void cleanFolders();

    // Save settings and filters.
    // If prompt is true, we only save if settings changed AND user agrees
    void save(bool prompt = false);
    
    // Load settings and filters.
    void load();

    // Load lists.
    void loadLists();
    
    // Add a line to a list
    void addListLine(string name, string line);

    // Get a message
    string getMessage(string name, int number);
    string getMessage(string name, string param1 = "", string param2 = "",
                                   string param3 = "", string param4 = "");

protected:
    CSettings();
    ~CSettings();

private:
    static CSettings* instance;
    
    void init();
    void loadSettings();
    void saveSettings();
    void loadFilters();
    void saveFilters();
    void loadList(string name);
    void loadMessages(string language);

    // Configuration file I/O
    void addLine(wxTextFile& f, string s);
    bool readSetting(wxTextFile& f, string& title, string& label,
                        string& value, bool trimValue);

    // Messages
    map<string, string> messages;  // label, text
};

#endif
// vi:ts=4:sw=4:et
