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

#pragma once

#include <string>

/* class CUrl
 * Parses a URL once and for all, so that we can quickly retrieve
 * a part of the URL without reparsing.
 */
class CUrl 
{
public:
    CUrl() : bypassIn(false), bypassOut(false), bypassText(false),
             debug(false), source(false), https(false) { }
    CUrl(const std::wstring& str);
    void parseUrl(const std::wstring& str);
    
    inline const std::wstring& getUrl()       const { return url;       }
    inline const std::wstring& getProtocol()  const { return protocol;  }
    inline const std::wstring& getFromHost()  const { return fromhost;  }
    inline const std::wstring& getHost()      const { return host;      }
    inline const std::wstring& getAfterHost() const { return afterhost; }
    inline const std::wstring& getPath()      const { return path;      }
    inline const std::wstring& getQuery()     const { return query;     }
    inline const std::wstring& getAnchor()    const { return anchor;    }
    inline const std::wstring& getHostPort()  const { return hostport;  }
    inline bool getBypassIn()           const { return bypassIn;  }
    inline bool getBypassOut()          const { return bypassOut; }
    inline bool getBypassText()         const { return bypassText;}
    inline bool getDebug()              const { return debug;     }
    inline bool getSource()             const { return source;    }
	inline bool getHttps()				const { return https;	  }

private:
    // fromhost is the URL without http://
    // hostport is host:port
    std::wstring url, protocol, fromhost, host, afterhost, path, query, anchor, hostport;
    bool bypassIn, bypassOut, bypassText, debug, source, https;
};


