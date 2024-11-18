/**
*	@file	RequestManager.h
*	@brief	�u���E�U�̃v���N�V�̃T�[�o�[�Ԃ̏������󂯎���
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

#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <atomic>
#include "Socket.h"
#include "DataReceptor.h"
#include "TextBuffer.h"
#include "FilterOwner.h"
#include "proximodo\url.h"
#include "proximodo\headerfilter.h"
#include "ssl.h"
#include "Decompressor.h"


// �O���錾
namespace Proxydomo { class CMatcher; }
struct ConnectionData;


// Processing steps (incoming or outgoing)
enum class STEP {
	STEP_START,     // marks the beginning of a request/response
	STEP_FIRSTLINE, // read GET/POST line, or status line
	STEP_HEADERS,   // read headers
	STEP_DECODE,    // check and filter headers
	STEP_CHUNK,     // read chunk size / trailers
	STEP_RAW,       // read raw message body
	STEP_TUNNELING, // read data until disconnection
	STEP_FINISH,	// marks the end of a request/response
};
inline const wchar_t* STEPtoString(STEP step);

class CRequestManager : public IDataReceptor
{
public:
	CRequestManager(std::unique_ptr<CSocket>&& psockBrowser);
	~CRequestManager();

	void	Manage();

	bool	IsValid() const { return m_valid; }
	void	SwitchToInvalid();

private:
	void	_ReloadHeaderFilters();

	// �u���E�U �� Proxy(this) �� �T�C�g
	bool	_ReceiveOut();
	std::string	m_recvOutBuf;

	// �u���E�U �� Proxy(this) �� �T�C�g
	void	_ProcessOut();
	std::string m_sendOutBuf;	/// �T�C�g�ɑ���f�[�^
	bool	_SendOut();			/// Send outgoing data to website

	
	void	_ProcessOutHeaderFilter();	// Filter outgoing headers
	void	_ProcessInHeaderFilter();	// Filter incoming headers

	void	_ConnectWebsite();

	bool	_HandleLocalPtron();
	bool	_HandleRedirectToHost();
	void	_SendConnectRequestToRemoteProxy(std::string& name);

	// �T�C�g����̃f�[�^����M
	bool	_ReceiveIn();
	std::string m_recvInBuf;

	// �T�C�g �� Proxy(this) �� �u���E�U
	void	_ProcessIn();
	bool	_SendIn();
	std::string m_sendInBuf;


	bool	_VerifyContentType(std::string& ctype);

	// IDataReceptor
	void	DataReset() { m_dumped = false; }
	void	DataFeed(const std::string& data);
	void	DataDump();

	void	_EndFeeding();

	void	_FakeResponse(const std::string& code, const std::wstring& filename = L"");

	void	_JudgeManageContinue();

	void	_AddTraceLog(LPCWSTR format, ...);
	std::wstring m_traceLog;

	// Constants
	enum { kReadBuffSize = 64 * 1024 };
	char m_readBuffer[kReadBuffSize];

	// Data members

	bool		m_bypass;	// for SSL filter
	// Filter instances
	bool		m_useChain;
	CTextBuffer	m_textFilterChain;
	std::vector<std::unique_ptr<CHeaderFilter>>	m_vecpInFilter;
	std::vector<std::unique_ptr<CHeaderFilter>>	m_vecpOutFilter;
	std::chrono::steady_clock::time_point	m_lastFiltersEnumTime;

	// Sockets
	std::shared_ptr<SocketIF>	m_psockBrowser;
	std::shared_ptr<SocketIF>	m_psockWebsite;
	std::wstring	m_previousHost;
	std::string		m_previousConnectRequest;	// for SSL & $SETPROXY

	// �����̊J�n�҂�����
	std::chrono::steady_clock::time_point	m_processIdleTime;	

	std::atomic_bool	m_valid;
	bool	m_dumped;

	int		m_redirectedIn;    // number of redirections by incoming headers

	IPv4Address	m_ipFromAddress;
	// for recv events only
	std::string m_logRequest;
	std::string m_logResponse;
	bool		m_bPostData;
	std::string	m_logPostData;

	// for correct incoming body processing
    bool	m_recvConnectionClose; // should the connection be closed when finished
    bool	m_sendConnectionClose;

	enum class ContentEncoding {
		kNone = 0,	// plain/compress
		kGzip,
		kDeflate,
		kBrotli,
		kZstd,
	};
	ContentEncoding		m_recvContentCoding;
	std::unique_ptr<IDecompressor>	m_decompressor;

    // Variables and functions for outgoing processing
    STEP	m_outStep;	// �u���E�U �� Proxy(this) �� �T�C�g �Ԃ̏����̏�Ԃ�����
	int64_t	m_outSize;
	bool	m_outChunked;
	struct { 
		std::string method, url, ver; 
	} m_requestLine;

	// Variables and functions for incoming processing
	STEP	m_inStep;	// �T�C�g �� Proxy(this) �� �u���E�U �Ԃ̏����̏�Ԃ�����
	int64_t	m_inSize;
	bool	m_inChunked;

	CFilterOwner	m_filterOwner;

	// for Debug
	int		m_RequestCountFromBrowser;	// �u���E�U����̃��N�G�X�g��(GET,HEAD�Ȃǂ̗v����)
	ConnectionData*	m_connectionData;
	std::string	m_requestManagerLog;

};


const wchar_t* STEPtoString(STEP step)
{
	switch (step) {
	case STEP::STEP_START:		return L"START";
	case STEP::STEP_FIRSTLINE:	return L"FIRSTLINE";
	case STEP::STEP_HEADERS:	return L"HEADERS";
	case STEP::STEP_DECODE:		return L"DECODE";
	case STEP::STEP_CHUNK:		return L"CHUNK";
	case STEP::STEP_RAW:		return L"RAW";
	case STEP::STEP_TUNNELING:	return L"TUNNELING";
	case STEP::STEP_FINISH:		return L"FINISH";
	default:	return L"err";
	}
}
