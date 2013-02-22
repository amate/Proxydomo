/**
*	@file	DataReceptor.h
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

/* class IDataReceptor
 * This virtual class is for classes that can receive data.
 * It allows chaining text processors. The last CDataReceptor of the
 * chain is not a text processor but the request manager itself.
 */
class IDataReceptor {

public:
    virtual ~IDataReceptor() { }

    virtual void DataReset() = 0;
    virtual void DataFeed(const std::string& data) = 0;
    virtual void DataDump() = 0;
};
