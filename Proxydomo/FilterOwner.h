/**
*	@file	FilterOwner.h
*
*/
/**
	this file is part of Proxydomo
	Copyright (C) amate 2013-

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#pragma once

#include <string>
#include <vector>
#include <map>
#include "proximodo\url.h"

/* class CFilterOwner
 * This class only contains and implements what is common to
 * filter owners (such as CRequestManager and filter tester window).
 * URL and variables are such common members.
 */

typedef std::vector<std::pair<std::string, std::string>> HeadPairList;		// first:name  second:value

class CFilterOwner
{
public:
	CUrl		url;
	std::string responseCode;            // response code from website
	long		requestNumber;

	int		    cnxNumber;                   // number of the connection

	bool		useSettingsProxy;			 // can be overridden by $USEPROXY
	
	std::string contactHost;				 // can be overridden by $SETPROXY
    std::string	rdirToHost;                  // set by $RDIR and $JUMP
    int			rdirMode;                    // 0: 302 response, 1: transparent

	bool		bypassIn;                    // tells if we can filter incoming headers
	bool		bypassOut;                   // tells if we can filter outgoing headers
	bool		bypassBody;                  // will tell if we can filter the body or not
	bool		bypassBodyForced;            // set to true if $FILTER changed bypassBody

	bool		killed;					// Has a pattern executed a \k ?

	std::map<std::wstring, std::wstring> variables;       // variables for $SET and $GET
	
	HeadPairList	outHeaders;			// Outgoing headers
	HeadPairList	inHeaders;			// Incoming headers
	std::string		fileType;           // useable by $TYPE

	// Header operation
	std::string		GetOutHeader(const std::string& name) { return GetHeader(outHeaders, name); }
	std::string		GetInHeader(const std::string& name) { return GetHeader(inHeaders, name); }
	void			SetOutHeader(const std::string& name, const std::string& value) { SetHeader(outHeaders, name, value); }
	void			SetInHeader(const std::string& name, const std::string& value) { SetHeader(inHeaders, name, value); }
	void			RemoveOutHeader(const std::string& name) { RemoveHeader(outHeaders, name); }
	void			RemoveInHeader(const std::string& name) { RemoveHeader(inHeaders, name); }

	static std::string	GetHeader(const HeadPairList& headers, const std::string& name);
	static void			SetHeader(HeadPairList& headers, const std::string& name, const std::string& value);
	static void			RemoveHeader(HeadPairList& headers, const std::string& name);
	static void			CleanHeader(HeadPairList& headers);
};






