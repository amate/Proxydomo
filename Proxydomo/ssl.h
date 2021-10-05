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

// CAØ–¾‘‚ğ¶¬‚·‚é
void	GenerateCACertificate();

class CSSLSession;

bool	ManageCertificateAPI(const std::string& url, SocketIF* sockBrowser);

// ‘O•ûéŒ¾
typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_st SSL;

///////////////////////////////////////////////////////
/// CSSLSession

class CSSLSession : public SocketIF
{
public:
	static std::shared_ptr<SocketIF> InitClientSession(std::shared_ptr<SocketIF> sockWebsite, const std::string& host,
															std::shared_ptr<SocketIF> sockBrowser, std::atomic_bool& valid);
	static std::shared_ptr<SocketIF> InitServerSession(std::shared_ptr<SocketIF> sockBrowser, const std::string& host,
															std::atomic_bool& valid);

	CSSLSession();
	~CSSLSession();

	bool	IsConnected() override { return m_sock.get() ? m_sock->IsConnected() : false; }
	bool	IsSecureSocket() override { return true; }
	SOCKET	GetSocket() override { return INVALID_SOCKET; }

	void	WriteStop() override { m_writeStop = true; }

	int		Read(char* buffer, int length) override;
	int		Write(const char* buffer, int length) override;

	void	Close() override;

private:
	std::shared_ptr<SocketIF>	m_sock;
	SSL_CTX*	m_ctx;
	SSL*		m_ssl;
	std::atomic_bool	m_writeStop;
};




