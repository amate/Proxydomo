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
#include <vector>
#include <deque>
#include <atlsync.h>
#include "Socket.h"

enum LogProxyEvent
{
	kLogProxyNewRequest,	// ブラウザソケットが開かれました
	kLogProxyEndRequest,	// ブラウザソケットが閉じられました
};

enum LogHttpEvent
{
	kLogHttpNewRequest,
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
	kLogFilterListReload,
	kLogFilterListBadLine,
	kLogFilterListMatch,
};


class ILogTrace
{
public:
	virtual void ProxyEvent(LogProxyEvent Event, const IPv4Address& addr) = 0;
	virtual void HttpEvent(LogHttpEvent Event, const IPv4Address& addr, int RequestNumber, const std::string& text) = 0;
	virtual void FilterEvent(LogFilterEvent Event, int RequestNumber, const std::string& title, const std::string& text) = 0;
	virtual void AddNewRequest(long requestNumber) { };
};




class CLog
{
public:
	static void	RegisterLogTrace(ILogTrace* pTrace)
	{
		CCritSecLock	lock(s_csvecLogTrace);
		s_vecpLogTrace.push_back(pTrace);
	}
	static void	RemoveLogTrace(ILogTrace* pTrace)
	{
		CCritSecLock	lock(s_csvecLogTrace);
		for (auto it = s_vecpLogTrace.begin(); it != s_vecpLogTrace.end(); ++it) {
			if (*it == pTrace) {
				s_vecpLogTrace.erase(it);
				break;
			}
		}
	}

	static void ProxyEvent(LogProxyEvent Event, const IPv4Address& addr)
	{
		std::vector<ILogTrace*> vec;
		{
			CCritSecLock	lock(s_csvecLogTrace);
			vec =s_vecpLogTrace;
		}
		for (auto& trace : vec)
			trace->ProxyEvent(Event, addr);
	}
	static void HttpEvent(LogHttpEvent Event, const IPv4Address& addr, int RequestNumber, const std::string& text)
	{
		std::vector<ILogTrace*> vec;
		{
			CCritSecLock	lock(s_csvecLogTrace);
			vec =s_vecpLogTrace;
		}
		for (auto& trace : vec)
			trace->HttpEvent(Event, addr, RequestNumber, text);
	}
	static void FilterEvent(LogFilterEvent Event, int RequestNumber, const std::string& title, const std::string& text)
	{
		std::vector<ILogTrace*> vec;
		{
			CCritSecLock	lock(s_csvecLogTrace);
			vec =s_vecpLogTrace;
		}
		for (auto& trace : vec)
			trace->FilterEvent(Event, RequestNumber, title, text);
	}

	static long	IncrementRequestCount() { return ::InterlockedIncrement(&s_RequestCount); }

	static long GetActiveRequestCount() { return s_ActiveRequestCount; }
	static long	IncrementActiveRequestCount() { return ::InterlockedIncrement(&s_ActiveRequestCount); }
	static long	DecrementActiveRequestCount() { return ::InterlockedDecrement(&s_ActiveRequestCount); }

	enum { kMaxRecentURLCount = 200 };

	static void AddNewRequest(	long requestNumber, 
								const std::string& responseCode, 
								const std::string& contentType, 
								const std::string& contentLength, 
								const std::string& url,
								bool kill = false)
	{
		CCritSecLock	lock(s_csdeqRecentURLs);
		if (s_deqRecentURLs.size() >= kMaxRecentURLCount)
			s_deqRecentURLs.pop_back();

		std::string contentType2 = contentType;
		size_t colon = contentType.find(';');
		if (colon != std::string::npos)
			contentType2 = contentType.substr(0, colon);
		s_deqRecentURLs.emplace_front(requestNumber, responseCode, contentType2, contentLength, url, kill);

		std::vector<ILogTrace*> vec;
		{
			CCritSecLock	lock(s_csvecLogTrace);
			vec =s_vecpLogTrace;
		}
		for (auto& trace : vec)
			trace->AddNewRequest(requestNumber);
	}

	static CCriticalSection			s_csvecLogTrace;
	static CCriticalSection			s_csdeqRecentURLs;

	struct RecentURLData {
		long	requestNumber;
		std::string responseCode;
		std::string contentType;
		std::string contentLength;
		std::string url;
		bool	kill;

		RecentURLData(	long requestNumber, 
						const std::string& responseCode, 
						const std::string& contentType, 
						const std::string& contentLength, 
						const std::string& url,
						bool kill) : requestNumber(requestNumber), responseCode(responseCode), contentType(contentType), contentLength(contentLength), url(url), kill(kill)
		{ }
	};
	static std::deque<RecentURLData>	s_deqRecentURLs;

private:
	static std::vector<ILogTrace*>	s_vecpLogTrace;
	static long			s_RequestCount;			/// Total number of requests received since Proximodo started
	static long			s_ActiveRequestCount;	/// Number of requests being processed

};

__declspec(selectany) CCriticalSection			CLog::s_csvecLogTrace;
__declspec(selectany) std::vector<ILogTrace*>	CLog::s_vecpLogTrace;
__declspec(selectany) long			CLog::s_RequestCount = 0;
__declspec(selectany) long			CLog::s_ActiveRequestCount = 0;
__declspec(selectany) CCriticalSection			CLog::s_csdeqRecentURLs;
__declspec(selectany) std::deque<CLog::RecentURLData>	CLog::s_deqRecentURLs;














