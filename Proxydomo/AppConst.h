/**
*	@file	AppConst.h
*	@brief	アプリケーション固有の設定など
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

/// アプリケーションの名前
#ifndef _DEBUG
	#ifndef _WIN64
	#define	APP_NAME	_T("Proxydomo")
	#else
	#define APP_NAME	_T("Proxydomo64")
	#endif
#else
	#ifndef _WIN64
	#define APP_NAME	_T("Proxydomo_debug")
	#else
	#define APP_NAME	_T("Proxydomo64_debug")
	#endif
#endif

/// アプリケーションのバージョン
#define APP_VERSION	_T("1.97")










