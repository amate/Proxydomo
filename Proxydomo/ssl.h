/**
*	@file	ssl.h
*	@brief	ssl misc
*/

#pragma once

#include <memory>
#include "Socket.h"

bool	InitSSL();
void	TermSSL();

// CAèÿñæèëÇê∂ê¨Ç∑ÇÈ
void	GenerateCACertificate(bool rsa);


struct WOLFSSL;
struct WOLFSSL_CTX;


class CSSLSession
{
public:
	static std::unique_ptr<CSSLSession>	InitClientSession(CSocket* sock, const std::string& host);
	static std::unique_ptr<CSSLSession> InitServerSession(CSocket* sock, const std::string& host);

	CSSLSession();
	~CSSLSession();

	bool	IsConnected() const { return m_sock ? m_sock->IsConnected() : false; }

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
};





































