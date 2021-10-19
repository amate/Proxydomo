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
#include <boost\algorithm\string.hpp>

#include <wincrypt.h>
#pragma comment (lib, "crypt32.lib")

// openSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/pem.h>

#pragma comment(lib, "libcrypto.lib")
#pragma comment(lib, "libssl.lib")

#include <atlbase.h>
#include <atlsync.h>
#include <atlwin.h>
#include "Misc.h"
#include "Logger.h"
#include "resource.h"
#include "UITranslator.h"
using namespace UITranslator;
#include "WinHTTPWrapper.h"
#include "Matcher.h"
#include "FilterOwner.h"
#include "proximodo\filter.h"
#include "CodeConvert.h"
#include "proximodo\util.h"

namespace {

LPCSTR	kCAFileName = "ca.pem.crt";
LPCSTR	kCAKeyFileName = "ca-key.pem";
LPCSTR	kCAEccKeyFileName = "ca-ecckey.pem";

// =============================================
// OpenSSL Handle Wrapper

struct X509_Deleter {
	void operator()(X509* p) {
		X509_free(p);
	}
};

using X509_ptr = std::unique_ptr<X509, X509_Deleter>;

struct EVP_PKEY_Deleter {
	void operator()(EVP_PKEY* p) {
		EVP_PKEY_free(p);
	}
};

using EVP_PKEY_ptr = std::unique_ptr<EVP_PKEY, EVP_PKEY_Deleter>;

struct BIO_Deleter {
	void operator()(BIO* p) {
		BIO_free(p);
	}
};

using BIO_ptr = std::unique_ptr<BIO, BIO_Deleter>;

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// Proxydomo <=> Website 用の SSL_CTX
SSL_CTX*		g_openssl_client_ctx = nullptr;

// 証明書と秘密鍵の組み合わせ
struct CertAndKey {
	X509_ptr	 cert;
	EVP_PKEY_ptr privateKey;
};

// ProxydomoCA certification and private key
CertAndKey		g_CA_cert_key;

// 接続許可/否定ホスト
std::unordered_map<std::string, bool>	g_mapHostAllowOrDeny;

// 一時接続許可/否定ホスト
std::unordered_map<std::string, std::pair<std::chrono::steady_clock::time_point, bool>>	g_mapHostTempAllowOrDeny;

CCriticalSection	g_csmapHost;
int				g_loadCACount = 0;
std::string		g_lastnoCAHost;		// ? なにこれ

// ホスト用に生成したサーバー証明書
std::unordered_map<std::string, std::unique_ptr<CertAndKey>>	g_mapHostServerCert;
CCriticalSection	g_csmapHostServerCert;

// AllowSSLServerHost.txt
std::shared_ptr<Proxydomo::CMatcher>	g_pAllowSSLServerHostMatcher;

// ManageCertificateErrorPage

struct SSLCallbackContext
{
	std::string host;
	std::shared_ptr<SocketIF>	sockBrowser;

	SSLCallbackContext(const std::string& host, std::shared_ptr<SocketIF> sockBrowser) 
		: host(host), sockBrowser(sockBrowser) {}
};

int g_sslCallbackContextIndex = 0;

// 外部から操作されないための認証トークン
std::string	g_authentication;

bool	LoadSystemTrustCA();

////////////////////////////////////////////////////////////////////////
// CCertificateErrorDialog

class CCertificateErrorDialog : public CDialogImpl<CCertificateErrorDialog>
{
public:
	enum { IDD = IDD_CERTIFICATEERROR };

	CCertificateErrorDialog(const std::string& host, const CString& errorMsg, bool bNoCA) : 
		m_host(host), m_errorMsg(errorMsg), m_bNoCA(bNoCA) {}

	BEGIN_MSG_MAP(CCertificateErrorDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_BTN_PERMANENTDENY	, OnCommand)
		COMMAND_ID_HANDLER(IDC_BTN_PERMANENTALLOW	, OnCommand)
		COMMAND_ID_HANDLER(IDC_BTN_TEMPDENY			, OnCommand)
		COMMAND_ID_HANDLER(IDC_BTN_TEMPALLOW		, OnCommand)

		COMMAND_ID_HANDLER(IDC_BTN_TRYTOGETROOTCA, OnTryToGetRootCA)
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
		ChangeControlTextForTranslateMessage(m_hWnd, IDC_BTN_TRYTOGETROOTCA);

		GetDlgItem(IDC_STATIC_DETAIL).SetWindowText(GetTranslateMessage(IDC_STATIC_DETAIL, (LPCWSTR)m_errorMsg).c_str());
		GetDlgItem(IDC_STATIC_HOST).SetWindowText(GetTranslateMessage(IDC_STATIC_HOST, (LPWSTR)CA2W(m_host.c_str())).c_str());

		if (m_bNoCA) {
			GetDlgItem(IDC_BTN_TRYTOGETROOTCA).ShowWindow(TRUE);
		}
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

	LRESULT OnTryToGetRootCA(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		auto DLData = WinHTTPWrapper::HttpDownloadData(("https://" + m_host).c_str());
		int lastLoadCACount = g_loadCACount;
		g_loadCACount = 0;
		//ATLVERIFY(LoadSystemTrustCA(g_sslclientCtx));
		if (lastLoadCACount < g_loadCACount) {	// success
			MessageBox(GetTranslateMessage(ID_SUCCEEDGETROOTCA).c_str(), GetTranslateMessage(ID_TRANS_SUCCESS).c_str(), MB_OK);
			g_lastnoCAHost = m_host;
			EndDialog(IDCANCEL);

		} else {
			MessageBox(GetTranslateMessage(ID_FAILEDGETROOTCA).c_str(), GetTranslateMessage(ID_TRANS_FAILURE).c_str(), MB_ICONWARNING | MB_OK);
		}
		return 0;
	}

private:
	std::string	m_host;
	CString	m_errorMsg;
	bool	m_bNoCA;
};


bool	ManageCatificateErrorPage(std::shared_ptr<SocketIF> sockBrowser, const std::string& host, const CString& errorMsg, bool bNoCA)
{
	std::wstring filename = L"./html/certificate_error.html";
	std::string contentType = "text/html";
	std::string content = CUtil::getFile(filename);
	if (content.empty())
		return false;	// 旧dialog形式へ

	content = CUtil::replaceAll(content, "%%title%%", CodeConvert::UTF8fromUTF16(GetTranslateMessage(IDD_CERTIFICATEERROR)));
	content = CUtil::replaceAll(content, "%%static-waring%%", CodeConvert::UTF8fromUTF16(GetTranslateMessage(IDC_STATIC_WARNING)));
	content = CUtil::replaceAll(content, "%%host-name%%", host);
	content = CUtil::replaceAll(content, "%%auth%%", g_authentication);
	content = CUtil::replaceAll(content, "%%error-detail%%", CodeConvert::UTF8fromUTF16(GetTranslateMessage(IDC_STATIC_DETAIL, (LPCWSTR)errorMsg)));

	content = CUtil::replaceAll(content, "%%noCA%%", bNoCA ? "true" : "false");
	content = CUtil::replaceAll(content, "%%try-getrootca%%", CodeConvert::UTF8fromUTF16(GetTranslateMessage(IDC_BTN_TRYTOGETROOTCA)));

	content = CUtil::replaceAll(content, "%%static-connectioncontinue%%", CodeConvert::UTF8fromUTF16(GetTranslateMessage(IDC_STATIC_CONNECTIONCONTINUE)));
	content = CUtil::replaceAll(content, "%%temp-allow%%", CodeConvert::UTF8fromUTF16(GetTranslateMessage(IDC_BTN_TEMPALLOW)));
	content = CUtil::replaceAll(content, "%%temp-deny%%", CodeConvert::UTF8fromUTF16(GetTranslateMessage(IDC_BTN_TEMPDENY)));
	content = CUtil::replaceAll(content, "%%permanent-allow%%", CodeConvert::UTF8fromUTF16(GetTranslateMessage(IDC_BTN_PERMANENTALLOW)));
	content = CUtil::replaceAll(content, "%%permanent-deny%%", CodeConvert::UTF8fromUTF16(GetTranslateMessage(IDC_BTN_PERMANENTDENY)));

#define CRLF "\r\n"

	std::string sendInBuf =
		"HTTP/1.1 200 OK" CRLF
		"Content-Type: " + contentType + CRLF
		"Content-Length: " + boost::lexical_cast<std::string>(content.size()) + CRLF
		"Connection: close" + CRLF CRLF;
	sendInBuf += content;

	std::atomic_bool valid = true;
	auto clientSession = CSSLSession::InitServerSession(sockBrowser, host, valid);
	//ATLASSERT(clientSession);
	if (clientSession == nullptr) {
		return true;	// deny
	}

	while (clientSession->Write(sendInBuf.data(), static_cast<int>(sendInBuf.length())) > 0);

	clientSession->Close();

	return true;
}

std::wstring SSL_ErrorString()
{
	enum { kBufferSize = 512 };
	char tempBuffer[kBufferSize] = "";
	auto error_no = ERR_get_error();
	ERR_error_string_n(error_no, tempBuffer, kBufferSize);
	auto errmsg = CodeConvert::UTF16fromUTF8(tempBuffer);
	return errmsg;
}
// ===========================================================


int myVerify(int preverify_ok, X509_STORE_CTX* x509_ctx)
{
	if (preverify_ok) {
		return preverify_ok;	// 証明書の検証OK！
	}

	int err = X509_STORE_CTX_get_error(x509_ctx);
	std::string certError = X509_verify_cert_error_string(err);

	{
		CCritSecLock lock(g_csmapHost);
		
		SSL* ssl = (SSL*)X509_STORE_CTX_get_ex_data(x509_ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
		ATLASSERT(ssl);
		SSLCallbackContext* pcontext = 
			static_cast<SSLCallbackContext*>(SSL_get_ex_data(ssl, g_sslCallbackContextIndex));
		ATLASSERT(pcontext);

		std::string host = pcontext->host;
		ATLASSERT(pcontext->sockBrowser);
		if (!pcontext->sockBrowser) {
			return 0;	// ヌルポなのはおかしい
		}
		if (host == g_lastnoCAHost) {
			return 0;

		} else {
			g_lastnoCAHost.clear();
		}

		auto itfound = g_mapHostAllowOrDeny.find(host);
		if (itfound != g_mapHostAllowOrDeny.end()) {
			if (itfound->second) {
				return 1;	// allow
			} else {
				return 0;	// deny
			}
		}

		auto ittempfound = g_mapHostTempAllowOrDeny.find(host);
		if (ittempfound != g_mapHostTempAllowOrDeny.end()) {
			// 5分超えていれば消す
			if ((std::chrono::steady_clock::now() - ittempfound->second.first) > std::chrono::minutes(5)) {
				g_mapHostTempAllowOrDeny.erase(ittempfound);
			} else {
				if (ittempfound->second.second) {
					return 1;	// allow
				} else {
					return 0;	// deny
				}
			}
		}

		{
			CFilterOwner owner;
			CFilter filter(owner);
			std::wstring utf16host = CodeConvert::UTF16fromUTF8(host);
			if (g_pAllowSSLServerHostMatcher->match(utf16host, &filter))
				return 1;	// allow
		}

		bool bNoCA = true;	// (store->error == ASN_NO_SIGNER_E);
		if (ManageCatificateErrorPage(pcontext->sockBrowser, host, certError.c_str(), bNoCA)) {
			return 0;	// deny
		}

		// old dialog version
		CCertificateErrorDialog crtErrorDlg(host, certError.c_str(), bNoCA);
		if (crtErrorDlg.DoModal() == IDOK) {
			return 1;	// allow
		} else {
			return 0;	// deny
		}
	}

	//printf("Allowing to continue anyway (shouldn't do this, EVER!!!)\n");

	return preverify_ok;
}

bool	LoadSystemTrustCA()
{
	std::string allCAData;

	auto funcAddCA = [&allCAData](HCERTSTORE store) -> bool {
		if (store == NULL)
			return false;

		X509_STORE* cert_store = SSL_CTX_get_cert_store(g_openssl_client_ctx);
		
		PCCERT_CONTEXT crtcontext = CertEnumCertificatesInStore(store, NULL);
		while (crtcontext) {
			if (crtcontext->dwCertEncodingType == X509_ASN_ENCODING) {
				CString crtName;
				CertGetNameString(crtcontext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, 0, crtName.GetBuffer(256), 256);
				crtName.ReleaseBuffer();

				//if (crtName == L"DST Root CA X3") {
				//	int a = 0;
				//	goto NextCert;
				//}

				BIO_ptr memASN1(BIO_new_mem_buf(crtcontext->pbCertEncoded, crtcontext->cbCertEncoded));
				X509_ptr caCert(d2i_X509_bio(memASN1.get(), nullptr));
				ATLVERIFY(X509_STORE_add_cert(cert_store, caCert.get()));
				++g_loadCACount;
#if 0
				int ret = wolfSSL_CTX_load_verify_buffer(ctx, crtcontext->pbCertEncoded, crtcontext->cbCertEncoded, SSL_FILETYPE_ASN1);
				if (ret != SSL_SUCCESS) {

					ATLTRACE(L"wolfSSL_CTX_load_verify_buffer failed: %s\n", (LPCWSTR)crtName);
				} else {
					++g_loadCACount;
				}
#endif
			}
			crtcontext = CertEnumCertificatesInStore(store, crtcontext);
		}
		CertCloseStore(store, 0);
		return true;
	};

	if (funcAddCA(CertOpenSystemStoreW(0, L"ROOT")) == false)
		return false;
	if (funcAddCA(CertOpenSystemStoreW(0, L"CA")) == false)
		return false;

#if 0	// CRL
	if (funcAddCA(CertOpenSystemStoreW(0, L"Disallowed")) == false)
		return false; 
#endif
	INFO_LOG << L"LoadSystemTrustCA - loadCACount: " << g_loadCACount;
	return true;
}


// サーバー証明書の作成
std::unique_ptr<CertAndKey>	CreateServerCert(const std::string& host)
{
	int ret = 0;

	/* Allocate memory for the X509 structure. */
	X509_ptr x509(X509_new());
	if (!x509) {
		ERROR_LOG << L"Unable to create X509 structure.";
		return nullptr;
	}

	/* Allocate memory for the EVP_PKEY structure. */
	EVP_PKEY_ptr pkey(EVP_EC_gen("P-256")); // EVP_RSA_gen(2048));
	if (!pkey) {
		ERROR_LOG << L"EVP_RSA_gen failed";
		return nullptr;
	}

	/* Set x509 version 3 */
	X509_set_version(x509.get(), X509_VERSION_3);

	/* Set the serial number. */
	std::random_device random_engine;
	std::uniform_int_distribution<long> dist(LONG_MIN, LONG_MAX);
	ASN1_INTEGER_set(X509_get_serialNumber(x509.get()), dist(random_engine));

	/* This certificate is valid from now until exactly one year from now. */
	const long valid_secs = 365 * 60 * 60 * 24;   // 1年
	X509_gmtime_adj(X509_get_notBefore(x509.get()), 0);
	X509_gmtime_adj(X509_get_notAfter(x509.get()), valid_secs);

	/* Set the public key for our certificate. */
	X509_set_pubkey(x509.get(), pkey.get());

	/* We want to copy the subject name to the issuer name. */
	X509_NAME* name = X509_get_subject_name(x509.get());

	/* Set the country code and common name. */
	X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char*)"Proxydomo TLS Server", -1, -1, 0);
	X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (const unsigned char*)"dummy CN", -1, -1, 0);

	/* Now set the issuer name. */
	auto issuer_name = X509_get_issuer_name(g_CA_cert_key.cert.get());
	X509_set_issuer_name(x509.get(), issuer_name);

	/* Add subjectAltName */
	std::string san_dns = "DNS:" + host;
	X509_EXTENSION* cert_ex = X509V3_EXT_conf_nid(NULL, NULL, NID_subject_alt_name, san_dns.data());
	X509_add_ext(x509.get(), cert_ex, -1);
	X509_EXTENSION_free(cert_ex);

	/* Actually sign the certificate with our key. */
	if (!X509_sign(x509.get(), g_CA_cert_key.privateKey.get(), EVP_sha256())) {
		ERROR_LOG << L"Error signing certificate.";
		return nullptr;
	}
#if 0
	BIO* mem3 = BIO_new(BIO_s_mem());
	ret = X509_print(mem3, x509.get());
	//ret = X509_print(mem3, ca_cart);

	char test[5120] = "";
	ret = BIO_read(mem3, test, 5120);
#endif
	// サーバー証明書生成完了！
	auto certAndKey = std::make_unique<CertAndKey>();
	certAndKey->cert.swap(x509);
	certAndKey->privateKey.swap(pkey);

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

	int ret = SSL_library_init();
	if (ret != 1) {
		ERROR_LOG << L"SSL_library_init failed";
		return false;
	}
	OpenSSL_add_all_algorithms();

	try {

		{	// server init
			auto pemCA = CUtil::LoadBinaryFile((LPCWSTR)cafile);
			auto pemCAkey = CUtil::LoadBinaryFile((LPCWSTR)rsacaKeyfile);
			if (pemCAkey.empty()) {
				pemCAkey = CUtil::LoadBinaryFile((LPCWSTR)ecccaKeyfile);
			}

			// CA証明書 と privateKey の読み込み
			BIO_ptr pmemCA(BIO_new_mem_buf((const void*)pemCA.data(), (int)pemCA.size()));
			g_CA_cert_key.cert.reset(PEM_read_bio_X509(pmemCA.get(), nullptr, nullptr, nullptr));

			BIO_ptr pmemKey(BIO_new_mem_buf((const void*)pemCAkey.data(), (int)pemCAkey.size()));
			g_CA_cert_key.privateKey.reset(PEM_read_bio_PrivateKey(pmemKey.get(), nullptr, nullptr, nullptr));
		}

		{	// client init
			g_openssl_client_ctx = SSL_CTX_new(SSLv23_client_method());
			if (!g_openssl_client_ctx) {
				throw std::runtime_error("SSL_CTX_new failed");
			}

			/* 証明書の検証設定 */
			SSL_CTX_set_verify(g_openssl_client_ctx, SSL_VERIFY_PEER, myVerify);

			g_sslCallbackContextIndex = SSL_get_ex_new_index(0, "SSLCallbackContext", NULL, NULL, NULL);

			if (!LoadSystemTrustCA()) {
				throw std::runtime_error("LoadSystemTrustCA failed");
			}
		}
		g_pAllowSSLServerHostMatcher = Proxydomo::CMatcher::CreateMatcher(L"$LST(AllowSSLServerHostList)");

		// 認証トークン生成
		enum { kMaxAuthLength = 8 };
		std::string chars = "abcdefghijklmnopqrstuvwxyz0123456789";
		std::random_device rd;
		std::mt19937 engine(rd());
		std::uniform_int_distribution<int> dist(0, static_cast<int>(chars.length()) - 1);
		std::array<int, kMaxAuthLength> randnum;
		for (int i = 0; i < kMaxAuthLength; ++i) {
			randnum[i] = dist(engine);
		}
		for (int i = 0; i < kMaxAuthLength; ++i) {
			g_authentication += chars[randnum[i]];
		}

		return true;

	} catch (std::exception& e) {
		TermSSL();

		ERROR_LOG << L"InitSSL failed : " << (LPWSTR)CA2W(e.what());
		return false;
	}	
}

void	TermSSL()
{
	if (g_openssl_client_ctx) {
		SSL_CTX_free(g_openssl_client_ctx);
		g_openssl_client_ctx = nullptr;
	}
}

void	LogErrorAndThrowRuntimeExecption(const std::wstring& errorMessage)
{
	ERROR_LOG << errorMessage;
	auto u8msg = CodeConvert::UTF8fromUTF16(errorMessage);
	throw std::runtime_error(u8msg);
}

// CA証明書を生成する
void	GenerateCACertificate()
{
	/* Allocate memory for the X509 structure. */
	X509_ptr x509(X509_new());
	if (!x509) {
		LogErrorAndThrowRuntimeExecption(L"Unable to create X509 structure.");
	}

	/* Allocate memory for the EVP_PKEY structure. */
	EVP_PKEY_ptr pkey(EVP_EC_gen("P-256"));
	if (!pkey) {
		LogErrorAndThrowRuntimeExecption(L"EVP_RSA_gen failed");
	}

	/* Set x509 version 3 */
	X509_set_version(x509.get(), X509_VERSION_3);

	/* Set the serial number. */
	std::random_device random_engine;
	std::uniform_int_distribution<long> dist(LONG_MIN, LONG_MAX);
	ASN1_INTEGER_set(X509_get_serialNumber(x509.get()), dist(random_engine));

	/* This certificate is valid from now until exactly one year from now. */
	const long valid_secs = 365 * 60 * 60 * 24;   // 1年
	X509_gmtime_adj(X509_get_notBefore(x509.get()), 0);
	X509_gmtime_adj(X509_get_notAfter(x509.get()), valid_secs);

	/* Set the public key for our certificate. */
	X509_set_pubkey(x509.get(), pkey.get());

	/* We want to copy the subject name to the issuer name. */
	X509_NAME* name = X509_get_subject_name(x509.get());

	/* Set the country code and common name. */
	X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (const unsigned char*)"Proxydomo CA2", -1, -1, 0);

	/* Now set the issuer name. */
	X509_set_issuer_name(x509.get(), name);		// subject == issuer

	/* Add CA flag */
	std::string extvalue = "CA:TRUE";
	X509_EXTENSION* cert_ex = X509V3_EXT_conf_nid(NULL, NULL, NID_basic_constraints, extvalue.data());
	X509_add_ext(x509.get(), cert_ex, -1);
	X509_EXTENSION_free(cert_ex);

	/* Actually sign the certificate with our key. */
	if (!X509_sign(x509.get(), pkey.get(), EVP_sha256())) {
		LogErrorAndThrowRuntimeExecption(L"Error signing certificate.");
	}

	// ca cert
	BIO_ptr pmemCA(BIO_new(BIO_s_mem()));
	PEM_write_bio_X509(pmemCA.get(), x509.get());

	// ca private key
	BIO_ptr pmemKey(BIO_new(BIO_s_mem()));
	PEM_write_bio_PrivateKey(pmemKey.get(), pkey.get(), nullptr, nullptr, 0, nullptr, nullptr);

	auto funcBIOWriteToFile = [](BIO_ptr& bio, const CString& filePath) {
		std::ofstream ofs((LPCWSTR)filePath, std::ios::binary | std::ios::out);
		if (!ofs) {
			LogErrorAndThrowRuntimeExecption(L"filePath open failed");
			return ;
		}

		char* data = nullptr;
		int dataSize = BIO_get_mem_data(bio.get(), &data);

		ofs.write(data, dataSize);
		ofs.close();
	};

	funcBIOWriteToFile(pmemCA, Misc::GetExeDirectory() + kCAFileName);
	funcBIOWriteToFile(pmemKey, Misc::GetExeDirectory() + kCAKeyFileName);
	// finish!
}

// https://local.ptron/certificate_api 
bool	ManageCertificateAPI(const std::string& url, SocketIF* sockBrowser)
{
	std::string authURL = "/certificate_api?&auth=" + g_authentication;
	if (boost::starts_with(url, authURL) == false)
		return false;

	auto funcGetValue = [&](const std::string& name) -> boost::optional<std::string> {
		std::regex rx(name + "=([^& ]+)");
		std::smatch result;
		if (std::regex_search(url, result, rx) == false)
			return boost::none;

		std::string value = result[1].str();
		return value;
	};

	auto operation = funcGetValue("operation");
	if (!operation)
		return false;

	auto host = funcGetValue("host");
	if (!host)
		return false;

	{
		std::string status = "Success";
		std::wstring description = L"none";
		CCritSecLock lock(g_csmapHost);
		if (*operation == "permanent-allow") {
			g_mapHostAllowOrDeny[*host] = true;
			description = GetTranslateMessage(ID_ADDHOSTALLOWLIST);

		} else if (*operation == "permanent-deny") {
			g_mapHostAllowOrDeny[*host] = false;
			description = GetTranslateMessage(ID_ADDHOSTDENYLIST);

		} else if (*operation == "temp-allow") {
			g_mapHostTempAllowOrDeny[*host] = std::make_pair(std::chrono::steady_clock::now(), true);
			description = GetTranslateMessage(ID_ADDHOSTTEMPALLOWLIST);

		} else if (*operation == "temp-deny") {
			g_mapHostTempAllowOrDeny[*host] = std::make_pair(std::chrono::steady_clock::now(), false);
			description = GetTranslateMessage(ID_ADDHOSTTEMPDENYLIST);

		} else if (*operation == "try-getrootca") {
			auto DLData = WinHTTPWrapper::HttpDownloadData(("https://" + *host).c_str());
			int lastLoadCACount = g_loadCACount;
			g_loadCACount = 0;
			ATLVERIFY(LoadSystemTrustCA());
			if (lastLoadCACount < g_loadCACount) {	// success
				description = CUtil::replaceAll(GetTranslateMessage(ID_SUCCEEDGETROOTCA), L"\n", L"\\\\n");	

			} else {
				description = GetTranslateMessage(ID_FAILEDGETROOTCA);
				status = "Failure";
			}
		} else {
			status = "Failure";
		}

		std::string content =  (boost::format(R"({"status": "%1%", "description": "%2%"})") % status % CodeConvert::UTF8fromUTF16(description)).str();
		std::string sendInBuf =
			"HTTP/1.1 200 OK" CRLF
			"Content-Type: application/json" CRLF
			"Content-Length: " + boost::lexical_cast<std::string>(content.size()) + CRLF
			"Access-Control-Allow-Origin: *" CRLF
			"Connection: close" + CRLF CRLF;
		sendInBuf += content;
		while (sockBrowser->Write(sendInBuf.data(), static_cast<int>(sendInBuf.length())) > 0) ;
		sockBrowser->Close();
	}
	return true;
}

static int NonBlockingSSL_Connect(SSL* ssl, std::atomic_bool& valid, bool isServer)
{
	int ret;
	int error;
	SOCKET sockfd = (SOCKET)SSL_get_fd(ssl);;
	int select_ret = 0;

	static const std::chrono::seconds timeout(60);
	auto connectStartTime = std::chrono::steady_clock::now();

	unsigned long op = 1;	// non blocking
	//ioctlsocket(sockfd, FIONBIO, &op);

	while (valid) {
		if (isServer) {
			ret = SSL_accept(ssl);
		} else {
			ret = SSL_connect(ssl);
		}
		error = SSL_get_error(ssl, ret);
		switch (error)
		{
		case SSL_ERROR_NONE:
			return TRUE;
			break;	// success

		case SSL_ERROR_WANT_READ:
		case SSL_ERROR_WANT_WRITE:
			continue;	// pending

		case SSL_ERROR_SYSCALL:
		default:
			// エラー処理
			return FALSE;
		}
		break;
	}

	//return ret;
	return FALSE;
}


////////////////////////////////////////////////////////////////
// CSSLSession

CSSLSession::CSSLSession() : m_ctx(nullptr), m_ssl(nullptr)
{
	m_writeStop = false;
}

CSSLSession::~CSSLSession()
{
	Close();
}


std::shared_ptr<SocketIF>	CSSLSession::InitClientSession(std::shared_ptr<SocketIF> sockWebsite, const std::string& host,
																std::shared_ptr<SocketIF> sockBrowser, std::atomic_bool& valid)
{
	auto session = std::make_shared<CSSLSession>();
	int ret = 0;

	session->m_ssl = SSL_new(g_openssl_client_ctx);
	if (session->m_ssl == nullptr)
		throw std::runtime_error("wolfSSL_new failed");

	SSL_set_fd(session->m_ssl, (int)sockWebsite->GetSocket());

	// ホスト名の検証を行う
	SSL_set_hostflags(session->m_ssl, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
	SSL_set1_host(session->m_ssl, host.c_str());

	// for SNI
	ret = SSL_set_tlsext_host_name(session->m_ssl, host.c_str());
	ATLASSERT(ret);

	// https://www.openssl.org/docs/man1.1.1/man3/SSL_clear_options.html
	// SECURE RENEGOTIATION
	SSL_set_options(session->m_ssl, SSL_OP_LEGACY_SERVER_CONNECT);

	// for verify_certificate callback
	SSLCallbackContext callbackContext(host, sockBrowser);
	SSL_set_ex_data(session->m_ssl, g_sslCallbackContextIndex, (void*)&callbackContext);	

	//wolfSSL_set_using_nonblock(session->m_ssl, 1);
	ret = NonBlockingSSL_Connect(session->m_ssl, valid, false);
	if (!ret) {
		//int err = wolfSSL_get_error(session->m_ssl, 0);
		//char buffer[WOLFSSL_MAX_ERROR_SZ];
		//ATLTRACE("host = %s error = %d, %s\n", host.c_str(), err, wolfSSL_ERR_error_string(err, buffer));
		//buffer;

		auto errmsg = SSL_ErrorString();
		WARN_LOG << L"SSL_Connect failed - host[" << host << L"] : " << errmsg;

		SSL_free(session->m_ssl);
		session->m_ssl = nullptr;
		sockWebsite->Close();
		return nullptr;
	}

	session->m_sock = sockWebsite;
	return session;
}

std::shared_ptr<SocketIF> CSSLSession::InitServerSession(std::shared_ptr<SocketIF> sockBrowser, const std::string& host, std::atomic_bool& valid)
{
	auto session = std::make_shared<CSSLSession>();
	int ret = 0;

	// 証明書生成 (キャッシュがあればそっちから持ってくる)
	CertAndKey* certAndKey = nullptr;
	{
		CCritSecLock lock(g_csmapHostServerCert);
		auto itfound = g_mapHostServerCert.find(host);
		if (itfound != g_mapHostServerCert.end()) {
			certAndKey = itfound->second.get();		// キャッシュから
		} else {
			auto serverCertAndKey = CreateServerCert(host);	// 生成
			certAndKey = serverCertAndKey.get();
			g_mapHostServerCert[host] = std::move(serverCertAndKey);
		}
	}
	ATLASSERT(certAndKey);

	const SSL_METHOD* method = SSLv23_server_method();
	if (method == nullptr) {
		throw std::runtime_error("SSLv23_server_method failed");
	}
	session->m_ctx = SSL_CTX_new(method);
	if (session->m_ctx == nullptr) {
		throw std::runtime_error("SSL_CTX_new failed");
	}

	ret = SSL_CTX_use_certificate(session->m_ctx, certAndKey->cert.get());
	ATLASSERT(ret);
	ret = SSL_CTX_use_PrivateKey(session->m_ctx, certAndKey->privateKey.get());
	ATLASSERT(ret);

	session->m_ssl = SSL_new(session->m_ctx);
	if (session->m_ssl == nullptr) {
		SSL_CTX_free(session->m_ctx);
		session->m_ctx = nullptr;
		throw std::runtime_error("SSL_new failed");
	}

	SSL_set_fd(session->m_ssl, (int)sockBrowser->GetSocket());

	//wolfSSL_set_using_nonblock(session->m_ssl, 1);
	ret = NonBlockingSSL_Connect(session->m_ssl, valid, true);
	if (!ret) {
#if 0
		auto errmsg = SSL_ErrorString();
#endif
		//WARN_LOG << L"SSL_Accept failed - host[" << host << L"]";	// : " << errmsg;

		SSL_free(session->m_ssl);
		session->m_ssl = nullptr;
		SSL_CTX_free(session->m_ctx);
		session->m_ctx = nullptr;
		sockBrowser->Close();
		return nullptr;
	}

	session->m_sock = sockBrowser;
	return session;
}


int	CSSLSession::Read(char* buffer, int length)
{
	if (m_ssl == nullptr)
		return -1;

	int ret = SSL_read(m_ssl, (void*)buffer, length);
	if (ret == 0) {
		Close();
		return 0;

	} else {
		int error = SSL_get_error(m_ssl, 0);
		if (ret < 0 && error == SSL_ERROR_WANT_READ) {
			return false;	// pending
		}
		if (ret < 0) {
			Close();
			return -1;	// error

		} else {
			return ret;	// success!
		}
	}
}

int	CSSLSession::Write(const char* buffer, int length)
{
	if (m_ssl == nullptr)
		return -1;

	int ret = 0;
	int error = 0;
	for (;;) {
		ret = SSL_write(m_ssl, (const void*)buffer, length);
		error = SSL_get_error(m_ssl, 0);
		if (ret < 0 && error == SSL_ERROR_WANT_WRITE && m_writeStop == false) {
			::Sleep(50);	// async pending
		} else {
			break;
		}
	}

	if (ret != length) {
		Close();
		return -1;	// error

	} else {
		return ret;
	}
}


void	CSSLSession::Close()
{
	if (m_ssl == nullptr || m_sock == nullptr)
		return;

	int ret = SSL_shutdown(m_ssl);
	try {
		m_sock->Close();
	}
	catch (SocketException& e) {
		e;
	}
	m_sock = nullptr;

	SSL_free(m_ssl);
	m_ssl = nullptr;
	if (m_ctx) {
		SSL_CTX_free(m_ctx);
		m_ctx = nullptr;
	}
}









