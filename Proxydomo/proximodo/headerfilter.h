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


#ifndef __headerfilter__
#define __headerfilter__

#include "filter.h"
#include <string>
#include <memory>

class CFilterDescriptor;
class CFilterOwner;
namespace Proxydomo { class CMatcher; }

/* class CHeaderFilter
 */
class CHeaderFilter : public CFilter {

public:
    CHeaderFilter(const CFilterDescriptor& desc, CFilterOwner& owner);
    ~CHeaderFilter();
    
    // Matches the header (input) and puts same (if no match)
    // or replaced string in output. If new content is empty,
    // output string will be empty. Returns true if matched.
    // Note that input should not include header-name: and
    // that there can be CRLF in input, and there may be
    // in output too (wrt HTTP standard).
    bool filter(std::wstring& header);

    // Name of processed header (in lowercase)
    std::wstring headerName;

private:
    // URL matcher
    std::shared_ptr<Proxydomo::CMatcher> urlMatcher;

    // Text matcher
    std::shared_ptr<Proxydomo::CMatcher> textMatcher;

    // Replace pattern
    std::wstring replacePattern;
};

#endif
