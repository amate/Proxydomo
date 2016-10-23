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


#ifndef __util__
#define __util__

#include <string>
#include <vector>
#include <map>

using namespace std;

/* This static class contains generic functions
 */
class CUtil {

public:
    // Locates the next end-of-line
    static bool endOfLine(const string& str, size_t start,
                          size_t& pos, size_t& len, int nbr=1);
    
    // Small function replacing C function isdigit()
    static inline bool digit(char c) { return (c<='9' && c>='0'); }
    static inline bool digit(wchar_t c) { return (c<=L'9' && c>=L'0'); }
    
    // Returns true if character is an hexadecimal digit
    static inline bool hexa(char c) { return ((c<='9' && c>='0') ||
                                        (toupper(c)<='F' && toupper(c)>='A')); }
    static inline bool hexa(wchar_t c) { return ((c<=L'9' && c>=L'0') ||
										(towupper(c)<=L'F' && towupper(c)>=L'A')); }

    // Case-insensitive compare
    static bool noCaseEqual(const string& s1, const string& s2);
	static bool noCaseEqual(const wstring& s1, const wstring& s2);

    // Returns true if s2 begins with s1
    static bool noCaseBeginsWith(const string& s1, const string& s2);
	static bool	noCaseBeginsWith(const wstring& s1, const wstring& s2);

    // Returns true if s2 contains s1
    static bool noCaseContains(const string& s1, const string& s2);
	static bool noCaseContains(const wstring& s1, const wstring& s2);

    // Trim string
    static string& trim(string& s, string list = string(" \t\r\n"));
	static wstring& trim(wstring& s, wstring list = wstring(L" \t\r\n"));

    // Decode hexadecimal number at string start
    static unsigned int readHex(const string& s);

    // Make a hex representation of a number
    static string makeHex(unsigned int n);

    // Append b to a, escaping HTML chars (< > &) as needed
    static void htmlEscape(string& a, const string& b);
	static std::wstring htmlEscape(const wchar_t* data, size_t len);

    // Deletes the dynamic content of a vector
    template <typename V>
    static void deleteVector(vector<V*>& v) {
        for (typename vector<V*>::iterator it = v.begin();
                    it != v.end(); it++)
            delete *it;
        v.clear();
    }

    // Deletes the dynamic content of a map
    template <typename K, typename V>
    static void deleteMap(map<K,V*>& m) {
        for (typename map<K,V*>::iterator it = m.begin();
                    it != m.end(); it++)
            delete it->second;
        m.clear();
    }

    // Put string in uppercase
    static string& upper(string& s);
    static wstring& upper(wstring& s);

    // Put string in lowercase
    static string& lower(string& s);
    static wstring& lower(wstring& s);

    // Convert a numerical IPV4 address to its dotted representation
    static string toDotted(unsigned long adr);

    // Convert a dotted IPV4 address to its ulong value
    static unsigned long fromDotted(string adr);

    // Escape a string
    static string ESC(const string& str);
    static wstring ESC(const wstring& str);

    // Escape special characters in a string
    static string WESC(const string& str);
    static wstring WESC(const wstring& str);

    // Unescape a string
    static string UESC(const string& str);
    static wstring UESC(const wstring& str);

    // Formats a number
    static wstring pad(int n, int size);

    // Check if keys are pressed
    static bool keyCheck(const string& keys);
    static bool keyCheck(const wstring& keys);

    // Finds the first unescaped occurence of a character in a string
    static size_t findUnescaped(const string& str, char c);

    // Replace all occurences of a string by another
    static string replaceAll(const string& str, string s1, string s2);
    static wstring replaceAll(const wstring& str, wstring s1, wstring s2);

    // Get the content of a binary file
    static string getFile(string filename);
	static string getFile(wstring filename);

    // Get MIME type of a file
    static string getMimeType(string filename);

    // Increment a string that will be terminated by (n)
    static string& increment(string& str);

    // Tell if a string is exclusively composed of digits
    static bool isUInt(string s);

    // Set content of clipboard
    //static void setClipboard(const string& str);

    // Get content of clipboard
    //static string getClipboard();

    // MIME BASE64 encoder/decoder
    static string encodeBASE64(const string& str);
    static string decodeBASE64(const string& str);

    // Launch default browser (for a Proximodo help page)
    //static void openBrowser(const string& path = "");

    // Launch default text editor (for a list file)
    static void openNotepad(const string& path);

    // Extract the executable name from a command line
    static string getExeName(const string& cmd);

    // Function for sorting a list control
    //static int wxCALLBACK sortFunction(long item1, long item2, long sortData);

    // Function to show and raise a top-level window
    //static void show(wxTopLevelWindow* window);

    // Converts / to the platform's path separator
    static string makePath(const string& str);
	static wstring makePath(const wstring& str);

    // Converts platform's path separators to /
    static string unmakePath(const string& str);
    
    // Quote a string if it contains quotes or special characters
    static string quote(string str, string codes = "/,");
    static string unquote(string str);
    static int getQuoted(const string& str, string& out,
                         int start = -1, char token = '\0');

private:
    // Case-insensitive compare binary function
    struct insensitive_compare : binary_function<char,char,bool> {
        bool operator()( const char& a, const char& b) {
            return tolower(a) == tolower(b);
        }
    };
};

#endif
// vi:ts=4:sw=4:et
