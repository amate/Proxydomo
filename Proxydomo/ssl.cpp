/**
*	@file	ssl.cpp
*	@brief	ssl misc
*/

#include "stdafx.h"
#include "ssl.h"
#include <fstream>
#include <vector>
#include <random>
#include <unordered_map>
#include <memory>
#include <chrono>

#include <wolfssl\ssl.h>
#include <wolfssl\wolfcrypt\asn_public.h>

#include <wincrypt.h>
#pragma comment (lib, "crypt32.lib")

#include <atlbase.h>
#include <atlsync.h>
#include <atlwin.h>
#include "Misc.h"
#include "Logger.h"
#include "resource.h"
#include "UITranslator.h"
using namespace UITranslator;

namespace {

LPCSTR	kCAFileName = "ca.pem.crt";
LPCSTR	kCAKeyFileName = "ca-key.pem";
LPCSTR	kCAEccKeyFileName = "ca-ecckey.pem";

enum { 
	kBuffSize = 1024 * 4,

	kRSA1024Keybit = 1024,

};

WOLFSSL_CTX*	g_sslclientCtx = nullptr;

std::vector<byte>	g_derCA;
struct CAKey {
	enum {
		kRsaKey,
		kEccKey,
	} keyType;

	union {
		RsaKey	rsaKey;
		ecc_key	eccKey;
	} key;
};
CAKey				g_caKey;

std::unordered_map<std::string, bool>	g_mapHostAllowOrDeny;
CCriticalSection						g_csmapHost;

struct serverCertAndKey {
	std::vector<byte>	derCert;
	std::vector<byte>	derkey;

	serverCertAndKey() : derCert(kBuffSize), derkey(kBuffSize) {}
};

std::unordered_map<std::string, std::unique_ptr<serverCertAndKey>>	g_mapHostServerCert;
CCriticalSection	g_csmapHostServerCert;


////////////////////////////////////////////////////////////////////////
// CCertificateErrorDialog

class CCertificateErrorDialog : public CDialogImpl<CCertificateErrorDialog>
{
public:
	enum { IDD = IDD_CERTIFICATEERROR };

	CCertificateErrorDialog(const std::string& host, const CString& errorMsg) : 
		m_host(host), m_errorMsg(errorMsg) {}

	BEGIN_MSG_MAP(CCertificateErrorDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_BTN_PERMANENTDENY	, OnCommand)
		COMMAND_ID_HANDLER(IDC_BTN_PERMANENTALLOW	, OnCommand)
		COMMAND_ID_HANDLER(IDC_BTN_TEMPDENY			, OnCommand)
		COMMAND_ID_HANDLER(IDC_BTN_TEMPALLOW		, OnCommand)
	END_MSG_MAP()

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		SetWindowText(GetTranslateMessage(IDD_CERTIFICATEERROR).c_str());

		ChangeControlTextForTranslateMessage(m_hWnd, IDC_STATIC_WARNING);
		ChangeControlTextForTranslateMessage(m_hWnd, IDC_STATIC_CONNECTIONCONTINUE);
		ChangeControlTextForTranslateMessage(m_hWnd, IDC_BTN_PERMANENTDENY);
		ChangeControlTextForTranslateMessage(m_hWnd, IDC_BTN_PERMANENTALLOW);
		ChangeControlTextForTranslateMessage(m_hWnd, IDC_BTN_TEMPDENY);
		ChangeControlTextForTranslateMessage(m_hWnd, IDC_BTN_TEMPALLOW);


		GetDlgItem(IDC_STATIC_DETAIL).SetWindowText(GetTranslateMessage(IDC_STATIC_DETAIL, (LPCWSTR)m_errorMsg).c_str());
		GetDlgItem(IDC_STATIC_HOST).SetWindowText(GetTranslateMessage(IDC_STATIC_HOST, (LPWSTR)CA2W(m_host.c_str())).c_str());
		return TRUE;
	}

	LRESULT OnCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		switch (wID) {
		case IDC_BTN_PERMANENTDENY:
			g_mapHostAllowOrDeny[m_host] = false;
		case IDC_BTN_TEMPDENY:
			EndDialog(IDCANCEL);
			break;

		case IDC_BTN_PERMANENTALLOW:
			g_mapHostAllowOrDeny[m_host] = true;
		case IDC_BTN_TEMPALLOW:
			EndDialog(IDOK);
			break;
		}
		return 0;
	}
private:
	std::string	m_host;
	CString	m_errorMsg;
};



// ===========================================================


std::vector<byte>	LoadBinaryFile(const std::wstring& filePath)
{
	std::ifstream fs(filePath, std::ios::in | std::ios::binary);
	if (!fs)
		return std::vector<byte>();

	fs.seekg(0, std::ios::end);
	auto eofPos = fs.tellg();
	fs.clear();
	fs.seekg(0, std::ios::beg);
	size_t fileSize = (size_t)eofPos.seekpos();

	std::vector<byte> vec(fileSize);
	fs.read((char*)vec.data(), fileSize);

	return vec;
}

void	SaveBinaryFile(const std::wstring& filePath, const std::vector<byte>& data)
{
	std::ofstream fs(filePath, std::ios::out | std::ios::binary | std::ios::trunc);
	fs.write((const char*)data.data(), data.size());
	fs.close();
}


int myVerify(int preverify, WOLFSSL_X509_STORE_CTX* store)
{
	(void)preverify;
	char buffer[WOLFSSL_MAX_ERROR_SZ];

	printf("In verification callback, error = %d, %s\n", store->error,
		wolfSSL_ERR_error_string(store->error, buffer));

	printf("Subject's domain name is %s\n", store->domain);

	{
		CCritSecLock lock(g_csmapHost);
		std::string host = store->domain;
		auto itfound = g_mapHostAllowOrDeny.find(host);
		if (itfound != g_mapHostAllowOrDeny.end()) {
			if (itfound->second) {
				return 1;	// allow
			} else {
				return 0;	// deny
			}
		}

		CCertificateErrorDialog crtErrorDlg(host, buffer);
		if (crtErrorDlg.DoModal() == IDOK) {
			return 1;	// allow
		} else {
			return 0;	// deny
		}
	}

	//printf("Allowing to continue anyway (shouldn't do this, EVER!!!)\n");
	return 0;
}

bool	LoadSystemTrustCA(WOLFSSL_CTX* ctx)
{
	auto funcAddCA = [ctx](HCERTSTORE store) -> bool {
		if (store == NULL)
			return false;

		PCCERT_CONTEXT crtcontext = CertEnumCertificatesInStore(store, NULL);
		while (crtcontext) {
			if (crtcontext->dwCertEncodingType == X509_ASN_ENCODING) {
				int ret = wolfSSL_CTX_load_verify_buffer(ctx, crtcontext->pbCertEncoded, crtcontext->cbCertEncoded, SSL_FILETYPE_ASN1);
				if (ret != SSL_SUCCESS) {
					CString crtName;
					CertGetNameString(crtcontext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, 0, crtName.GetBuffer(256), 256);
					crtName.ReleaseBuffer();
					ATLTRACE(L"wolfSSL_CTX_load_verify_buffer failed: %s\n", (LPCWSTR)crtName);
				}
			}
			crtcontext = CertEnumCertificatesInStore(store, crtcontext);
		}
		return true;
	};

	if (funcAddCA(CertOpenSystemStoreW(0, L"ROOT")) == false)
		return false;
	if (funcAddCA(CertOpenSystemStoreW(0, L"CA")) == false)
		return false;

	return true;
}


std::string CreateWildcardHost(const std::string& host)
{
	std::string wildcardHost;
	auto dotPos = host.find('.');
	if (dotPos == std::string::npos)
		throw std::runtime_error("CreateWildcardHost failed");

	wildcardHost = "*" + host.substr(dotPos);
	if (wildcardHost.length() >= CTC_NAME_SIZE) {
		ERROR_LOG << L"CreateWildcardHost : ワイルドカードホスト名の生成に失敗[" << host << L"]";
		throw std::runtime_error("CreateWildcardHost failed");
	}
	return wildcardHost;
}


std::unique_ptr<serverCertAndKey>	CreateServerCert(const std::string& host)
{
	auto certAndKey = std::make_unique<serverCertAndKey>();
	int ret = 0;

	Cert serverCert = {};
	wc_InitCert(&serverCert);
	::strcpy_s(serverCert.subject.org, "Proxydomo TLS Server");
	if (host.size() < CTC_NAME_SIZE) {
		::strcpy_s(serverCert.subject.commonName, host.c_str());
	} else {
		std::string wildcardHost = CreateWildcardHost(host);
		::strcpy_s(serverCert.subject.commonName, wildcardHost.c_str());
	}

	if (g_caKey.keyType == CAKey::kRsaKey) {
		serverCert.sigType = CTC_SHA256wRSA;

		ret = wc_SetIssuerBuffer(&serverCert, g_derCA.data(), (int)g_derCA.size());
		ATLASSERT(ret == 0);

		RsaKey	key;
		wc_InitRsaKey(&key, 0);
		RNG    rng;
		wc_InitRng(&rng);
		wc_MakeRsaKey(&key, kRSA1024Keybit, 65537, &rng);

		ret = wc_RsaKeyToDer(&key, certAndKey->derkey.data(), kBuffSize);
		if (ret < 0) {
			wc_FreeRng(&rng);
			wc_FreeRsaKey(&key);
			throw std::runtime_error("wc_RsaKeyToDer failed");
		}
		certAndKey->derkey.resize(ret);
		certAndKey->derkey.shrink_to_fit();

		ret = wc_MakeCert(&serverCert, certAndKey->derCert.data(), kBuffSize, &key, nullptr, &rng);
		if (ret < 0) {
			wc_FreeRng(&rng);
			wc_FreeRsaKey(&key);
			throw std::runtime_error("wc_MakeCert failed");
		}

		ret = wc_SignCert(serverCert.bodySz, CTC_SHA256wRSA, certAndKey->derCert.data(), kBuffSize, &g_caKey.key.rsaKey, nullptr, &rng);
		if (ret < 0) {
			wc_FreeRng(&rng);
			wc_FreeRsaKey(&key);
			throw std::runtime_error("wc_SignCert failed");
		}
		certAndKey->derCert.resize(ret);
		certAndKey->derCert.shrink_to_fit();

		wc_FreeRng(&rng);
		wc_FreeRsaKey(&key);

	} else {
		serverCert.sigType = CTC_SHA256wECDSA;

		ret = wc_SetIssuerBuffer(&serverCert, g_derCA.data(), (int)g_derCA.size());
		ATLASSERT(ret == 0);

		ecc_key	key;
		wc_ecc_init(&key);
		RNG    rng;
		wc_InitRng(&rng);
		ret = wc_ecc_make_key(&rng, 32, &key);
		ATLASSERT(ret == MP_OKAY);

		ret = wc_EccKeyToDer(&key, certAndKey->derkey.data(), kBuffSize);
		if (ret < 0) {
			wc_FreeRng(&rng);
			wc_ecc_free(&key);
			throw std::runtime_error("wc_RsaKeyToDer failed");
		}
		certAndKey->derkey.resize(ret);
		certAndKey->derkey.shrink_to_fit();

		ret = wc_MakeCert(&serverCert, certAndKey->derCert.data(), kBuffSize, nullptr, &key, &rng);
		if (ret < 0) {
			wc_FreeRng(&rng);
			wc_ecc_free(&key);
			throw std::runtime_error("wc_MakeCert failed");
		}

		ret = wc_SignCert(serverCert.bodySz, CTC_SHA256wECDSA, certAndKey->derCert.data(), kBuffSize, nullptr, &g_caKey.key.eccKey, &rng);
		if (ret < 0) {
			wc_FreeRng(&rng);
			wc_ecc_free(&key);
			throw std::runtime_error("wc_SignCert failed");
		}
		certAndKey->derCert.resize(ret);
		certAndKey->derCert.shrink_to_fit();

		wc_FreeRng(&rng);
		wc_ecc_free(&key);
	}

	return certAndKey;
}


}	// namespace


bool	InitSSL()
{
	CString cafile = Misc::GetExeDirectory() + kCAFileName;
	CString rsacaKeyfile = Misc::GetExeDirectory() + kCAKeyFileName;
	CString ecccaKeyfile = Misc::GetExeDirectory() + kCAEccKeyFileName;
	if ( ::PathFileExists(cafile) == FALSE || 
		(::PathFileExists(rsacaKeyfile) == FALSE && ::PathFileExists(ecccaKeyfile) == FALSE)) {
		INFO_LOG << L"CA証明書がないのでSSLはフィルタリングできません。";
		return false;
	}

	int ret = wolfSSL_Init();
	if (ret != SSL_SUCCESS) {
		ERROR_LOG << L"wolfSSL_Init failed";
		return false;
	}

	try {

		{	// server init
			auto pemCA = LoadBinaryFile((LPCWSTR)cafile);
			auto pemCAkey = LoadBinaryFile((LPCWSTR)rsacaKeyfile);
			if (pemCAkey.size()) {
				g_caKey.keyType = CAKey::kRsaKey;
			} else {
				g_caKey.keyType = CAKey::kEccKey;
				pemCAkey = LoadBinaryFile((LPCWSTR)ecccaKeyfile);
			}

			g_derCA.resize(kBuffSize);
			ret = wolfSSL_CertPemToDer(pemCA.data(), (int)pemCA.size(), g_derCA.data(), kBuffSize, CA_TYPE);
			if (ret < 0)
				throw std::runtime_error("wolfSSL_CertPemToDer failed");
			g_derCA.resize(ret);
			g_derCA.shrink_to_fit();

			std::vector<byte> derCAkey(kBuffSize);
			ret = wolfSSL_KeyPemToDer(pemCAkey.data(), (int)pemCAkey.size(), derCAkey.data(), kBuffSize, nullptr);
			if (ret < 0)
				throw std::runtime_error("wolfSSL_KeyPemToDer failed");
			derCAkey.resize(ret);

			if (g_caKey.keyType == CAKey::kRsaKey) {
				wc_InitRsaKey(&g_caKey.key.rsaKey, 0);
				word32 idx = 0;
				ret = wc_RsaPrivateKeyDecode(derCAkey.data(), &idx, &g_caKey.key.rsaKey, (word32)derCAkey.size());
				ATLASSERT(ret == 0);
				if (ret != 0) {
					wc_FreeRsaKey(&g_caKey.key.rsaKey);
					throw std::runtime_error("wc_RsaPrivateKeyDecode failed");
				}
			} else {
				wc_ecc_init(&g_caKey.key.eccKey);
				word32 idx = 0;
				ret = wc_EccPrivateKeyDecode(derCAkey.data(), &idx, &g_caKey.key.eccKey, (word32)derCAkey.size());
				ATLASSERT(ret == 0);
				if (ret != 0) {
					wc_ecc_free(&g_caKey.key.eccKey);
					throw std::runtime_error("wc_EccPrivateKeyDecode failed");
				}				
			}
		}

		{	// client init
			WOLFSSL_METHOD* method = wolfSSLv23_client_method();
			if (method == nullptr) {
				throw std::runtime_error("wolfSSLv23_client_method failed");
			}
			g_sslclientCtx = wolfSSL_CTX_new(method);
			if (g_sslclientCtx == nullptr) {
				throw std::runtime_error("wolfSSL_CTX_new failed");
			}

			wolfSSL_CTX_set_verify(g_sslclientCtx, SSL_VERIFY_PEER, myVerify);

			if (LoadSystemTrustCA(g_sslclientCtx) == false) {
				throw std::runtime_error("LoadSystemTrustCA failed");
			}

			ret = wolfSSL_CTX_UseSessionTicket(g_sslclientCtx);
			ATLASSERT(ret == SSL_SUCCESS);

			ret = wolfSSL_CTX_EnableOCSP(g_sslclientCtx, WOLFSSL_OCSP_CHECKALL);
			ATLASSERT(ret == SSL_SUCCESS);

			ret = wolfSSL_CTX_UseSupportedCurve(g_sslclientCtx, WOLFSSL_ECC_SECP256R1);
			ATLASSERT(ret == SSL_SUCCESS);
			ret = wolfSSL_CTX_UseSupportedCurve(g_sslclientCtx, WOLFSSL_ECC_SECP384R1);
			ATLASSERT(ret == SSL_SUCCESS);
			ret = wolfSSL_CTX_UseSupportedCurve(g_sslclientCtx, WOLFSSL_ECC_SECP521R1);
			ATLASSERT(ret == SSL_SUCCESS);

		}
		return true;

	} catch (std::exception& e) {
		if (g_sslclientCtx) {
			wolfSSL_CTX_free(g_sslclientCtx);
			g_sslclientCtx = nullptr;
		}
		wolfSSL_Cleanup();

		ERROR_LOG << L"InitSSL failed : " << (LPWSTR)CA2W(e.what());
		return false;
	}	
}

void	TermSSL()
{
	if (g_sslclientCtx) {
		wolfSSL_CTX_free(g_sslclientCtx);
		g_sslclientCtx = nullptr;
	}
	wolfSSL_Cleanup();
}


// CA証明書を生成する
void	GenerateCACertificate(bool rsa)
{
	Cert caCert = {};
	wc_InitCert(&caCert);
	::strcpy_s(caCert.subject.commonName, "Proxydomo CA");
	caCert.daysValid = 365;
	caCert.isCA = 1;

	if (rsa) {
		caCert.sigType = CTC_SHA256wRSA;

		RsaKey	cakey;
		wc_InitRsaKey(&cakey, 0);
		RNG    rng;
		wc_InitRng(&rng);
		wc_MakeRsaKey(&cakey, kRSA1024Keybit, 65537, &rng);

		std::vector<byte> derCA(kBuffSize);
		int ret = wc_MakeSelfCert(&caCert, derCA.data(), kBuffSize, &cakey, &rng);
		if (ret < 0) {
			wc_FreeRng(&rng);
			wc_FreeRsaKey(&cakey);
			throw std::runtime_error("wc_MakeSelfCert failed");
		}
		derCA.resize(ret);

		std::vector<byte> pemCA(kBuffSize);
		ret = wc_DerToPem(derCA.data(), (word32)derCA.size(), pemCA.data(), kBuffSize, CERT_TYPE);
		if (ret < 0) {
			wc_FreeRng(&rng);
			wc_FreeRsaKey(&cakey);
			throw std::runtime_error("wc_DerToPem failed");
		}
		pemCA.resize(ret);

		std::vector<byte> derkey(kBuffSize);
		ret = wc_RsaKeyToDer(&cakey, derkey.data(), kBuffSize);
		if (ret < 0) {
			wc_FreeRng(&rng);
			wc_FreeRsaKey(&cakey);
			throw std::runtime_error("wc_RsaKeyToDer failed");
		}
		derkey.resize(ret);

		std::vector<byte> pemkey(kBuffSize);
		ret = wc_DerToPem(derkey.data(), (word32)derkey.size(), pemkey.data(), kBuffSize, PRIVATEKEY_TYPE);
		if (ret < 0) {
			wc_FreeRng(&rng);
			wc_FreeRsaKey(&cakey);
			throw std::runtime_error("wc_DerToPem failed");
		}
		pemkey.resize(ret);

		wc_FreeRng(&rng);
		wc_FreeRsaKey(&cakey);

		DeleteFileW(static_cast<LPCWSTR>(Misc::GetExeDirectory() + kCAEccKeyFileName));

		SaveBinaryFile(static_cast<LPCWSTR>(Misc::GetExeDirectory() + kCAFileName), pemCA);
		SaveBinaryFile(static_cast<LPCWSTR>(Misc::GetExeDirectory() + kCAKeyFileName), pemkey);

	} else {
		caCert.sigType = CTC_SHA256wECDSA;

		ecc_key	cakey;
		wc_ecc_init(&cakey);
		RNG    rng;
		wc_InitRng(&rng);
		wc_ecc_make_key(&rng, 32, &cakey);

		std::vector<byte> derCA(kBuffSize);
		int ret = wc_MakeCert(&caCert, derCA.data(), kBuffSize, nullptr, &cakey, &rng);
		if (ret < 0) {
			wc_FreeRng(&rng);
			wc_ecc_free(&cakey);
			throw std::runtime_error("wc_MakeCert failed");
		}
		ret = wc_SignCert(caCert.bodySz, caCert.sigType, derCA.data(), kBuffSize, nullptr, &cakey, &rng);
		if (ret < 0) {
			wc_FreeRng(&rng);
			wc_ecc_free(&cakey);
			throw std::runtime_error("wc_SignCert failed");
		}
		derCA.resize(ret);

		std::vector<byte> pemCA(kBuffSize);
		ret = wc_DerToPem(derCA.data(), (word32)derCA.size(), pemCA.data(), kBuffSize, CERT_TYPE);
		if (ret < 0) {
			wc_FreeRng(&rng);
			wc_ecc_free(&cakey);
			throw std::runtime_error("wc_DerToPem failed");
		}
		pemCA.resize(ret);

		std::vector<byte> derkey(kBuffSize);
		ret = wc_EccKeyToDer(&cakey, derkey.data(), kBuffSize);
		if (ret < 0) {
			wc_FreeRng(&rng);
			wc_ecc_free(&cakey);
			throw std::runtime_error("wc_EccKeyToDer failed");
		}
		derkey.resize(ret);

		std::vector<byte> pemkey(kBuffSize);
		ret = wc_DerToPem(derkey.data(), (word32)derkey.size(), pemkey.data(), kBuffSize, ECC_PRIVATEKEY_TYPE);
		if (ret < 0) {
			wc_FreeRng(&rng);
			wc_ecc_free(&cakey);
			throw std::runtime_error("wc_DerToPem failed");
		}
		pemkey.resize(ret);

		wc_FreeRng(&rng);
		wc_ecc_free(&cakey);

		DeleteFileW(static_cast<LPCWSTR>(Misc::GetExeDirectory() + kCAKeyFileName));

		SaveBinaryFile(static_cast<LPCWSTR>(Misc::GetExeDirectory() + kCAFileName), pemCA);
		SaveBinaryFile(static_cast<LPCWSTR>(Misc::GetExeDirectory() + kCAEccKeyFileName), pemkey);
	}
}


////////////////////////////////////////////////////////////////
// CSSLSession

CSSLSession::CSSLSession() : 
	m_sock(nullptr), m_ctx(nullptr), m_ssl(nullptr),
	m_nLastReadCount(0), m_nLastWriteCount(0)
{
	m_writeStop = false;
}

CSSLSession::~CSSLSession()
{
	Close();
}


std::unique_ptr<CSSLSession>	CSSLSession::InitClientSession(CSocket* sock, const std::string& host)
{
	auto session = std::make_unique<CSSLSession>();
	int ret = 0;

	session->m_ssl = wolfSSL_new(g_sslclientCtx);
	if (session->m_ssl == nullptr)
		throw std::runtime_error("wolfSSL_new failed");

	wolfSSL_set_fd(session->m_ssl, (int)sock->GetSocket());

	ret = wolfSSL_UseSNI(session->m_ssl, WOLFSSL_SNI_HOST_NAME, (const void*)host.c_str(), (unsigned short)host.length());
	ATLASSERT(ret == SSL_SUCCESS);

	ret = wolfSSL_UseSecureRenegotiation(session->m_ssl);
	ATLASSERT(ret == SSL_SUCCESS);

	ret = wolfSSL_check_domain_name(session->m_ssl, host.c_str());
	ATLASSERT(ret == SSL_SUCCESS);

	sock->SetBlocking(true);
	ret = wolfSSL_connect(session->m_ssl);
	if (ret != SSL_SUCCESS) {
		int err = wolfSSL_get_error(session->m_ssl, 0);
		char buffer[WOLFSSL_MAX_ERROR_SZ];
		ATLTRACE("error = %d, %s\n", err, wolfSSL_ERR_error_string(err, buffer));

		wolfSSL_free(session->m_ssl);
		session->m_ssl = nullptr;
		return nullptr;
	}

	wolfSSL_set_using_nonblock(session->m_ssl, 1);
	sock->SetBlocking(false);

	session->m_sock = sock;
	return session;
}

std::unique_ptr<CSSLSession> CSSLSession::InitServerSession(CSocket* sock, const std::string& host)
{
	auto session = std::make_unique<CSSLSession>();
	int ret = 0;

	serverCertAndKey* certAndKey = nullptr;
	{
		std::string findHost = host;
		if (host.size() >= CTC_NAME_SIZE) {
			findHost = CreateWildcardHost(host);
		}

		CCritSecLock lock(g_csmapHostServerCert);
		auto itfound = g_mapHostServerCert.find(findHost);
		if (itfound != g_mapHostServerCert.end()) {
			certAndKey = itfound->second.get();
		} else {
			auto serverCertAndKey = CreateServerCert(host);
			certAndKey = serverCertAndKey.get();
			g_mapHostServerCert[findHost] = std::move(serverCertAndKey);
		}

	}

	WOLFSSL_METHOD* method = wolfSSLv23_server_method();
	if (method == nullptr) {
		throw std::runtime_error("wolfSSLv23_server_method failed");
	}
	session->m_ctx = wolfSSL_CTX_new(method);
	if (session->m_ctx == nullptr) {
		throw std::runtime_error("wolfSSL_CTX_new failed");
	}

	ret = wolfSSL_CTX_use_certificate_buffer(session->m_ctx, certAndKey->derCert.data(), (long)certAndKey->derCert.size(), SSL_FILETYPE_ASN1);
	ATLASSERT(ret == SSL_SUCCESS);

	ret = wolfSSL_CTX_use_PrivateKey_buffer(session->m_ctx, certAndKey->derkey.data(), (long)certAndKey->derkey.size(), SSL_FILETYPE_ASN1);
	ATLASSERT(ret == SSL_SUCCESS);

	session->m_ssl = wolfSSL_new(session->m_ctx);
	if (session->m_ssl == nullptr) {
		wolfSSL_CTX_free(session->m_ctx);
		session->m_ctx = nullptr;
		throw std::runtime_error("wolfSSL_new failed");
	}

	wolfSSL_set_fd(session->m_ssl, (int)sock->GetSocket());

	sock->SetBlocking(true);
	ret = wolfSSL_accept(session->m_ssl);
	if (ret != SSL_SUCCESS) {
		int err = wolfSSL_get_error(session->m_ssl, 0);
		char buffer[WOLFSSL_MAX_ERROR_SZ];
		ATLTRACE("error = %d, %s\n", err, wolfSSL_ERR_error_string(err, buffer));
		INFO_LOG << L"wolfSSL_accept failed [" << err << L"] : " << (LPWSTR)CA2W(buffer);

		wolfSSL_free(session->m_ssl);
		session->m_ssl = nullptr;
		wolfSSL_CTX_free(session->m_ctx);
		session->m_ctx = nullptr;
		return nullptr;
	}

	wolfSSL_set_using_nonblock(session->m_ssl, 1);
	sock->SetBlocking(false);

	session->m_sock = sock;
	return session;
}


bool	CSSLSession::Read(char* buffer, int length)
{
	if (m_ssl == nullptr)
		return false;

	m_nLastReadCount = 0;
	int ret = wolfSSL_read(m_ssl, (void*)buffer, length);
	if (ret == 0) {
		Close();
		return true;

	} else {
		int error = wolfSSL_get_error(m_ssl, 0);
		if (ret == SSL_FATAL_ERROR && error == SSL_ERROR_WANT_READ) {
			return false;	// pending
		}
		if (ret < 0) {
			Close();
			return false;

		} else {
			m_nLastReadCount = ret;
			return true;
		}
	}
}

bool	CSSLSession::Write(const char* buffer, int length)
{
	if (m_ssl == nullptr)
		return false;

	m_nLastWriteCount = 0;
	int ret = 0;
	int error = 0;
	for (;;) {
		ret = wolfSSL_write(m_ssl, (const void*)buffer, length);
		error = wolfSSL_get_error(m_ssl, 0);
		if (ret == SSL_FATAL_ERROR && error == SSL_ERROR_WANT_WRITE && m_writeStop == false) {
			::Sleep(50);
		} else {
			break;
		}
	}

	if (ret != length) {
		Close();
		return false;

	} else {
		m_nLastWriteCount = ret;
		return true;
	}
}


void	CSSLSession::Close()
{
	if (m_ssl == nullptr || m_sock == nullptr)
		return;

	int ret = wolfSSL_shutdown(m_ssl);
	try {
		m_sock->Close();
	}
	catch (SocketException& e) {
		e;
	}
	m_sock = nullptr;

	wolfSSL_free(m_ssl);
	m_ssl = nullptr;
	if (m_ctx) {
		wolfSSL_CTX_free(m_ctx);
		m_ctx = nullptr;
	}
}









