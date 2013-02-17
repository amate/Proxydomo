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
             debug(false), source(false) { }
    CUrl(const std::string& str);
    void parseUrl(const std::string& str);
    
    inline const std::string& getUrl()       const { return url;       }
    inline const std::string& getProtocol()  const { return protocol;  }
    inline const std::string& getFromHost()  const { return fromhost;  }
    inline const std::string& getHost()      const { return host;      }
    inline const std::string& getAfterHost() const { return afterhost; }
    inline const std::string& getPath()      const { return path;      }
    inline const std::string& getQuery()     const { return query;     }
    inline const std::string& getAnchor()    const { return anchor;    }
    inline const std::string& getHostPort()  const { return hostport;  }
    inline bool getBypassIn()           const { return bypassIn;  }
    inline bool getBypassOut()          const { return bypassOut; }
    inline bool getBypassText()         const { return bypassText;}
    inline bool getDebug()              const { return debug;     }
    inline bool getSource()             const { return source;    }

private:
    // fromhost is the URL without http://
    // hostport is host:port
    std::string url, protocol, fromhost, host, afterhost, path, query, anchor, hostport;
    bool bypassIn, bypassOut, bypassText, debug, source;
};


