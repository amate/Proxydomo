/**
*	@file	ssl.h
*	@brief	ssl misc
*/

#pragma once

#include <memory>
#include <atomic>
#include "Socket.h"

bool	InitSSL();
void	TermSSL();

// CAèÿñæèëÇê∂ê¨Ç∑ÇÈ
void	GenerateCACertificate(bool rsa);

class CSSLSession;

bool	ManageCertificateAPI(const std::string& url, CSSLSession* sockBrowser);


struct WOLFSSL;
struct WOLFSSL_CTX;


class CSSLSession
{
public:
	static std::unique_ptr<CSSLSession>	InitClientSession(CSocket* sockWebsite, const std::string& host, CSocket* sockBrowser, std::atomic_bool& valid);
	static std::unique_ptr<CSSLSession> InitServerSession(CSocket* sockBrowser, const std::string& host, std::atomic_bool& valid);

	CSSLSession();
	~CSSLSession();

	bool	IsConnected() const { return m_sock ? m_sock->IsConnected() : false; }

	void	WriteStop() { m_writeStop = true; }

	bool	Read(char* buffer, int length);
	int		GetLastReadCount() const { return m_nLastReadCount; }

	bool	Write(const char* buffer, int length);
	int		GetLastWriteCount() const { return m_nLastWriteCount; }

	void	Close();

private:
	CSocket*	m_sock;
	WOLFSSL_CTX*	m_ctx;
	WOLFSSL*		m_ssl;
	int		m_nLastReadCount;
	int		m_nLastWriteCount;
	std::atomic_bool	m_writeStop;
};





































