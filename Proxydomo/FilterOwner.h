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
#include <unordered_map>
#include "proximodo\url.h"

/* class CFilterOwner
 * This class only contains and implements what is common to
 * filter owners (such as CRequestManager and filter tester window).
 * URL and variables are such common members.
 */

typedef std::vector<std::pair<std::wstring, std::wstring>> HeadPairList;		// first:name  second:value

class CFilterOwner
{
public:
	CFilterOwner();

	void	Reset();

	CUrl		url;
	struct {
		std::string ver, code, msg;
	}			responseLine;			 // for WebFilterDebug
	std::string responseCode;            // response code from website
	long		requestNumber;

	//int		    cnxNumber;                   // number of the connection

	bool		useSettingsProxy;			 // can be overridden by $USEPROXY
	
	std::wstring contactHost;				 // can be overridden by $SETPROXY
    std::wstring rdirToHost;                 // set by $RDIR and $JUMP
	enum class RedirectMode {
		kNone,
		kJump,	// 302 response
		kRdir,	// transparent
	} rdirMode;

	bool		bypassIn;                    // tells if we can filter incoming headers
	bool		bypassOut;                   // tells if we can filter outgoing headers
	bool		bypassBody;                  // will tell if we can filter the body or not
	bool		bypassBodyForced;            // set to true if $FILTER changed bypassBody

	bool		killed;					// Has a pattern executed a \k ?

	std::unordered_map<std::wstring, std::wstring> variables;       // variables for $SET and $GET
	
	HeadPairList	outHeaders;			// Outgoing headers
	HeadPairList	outHeadersFiltered;	// Filtered Outgoingheaders
	HeadPairList	inHeaders;			// Incoming headers
	HeadPairList	inHeadersFiltered;	// Filtered Incoming headers
	std::string		fileType;           // useable by $TYPE

	// Header operation
	std::wstring	GetOutHeader(const std::wstring& name) { return GetHeader(outHeaders, name); }
	std::wstring	GetInHeader(const std::wstring& name) { return GetHeader(inHeaders, name); }
	void			SetOutHeader(const std::wstring& name, const std::wstring& value) { SetHeader(outHeaders, name, value); }
	void			SetInHeader(const std::wstring& name, const std::wstring& value) { SetHeader(inHeaders, name, value); }
	void			RemoveOutHeader(const std::wstring& name) { RemoveHeader(outHeaders, name); }
	void			RemoveInHeader(const std::wstring& name) { RemoveHeader(inHeaders, name); }

	static std::wstring	GetHeader(const HeadPairList& headers, const std::wstring& name);
	static void			SetHeader(HeadPairList& headers, const std::wstring& name, const std::wstring& value);
	static void			RemoveHeader(HeadPairList& headers, const std::wstring& name);
	static void			CleanHeader(HeadPairList& headers);
};






