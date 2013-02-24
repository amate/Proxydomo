/**
*	@file	Log.h
*	@brief	ログクラス
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
#include "Socket.h"

enum LogProxyEvent
{
	kLogProxyNewRequest,	// ブラウザソケットが開かれました
	kLogProxyEndRequest,	// ブラウザソケットが閉じられました
};

enum LogHttpEvent
{
	kLogHttpRecvOut,	// Browserからデータを受信
	kLogHttpSendOut,	// Websiteへデータを送信
	kLogHttpPostOut,
	kLogHttpRecvIn,		// Websiteからデータを受信
	kLogHttpSendIn,		// Browserへデータを送信

};

enum LogFilterEvent
{
	kLogFilterHeaderMatch,
	kLogFilterHeaderReplace,
	kLogFilterTextMatch,
	kLogFilterTextReplace,
	kLogFilterLogCommand,
	kLogFilterJump,
	kLogFilterRdir,
};


class ILogTrace
{
public:
	virtual void ProxyEvent(LogProxyEvent Event, const IPv4Address& addr) = 0;
	virtual void HttpEvent(LogHttpEvent Event, const IPv4Address& addr, int RequestNumber, const std::string& text) = 0;
	virtual void FilterEvent(LogFilterEvent Event, int RequestNumber, const std::string& title, const std::string& text) = 0;
};




class CLog
{
public:
	static void	RegisterLogTrace(ILogTrace* pTrace) { s_pLogTrace = pTrace; }
	static void	RemoveLogTrace() { s_pLogTrace = nullptr; }

	static void ProxyEvent(LogProxyEvent Event, const IPv4Address& addr)
	{
		if (s_pLogTrace)
			s_pLogTrace->ProxyEvent(Event, addr);
	}
	static void HttpEvent(LogHttpEvent Event, const IPv4Address& addr, int RequestNumber, const std::string& text)
	{
		if (s_pLogTrace)
			s_pLogTrace->HttpEvent(Event, addr, RequestNumber, text);
	}
	static void FilterEvent(LogFilterEvent Event, int RequestNumber, const std::string& title, const std::string& text)
	{
		if (s_pLogTrace)
			s_pLogTrace->FilterEvent(Event, RequestNumber, title, text);
	}

	static long	IncrementRequestCount() { return ::InterlockedIncrement(&s_RequestCount); }

	static long GetActiveRequestCount() { return s_ActiveRequestCount; }
	static long	IncrementActiveRequestCount() { return ::InterlockedIncrement(&s_ActiveRequestCount); }
	static long	DecrementActiveRequestCount() { return ::InterlockedDecrement(&s_ActiveRequestCount); }


private:
	// 
	static ILogTrace*	s_pLogTrace;
	static long			s_RequestCount;			/// Total number of requests received since Proximodo started
	static long			s_ActiveRequestCount;	/// Number of requests being processed
};


__declspec(selectany) ILogTrace*	CLog::s_pLogTrace = nullptr;
__declspec(selectany) long			CLog::s_RequestCount = 0;
__declspec(selectany) long			CLog::s_ActiveRequestCount = 0;
















