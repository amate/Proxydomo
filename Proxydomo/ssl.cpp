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

#pragma comment(lib, "libgnutls-28.lib")
#include <gnutls\x509.h>
#include <gnutls\abstract.h>
#include <gnutls\crypto.h>
#include <atlbase.h>
#include <atlsync.h>
#include <atlwin.h>
#include "Misc.h"
#include "Logger.h"
#include "resource.h"

namespace {

LPCSTR	kCAFileName = "ca.pem.crt";
LPCSTR	kCAKeyFileName = "ca-key.pem";

gnutls_priority_t g_server_priority_cache = NULL;
//gnutls_dh_params_t dh_params = NULL;

gnutls_x509_crt_t g_ca_crt = NULL;
gnutls_privkey_t g_ca_key = NULL;

gnutls_certificate_credentials_t	g_client_cred = NULL;
gnutls_priority_t					g_client_priority_cache = NULL;

std::unordered_map<std::string, bool>	g_mapHostAllowOrDeny;
CCriticalSection						g_csmapHost;


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
		m_errorMsg.Insert(0, _T("詳細:"));
		GetDlgItem(IDC_STATIC_DETAIL).SetWindowText(m_errorMsg);

		CString host;
		host.Format(_T("Host: %s"), (LPWSTR)CA2W(m_host.c_str()));
		GetDlgItem(IDC_STATIC_HOST).SetWindowText(host);
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


/* This function will verify the peer's certificate, and check
* if the hostname matches, as well as the activation, expiration dates.
*/
int verify_certificate_callback(gnutls_session_t session)
{
	unsigned int status;
	int ret;
	gnutls_certificate_type_t type;
	const char* hostname = nullptr;
	size_t data_length = 512;
	gnutls_datum_t out;

	/* read hostname */
	//unsigned int typeName = GNUTLS_NAME_DNS;
	//gnutls_server_name_get(session, (void*)hostname, &data_length, &typeName, 0);
	hostname = (const char*)gnutls_session_get_ptr(session);

	/* This verification function uses the trusted CAs in the credentials
	* structure. So you must have installed one or more CA certificates.
	*/

	/* The following demonstrate two different verification functions,
	* the more flexible gnutls_certificate_verify_peers(), as well
	* as the old gnutls_certificate_verify_peers3(). */
#if 0
	{
		gnutls_typed_vdata_st data[2];

		memset(data, 0, sizeof(data));

		data[0].type = GNUTLS_DT_DNS_HOSTNAME;
		data[0].data = (void*)hostname;

		data[1].type = GNUTLS_DT_KEY_PURPOSE_OID;
		data[1].data = (void*)GNUTLS_KP_TLS_WWW_SERVER;

		ret = gnutls_certificate_verify_peers(session, data, 2,
			&status);
	}
#else
	ret = gnutls_certificate_verify_peers3(session, hostname, &status);
#endif
	if (ret < 0) {
		//printf("Error\n");
		return GNUTLS_E_CERTIFICATE_ERROR;
	}

	type = gnutls_certificate_type_get(session);
	ret = gnutls_certificate_verification_status_print(status, type, &out, 0);
	if (ret < 0) {
		//printf("Error\n");
		return GNUTLS_E_CERTIFICATE_ERROR;
	}

	if (status == 66) {
		ERROR_LOG << L"certificate verify error 66 [host: " << (LPWSTR)CA2W(hostname) << L"]";
		status = 0;
	}

	if (status != 0 && status != 66) {
		CCritSecLock lock(g_csmapHost);
		std::string host = hostname;
		auto itfound = g_mapHostAllowOrDeny.find(host);
		if (itfound != g_mapHostAllowOrDeny.end()) {
			gnutls_free(out.data);

			if (itfound->second) {
				return 0;
			} else {
				return GNUTLS_E_CERTIFICATE_ERROR;
			}
		}

		CCertificateErrorDialog crtErrorDlg(host, out.data);
		gnutls_free(out.data);

		if (crtErrorDlg.DoModal() == IDOK) {
			return 0;
		} else {
			return GNUTLS_E_CERTIFICATE_ERROR;
		}
	}

	gnutls_free(out.data);

	if (status != 0)        /* Certificate is not trusted */
		return GNUTLS_E_CERTIFICATE_ERROR;

	/* notify gnutls to continue handshake normally */
	return 0;
}


// ===========================================================

class CGnuTLSException : public std::exception
{
public:
	CGnuTLSException(const char* msg, int errorno = 0) : std::exception(msg), m_errorno(errorno) {}

private:
	int		m_errorno;
};

std::vector<BYTE>	LoadBinaryFile(const std::string& filePath)
{
	std::ifstream fs(filePath, std::ios::in | std::ios::binary);
	if (!fs)
		return std::vector<BYTE>();

	fs.seekg(0, std::ios::end);
	auto eofPos = fs.tellg();
	fs.clear();
	fs.seekg(0, std::ios::beg);
	auto begPos = fs.tellg();
	size_t fileSize = eofPos - begPos;

	std::vector<BYTE> vec(fileSize);
	fs.read((char*)vec.data(), fileSize);

	return vec;
}


gnutls_x509_privkey_t generate_private_key()
{
	gnutls_x509_privkey_t key;

	//key = generate_private_key_int(cinfo);
	int ret = 0;
	ret = gnutls_x509_privkey_init(&key);
	if (ret < 0) {
		throw CGnuTLSException("gnutls_x509_privkey_init failed", ret);
	}

	unsigned int bits = gnutls_sec_param_to_pk_bits(GNUTLS_PK_RSA, GNUTLS_SEC_PARAM_NORMAL);

	//fprintf(stdlog, "Generating a %d bit %s private key...\n", bits, gnutls_pk_algorithm_get_name(key_type));

	ret = gnutls_x509_privkey_generate(key, GNUTLS_PK_RSA, bits, 0);
	if (ret < 0) {
		throw CGnuTLSException("gnutls_x509_privkey_generate failed", ret);
	}

	ret = gnutls_x509_privkey_verify_params(key);
	if (ret < 0) {
		throw CGnuTLSException("gnutls_x509_privkey_verify_params failed", ret);
	}

	return key;
}

/* Load the private key.
*/
gnutls_privkey_t load_private_key(const std::string& privateKeyPath)
{
	gnutls_privkey_t key;
	auto vecBinary = LoadBinaryFile(privateKeyPath);

	if (vecBinary.empty()) {
		throw CGnuTLSException("load_private_key fileLoad failed");
	}

	gnutls_datum_t dat;
	dat.data = (unsigned char*)vecBinary.data();
	dat.size = vecBinary.size();

	//key = _load_privkey(&dat, info);
	int ret;
	ret = gnutls_privkey_init(&key);
	if (ret < 0) {
		throw CGnuTLSException("gnutls_privkey_init failed", ret);
	}

	ret = gnutls_privkey_import_x509_raw(key, &dat, GNUTLS_X509_FMT_PEM, NULL, 0);
	if (ret < 0) {
		throw CGnuTLSException("gnutls_privkey_import_x509_raw failed", ret);
	}

	return key;
}

/* Loads the CA's certificate
*/
gnutls_x509_crt_t load_ca_cert(const std::string& caFilePath)
{
	gnutls_x509_crt_t crt;
	int ret;
	ret = gnutls_x509_crt_init(&crt);
	if (ret < 0) {
		throw CGnuTLSException("gnutls_x509_crt_init", ret);
	}

	auto vecBinary = LoadBinaryFile(caFilePath);
	if (vecBinary.empty()) {
		throw CGnuTLSException("load_ca_cert fileLoad failed");
	}
	gnutls_datum_t dat;
	dat.data = (unsigned char*)vecBinary.data();
	dat.size = vecBinary.size();

	ret = gnutls_x509_crt_import(crt, &dat, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		throw CGnuTLSException("gnutls_x509_crt_import failed", ret);
	}
	return crt;
}


gnutls_pubkey_t load_public_key_or_import(gnutls_privkey_t privkey)
{
	gnutls_pubkey_t pubkey;
	int ret;
	ret = gnutls_pubkey_init(&pubkey);
	if (ret < 0) {
		throw CGnuTLSException("gnutls_pubkey_init failed", ret);
	}

	ret = gnutls_pubkey_import_privkey(pubkey, privkey, 0, 0);
	if (ret < 0) {	/* could not get (e.g. on PKCS #11 */
		gnutls_pubkey_deinit(pubkey);
		throw CGnuTLSException("gnutls_pubkey_import_privkey failed", ret);
		//return load_pubkey(mand, info);
	}
	return pubkey;
}

void get_serial(unsigned char* serial, size_t * size)
{
	gnutls_rnd(GNUTLS_RND_NONCE, (void*)serial, *size);
}

struct CertificateTemplate
{
	std::string cn;
	bool	ca;
	bool	cert_signing_key;

	std::string	organization;
	bool	tls_www_server;
	bool	encryption_key;
	bool	signing_key;

	CertificateTemplate() : 
		ca(false), cert_signing_key(false), 
		tls_www_server(false), encryption_key(false), signing_key(false) {}

	void	InitCACertificateTemplate(const std::string& cnName)
	{
		cn = cnName;
		ca = true;
		cert_signing_key = true;
	}

	void	InitSignedCertificateTemplate(const std::string& cnName, const std::string& organizationName)
	{
		cn = cnName;
		organization =  organizationName;
		tls_www_server = true;
		encryption_key = true;
		signing_key = true;
	}

};


gnutls_x509_crt_t generate_certificate(	gnutls_privkey_t* ret_key, 
										gnutls_x509_crt_t ca_crt, 
										gnutls_x509_privkey_t privateKey, 
										const CertificateTemplate& cfTemplate )
{
	gnutls_x509_crt_t crt;
	gnutls_privkey_t key = NULL;
	gnutls_pubkey_t pubkey;
	size_t size;
	int ret;
	int client;
	int result, ca_status = 0, is_ike = 0, path_len;
	time_t secs;
	int vers;
	unsigned int usage = 0, server;
	gnutls_x509_crq_t crq;	/* request */

	ret = gnutls_x509_crt_init(&crt);
	if (ret < 0) {
		throw CGnuTLSException("gnutls_x509_crt_init failed", ret);
	}
	// 秘密鍵を設定
	gnutls_privkey_init(&key);
	gnutls_privkey_import_x509(key, privateKey, GNUTLS_PRIVKEY_IMPORT_COPY);

	pubkey = load_public_key_or_import(key);

	{
		//get_dn_crt_set(crt);

		//get_cn_crt_set(crt);
		ret = gnutls_x509_crt_set_dn_by_oid(crt, GNUTLS_OID_X520_COMMON_NAME, 0, cfTemplate.cn.c_str(), cfTemplate.cn.length());
		//get_uid_crt_set(crt);
		//get_unit_crt_set(crt);
		//get_organization_crt_set(crt);
		if (cfTemplate.organization.length() > 0) {
			ret = gnutls_x509_crt_set_dn_by_oid(crt,
				GNUTLS_OID_X520_ORGANIZATION_NAME,
				0, cfTemplate.organization.c_str(), cfTemplate.organization.length());
		}
		//get_locality_crt_set(crt);
		//get_state_crt_set(crt);
		//get_country_crt_set(crt);
		//get_dc_set(TYPE_CRT, crt);

		//get_oid_crt_set(crt);
		//get_key_purpose_set(TYPE_CRT, crt);

		//get_pkcs9_email_crt_set(crt);
	}

	ret = gnutls_x509_crt_set_pubkey(crt, pubkey);
	if (ret < 0) {
		throw CGnuTLSException("gnutls_x509_crt_set_pubkey failed", ret);
	}

	{
		size_t serial_size;
		unsigned char serial[16];
		serial_size = sizeof(serial);

		get_serial(serial, &serial_size);

		ret = gnutls_x509_crt_set_serial(crt, serial, serial_size);
		if (ret < 0) {
			throw CGnuTLSException("gnutls_x509_crt_set_serial failed", ret);
		}
	}

	secs = time(NULL);// get_activation_date();
	ret = gnutls_x509_crt_set_activation_time(crt, secs);
	if (ret < 0) {
		throw CGnuTLSException("gnutls_x509_crt_set_activation_time failed", ret);
	}

	secs += 365 * 24*60*60;	// 1年間有効
	ret = gnutls_x509_crt_set_expiration_time(crt, secs);
	if (ret < 0) {
		throw CGnuTLSException("gnutls_x509_crt_set_expiration_time failed", ret);
	}

	/* append additional extensions */
	{

		ca_status = cfTemplate.ca;// get_ca_status();
		if (ca_status) {
			path_len = -1;	// default //0; // get_path_len();
		} else {
			path_len = -1;
		}

		ret = gnutls_x509_crt_set_basic_constraints(crt, ca_status, path_len);
		if (ret < 0) {
			throw CGnuTLSException("gnutls_x509_crt_set_expiration_time failed", ret);
		}

		client = false; //get_tls_client_status();
		if (client != 0) {
			result = gnutls_x509_crt_set_key_purpose_oid(crt,
				GNUTLS_KP_TLS_WWW_CLIENT,
				0);
			if (result < 0) {
				fprintf(stderr, "key_kp: %s\n",
					gnutls_strerror(result));
				exit(1);
			}
		}

		is_ike = false;// get_ipsec_ike_status();
		server = cfTemplate.tls_www_server; // get_tls_server_status();

		//get_dns_name_set(TYPE_CRT, crt);
		//get_uri_set(TYPE_CRT, crt);
		//get_ip_addr_set(TYPE_CRT, crt);
		//get_policy_set(crt);

		if (server != 0) {
			ret = gnutls_x509_crt_set_key_purpose_oid(crt, GNUTLS_KP_TLS_WWW_SERVER, 0);
			if (ret < 0) {
				throw CGnuTLSException("gnutls_x509_crt_set_key_purpose_oid failed", ret);
			}
		}

		if (!ca_status || server) {
			int pk;
			pk = gnutls_x509_crt_get_pk_algorithm(crt, NULL);
			if (pk == GNUTLS_PK_RSA) {	/* DSA and ECDSA keys can only sign. */
				ret = cfTemplate.signing_key; //get_sign_status(server);
				if (ret) {
					usage |= GNUTLS_KEY_DIGITAL_SIGNATURE;
				}

				ret = cfTemplate.encryption_key; // get_encrypt_status(server);
				if (ret) {
					usage |= GNUTLS_KEY_KEY_ENCIPHERMENT;
				}
			} else {
				usage |= GNUTLS_KEY_DIGITAL_SIGNATURE;
			}

			if (is_ike) {
				result =
					gnutls_x509_crt_set_key_purpose_oid
					(crt, GNUTLS_KP_IPSEC_IKE, 0);
				if (result < 0) {
					throw CGnuTLSException("gnutls_x509_crt_set_key_purpose_oid failed", result);
				}
			}
		}


		if (ca_status) {
			result = cfTemplate.cert_signing_key;	// get_cert_sign_status();
			if (result)
				usage |= GNUTLS_KEY_KEY_CERT_SIGN;

			result = false;	// get_crl_sign_status();
			if (result)
				usage |= GNUTLS_KEY_CRL_SIGN;

			result = false;	// get_code_sign_status();
			if (result) {
				result = gnutls_x509_crt_set_key_purpose_oid(crt, GNUTLS_KP_CODE_SIGNING, 0);
				if (result < 0) {
					throw CGnuTLSException("gnutls_x509_crt_set_key_purpose_oid failed", result);
				}
			}

			//crt_constraints_set(crt);

			result = false;	// get_ocsp_sign_status();
			if (result) {
				result = gnutls_x509_crt_set_key_purpose_oid(crt, GNUTLS_KP_OCSP_SIGNING, 0);
				if (result < 0) {
					throw CGnuTLSException("gnutls_x509_crt_set_key_purpose_oid failed", result);
				}
			}

			result = false;	// get_time_stamp_status();
			if (result) {
				result =
					gnutls_x509_crt_set_key_purpose_oid(crt, GNUTLS_KP_TIME_STAMPING, 0);
				if (result < 0) {
					throw CGnuTLSException("gnutls_x509_crt_set_key_purpose_oid failed", result);
				}
			}
		}
		//get_ocsp_issuer_set(crt);
		//get_ca_issuers_set(crt);

		if (usage != 0) {
			/* http://tools.ietf.org/html/rfc4945#section-5.1.3.2: if any KU is
			set, then either digitalSignature or the nonRepudiation bits in the
			KeyUsage extension MUST for all IKE certs */
			if (is_ike && cfTemplate.signing_key)
				usage |= GNUTLS_KEY_NON_REPUDIATION;
			ret = gnutls_x509_crt_set_key_usage(crt, usage);
			if (ret < 0) {
				throw CGnuTLSException("gnutls_x509_crt_set_key_usage failed", ret);
			}
		}

		/* Subject Key ID.
		*/
		enum { lbuffer_size = 128 };
		unsigned char lbuffer[lbuffer_size];
		size = lbuffer_size;
		ret = gnutls_x509_crt_get_key_id(crt, 0, lbuffer, &size);
		if (ret >= 0) {
			ret = gnutls_x509_crt_set_subject_key_id(crt, lbuffer, size);
			if (ret < 0) {
				throw CGnuTLSException("gnutls_x509_crt_set_subject_key_id failed", ret);
			}
		}

		/* Authority Key ID.
		*/
		if (ca_crt != NULL) {
			size = lbuffer_size;
			ret = gnutls_x509_crt_get_subject_key_id(ca_crt, lbuffer, &size, NULL);
			if (ret < 0) {
				size = lbuffer_size;
				ret = gnutls_x509_crt_get_key_id(ca_crt, 0, lbuffer, &size);
			}
			if (ret >= 0) {
				ret = gnutls_x509_crt_set_authority_key_id(crt, lbuffer, size);
				if (ret < 0) {
					throw CGnuTLSException("gnutls_x509_crt_set_authority_key_id failed", ret);
				}
			}
		}
	}

	/* Version.
	*/
	vers = 3;
	ret = gnutls_x509_crt_set_version(crt, vers);
	if (ret < 0) {
		throw CGnuTLSException("gnutls_x509_crt_set_authority_key_id failed", ret);
	}

	*ret_key = key;
	return crt;

}

gnutls_digest_algorithm_t get_dig_for_pub(gnutls_pubkey_t pubkey)
{
	gnutls_digest_algorithm_t dig;
	int result;
	unsigned int mand;

	result = gnutls_pubkey_get_preferred_hash_algorithm(pubkey, &dig, &mand);
	if (result < 0) {
		throw CGnuTLSException("gnutls_pubkey_get_preferred_hash_algorithm failed", result);
	}

	/* if algorithm allows alternatives */
	//if (mand == 0 && default_dig != GNUTLS_DIG_UNKNOWN)
	//	dig = default_dig;

	return dig;
}

gnutls_digest_algorithm_t get_dig(gnutls_x509_crt_t crt)
{
	gnutls_digest_algorithm_t dig;
	gnutls_pubkey_t pubkey;
	int result;

	gnutls_pubkey_init(&pubkey);

	result = gnutls_pubkey_import_x509(pubkey, crt, 0);
	if (result < 0) {
		throw CGnuTLSException("gnutls_pubkey_import_x509 failed", result);
	}

	dig = get_dig_for_pub(pubkey);

	gnutls_pubkey_deinit(pubkey);

	return dig;
}


// サーバー証明書を作成する
gnutls_x509_crt_t generate_signed_certificate(gnutls_x509_privkey_t* serverKey, const std::string& host)
{
	gnutls_x509_crt_t crt;
	gnutls_privkey_t key;
	size_t size;
	int result;
	gnutls_privkey_t ca_key = g_ca_key;
	gnutls_x509_crt_t ca_crt = g_ca_crt;

	//fprintf(stdlog, "Generating a signed certificate...\n");

	gnutls_x509_privkey_t x509serverKey = generate_private_key();	// server-key
	*serverKey = x509serverKey;
	CertificateTemplate cfTemplate;
	cfTemplate.InitSignedCertificateTemplate(host, "Proxydomo TLS Server");
	crt = generate_certificate(&key, ca_crt, x509serverKey, cfTemplate);

	/* Copy the CRL distribution points.
	*/
	gnutls_x509_crt_cpy_crl_dist_points(crt, ca_crt);
	/* it doesn't matter if we couldn't copy the CRL dist points.
	*/

	//print_certificate_info(crt, stdlog, 0);

	//fprintf(stdlog, "\n\nSigning certificate...\n");

	result = gnutls_x509_crt_privkey_sign(crt, ca_crt, ca_key, get_dig(ca_crt), 0);
	if (result < 0) {
		throw CGnuTLSException("gnutls_x509_crt_privkey_sign failed", result);
	}
#if 0
	size = lbuffer_size;
	result =
		gnutls_x509_crt_export(crt, outcert_format, lbuffer, &size);
	if (result < 0) {
		fprintf(stderr, "crt_export: %s\n", gnutls_strerror(result));
		exit(1);
	}

	fwrite(lbuffer, 1, size, outfile);
#endif
	gnutls_privkey_deinit(key);

	return crt;
}

// CA証明書を作る
gnutls_x509_crt_t generate_self_signed(gnutls_x509_privkey_t privateKey)
{
	gnutls_x509_crt_t crt;
	gnutls_privkey_t key;
	size_t size;
	int result;

	//fprintf(stdlog, "Generating a self signed certificate...\n");

	CertificateTemplate cfTemplate;
	cfTemplate.InitCACertificateTemplate("Proxydomo CA");
	crt = generate_certificate(&key, NULL, privateKey, cfTemplate);

	//fprintf(stdlog, "\n\nSigning certificate...\n");

	result = gnutls_x509_crt_privkey_sign(crt, crt, key, get_dig(crt), 0);
	if (result < 0) {
		throw CGnuTLSException("gnutls_x509_crt_privkey_sign failed", result);
	}
	gnutls_privkey_deinit(key);
	return crt;
}


}	// namespace


bool	InitSSL()
{
	int ret = gnutls_global_init();
	ATLVERIFY(ret == GNUTLS_E_SUCCESS);
	if (ret != GNUTLS_E_SUCCESS)
		return false;

	CString x509_cafile = Misc::GetExeDirectory() + kCAFileName;
	CString x509_caKeyfile = Misc::GetExeDirectory() + kCAKeyFileName;
	if (::PathFileExists(x509_cafile) == FALSE || ::PathFileExists(x509_caKeyfile) == FALSE) {
		INFO_LOG << L"CA証明書がないのでSSLはフィルタリングできません。";
		return false;
	}

	// server init
	{
		try {
			g_ca_key = load_private_key((LPSTR)CT2A(x509_caKeyfile));
			g_ca_crt = load_ca_cert((LPSTR)CT2A(x509_cafile));
		} catch (CGnuTLSException& e) {
			ERROR_LOG << L"証明書の読み込みに失敗。: " << (LPWSTR)CA2W(e.what());
		}

		// generate_dh_params(void)
		unsigned int bits = gnutls_sec_param_to_pk_bits(GNUTLS_PK_DH, GNUTLS_SEC_PARAM_LEGACY);

		/* Generate Diffie-Hellman parameters - for use with DHE
		* kx algorithms. When short bit length is used, it might
		* be wise to regenerate parameters often.
		*/
		//gnutls_dh_params_t dh_params = NULL;
		//gnutls_dh_params_init(&dh_params);
		//gnutls_dh_params_generate2(dh_params, bits);

		// 暗号の優先度を設定
		ret = gnutls_priority_init(&g_server_priority_cache, 
			"PERFORMANCE:%SERVER_PRECEDENCE:-VERS-SSL3.0", NULL);
		ATLASSERT(ret == GNUTLS_E_SUCCESS);
	}

	// client init
	{
		ret = gnutls_certificate_allocate_credentials(&g_client_cred);
		ATLASSERT(ret == GNUTLS_E_SUCCESS);

		// OSから証明書読み込み
		ret = gnutls_certificate_set_x509_system_trust(g_client_cred);
		ATLASSERT(ret > 0);

		gnutls_certificate_set_verify_function(g_client_cred, verify_certificate_callback);

		// 暗号の優先度を設定 : SSL3.0を無効化
		ret = gnutls_priority_init(&g_client_priority_cache, 
			"NORMAL:%LATEST_RECORD_VERSION:-VERS-SSL3.0", NULL);
		ATLASSERT(ret == GNUTLS_E_SUCCESS);
	}
	return true;
}

void	TermSSL()
{
	gnutls_priority_deinit(g_server_priority_cache);
	g_server_priority_cache = NULL;

	gnutls_x509_crt_deinit(g_ca_crt);
	g_ca_crt = NULL;
	gnutls_privkey_deinit(g_ca_key);
	g_ca_key = NULL;

	gnutls_certificate_free_credentials(g_client_cred);
	g_client_cred = NULL;

	gnutls_priority_deinit(g_client_priority_cache);
	g_client_priority_cache = NULL;

	gnutls_global_deinit();
}


// CA証明書を生成する
void	GenerateCACertificate()
{
	gnutls_x509_privkey_t privateKey = generate_private_key();
	gnutls_x509_crt_t cacrt = generate_self_signed(privateKey);

	enum { kBuffSize = 12 * 1024 };
	std::unique_ptr<char[]>	buff(new char[kBuffSize]);
	size_t size = kBuffSize;
	int result = gnutls_x509_crt_export(cacrt, GNUTLS_X509_FMT_PEM, (void*)buff.get(), &size);
	if (result < 0) {
		throw CGnuTLSException("gnutls_x509_crt_export failed", result);
	}

	{
		std::string caFilePath = CT2A(Misc::GetExeDirectory());
		caFilePath += kCAFileName;
		std::ofstream cafs(caFilePath, std::ios::out | std::ios::binary);
		if (!cafs)
			throw std::exception("cafs file open error");

		cafs.write(buff.get(), size);
		cafs.close();
	}

	size = kBuffSize;
	result = gnutls_x509_privkey_export(privateKey, GNUTLS_X509_FMT_PEM, (void*)buff.get(), &size);
	if (result < 0) {
		throw CGnuTLSException("gnutls_x509_privkey_export failed", result);
	}

	{
		std::string caKeyFilePath = CT2A(Misc::GetExeDirectory());
		caKeyFilePath += kCAKeyFileName;
		std::ofstream cakeyfs(caKeyFilePath, std::ios::out | std::ios::binary);
		if (!cakeyfs)
			throw std::exception("cakeyfs file open error");

		cakeyfs.write(buff.get(), size);
		cakeyfs.close();
	}
}


////////////////////////////////////////////////////////////////
// CSSLSession

CSSLSession::CSSLSession() : 
	m_sock(nullptr), m_session(NULL),
	m_nLastReadCount(0), m_nLastWriteCount(0)
{
}

CSSLSession::~CSSLSession()
{
	Close();
}


std::unique_ptr<CSSLSession>	CSSLSession::InitClientSession(CSocket* sock, const std::string& host)
{
	auto session = std::make_unique<CSSLSession>();
	int ret = 0;

	ret = gnutls_init(&session->m_session, GNUTLS_CLIENT);

	gnutls_session_set_ptr(session->m_session, (void *)host.c_str());

	ret = gnutls_server_name_set(session->m_session, GNUTLS_NAME_DNS, (const void*)host.c_str(), host.length());

	ret = gnutls_priority_set(session->m_session, g_client_priority_cache);
	ATLASSERT(ret == GNUTLS_E_SUCCESS);

	/* put the x509 credentials to the current session
	*/
	ret = gnutls_credentials_set(session->m_session, GNUTLS_CRD_CERTIFICATE, g_client_cred);
	ATLASSERT(ret == GNUTLS_E_SUCCESS);

	sock->SetBlocking(false);
	gnutls_transport_set_int(session->m_session, sock->GetSocket());
	gnutls_handshake_set_timeout(session->m_session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);

	static std::unordered_map<std::string, std::shared_ptr<gnutls_datum_t>>	s_mapHostSessionData;
	static CCriticalSection	s_cs;
	std::shared_ptr<gnutls_datum_t>	psessionCache;
	{
		CCritSecLock lock(s_cs);
		auto itfound = s_mapHostSessionData.find(host);
		if (itfound != s_mapHostSessionData.end()) {
			psessionCache = itfound->second;
			ret = gnutls_session_set_data(session->m_session, psessionCache->data, psessionCache->size);
		}
	}

	// Perform the TLS handshake
	do {
		ret = gnutls_handshake(session->m_session);
	} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0) {
		//fprintf(stderr, "*** Handshake failed\n");
		gnutls_deinit(session->m_session);
		session->m_session = NULL;
		CString error = gnutls_strerror(ret);
		//gnutls_perror(ret);
		//goto end;
		return nullptr;
	} else {

		/* get the session data size */
		std::shared_ptr<gnutls_datum_t>	psessionData(new gnutls_datum_t, 
			[](gnutls_datum_t* data) {
			gnutls_free((void*)data->data);
			delete data;
		});
		ret = gnutls_session_get_data2(session->m_session, psessionData.get());
		ATLASSERT(ret == GNUTLS_E_SUCCESS);

		{
#if 0
			if (psessionCache) {
				if (gnutls_session_is_resumed(session->m_session)) {
					ATLTRACE(_T("session resumed: %s\n"), (LPWSTR)CA2W(host.c_str()));
				}
			}
#endif
			CCritSecLock lock(s_cs);
			s_mapHostSessionData[host] = psessionData;
		}

		char *desc;
		desc = gnutls_session_get_desc(session->m_session);
		//printf("- Session info: %s\n", desc);
		gnutls_free(desc);
	}
	session->m_sock = sock;
	//session->m_sock->SetBlocking(false);
	return session;
}

std::unique_ptr<CSSLSession> CSSLSession::InitServerSession(CSocket* sock, const std::string& host)
{
	auto session = std::make_unique<CSSLSession>();
	int ret = 0;

	gnutls_certificate_credentials_t server_cred;

	static std::unordered_map<std::string, gnutls_certificate_credentials_t>	s_mapHostServerCred;
	static CCriticalSection	s_cs;
	{
		CCritSecLock lock(s_cs);
		auto itfound = s_mapHostServerCred.find(host);
		if (itfound != s_mapHostServerCred.end()) {
			server_cred = itfound->second;

		} else {
			ATLVERIFY(gnutls_certificate_allocate_credentials(&server_cred) == GNUTLS_E_SUCCESS);

			// サーバー証明書生成/登録
			gnutls_x509_privkey_t x509serverKey;
			gnutls_x509_crt_t crt = generate_signed_certificate(&x509serverKey, host);
			ret = gnutls_certificate_set_x509_key(server_cred, &crt, 1, x509serverKey);
			gnutls_x509_privkey_deinit(x509serverKey);
			//ATLASSERT(ret == GNUTLS_E_SUCCESS);

			//gnutls_certificate_set_dh_params(server_cred, dh_params);
			s_mapHostServerCred[host] = server_cred;
		}
	}

	// 暗号の優先度を設定
	gnutls_init(&session->m_session, GNUTLS_SERVER);
	gnutls_priority_set(session->m_session, g_server_priority_cache);
	gnutls_credentials_set(session->m_session, GNUTLS_CRD_CERTIFICATE, server_cred);

	/* We don't request any certificate from the client.
	* If we did we would need to verify it. One way of
	* doing that is shown in the "Verifying a certificate"
	* example.
	*/
	gnutls_certificate_server_set_request(session->m_session, GNUTLS_CERT_IGNORE);

	sock->SetBlocking(false);
	gnutls_transport_set_int(session->m_session, sock->GetSocket());

	do {
		ret = gnutls_handshake(session->m_session);
	} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0) {
		gnutls_deinit(session->m_session);
		session->m_session = NULL;
		CString error = gnutls_strerror(ret);
		return nullptr;

	}

	session->m_sock = sock;
	//session->m_sock->SetBlocking(false);
	return session;
}


bool	CSSLSession::Read(char* buffer, int length)
{
	if (m_session == NULL)
		return false;

	m_nLastReadCount = 0;
	ssize_t ret = gnutls_record_recv(m_session, buffer, length);
	if (ret == 0) {
		//printf("\n- Peer has closed the GnuTLS connection\n");
		Close();
		return true;

	} else if (ret < 0 && gnutls_error_is_fatal(ret) == 0) {
		//fprintf(stderr, "*** Warning: %s\n", gnutls_strerror(ret));
		return false;

	} else if (ret < 0) {
		//fprintf(stderr, "\n*** Received corrupted "
		//	"data(%d). Closing the connection.\n\n", ret);
		Close();
		return false;

	} else {
		ATLASSERT(ret > 0);
		m_nLastReadCount = (int)ret;
		return true;
	}
}

bool	CSSLSession::Write(const char* buffer, int length)
{
	if (m_session == NULL)
		return false;

	m_nLastWriteCount = 0;
	ssize_t ret = 0;
	for (;;) {
		ret = gnutls_record_send(m_session, buffer, length);
		if (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED) {
			::Sleep(10);
		} else {
			break;
		}
	};

	if (ret < 0 && gnutls_error_is_fatal(ret) == 0) {
		//fprintf(stderr, "*** Warning: %s\n", gnutls_strerror(ret));
		return false;
	} else if (ret < 0) {
		Close();
		return false;
	} else {
		m_nLastWriteCount = (int)ret;
		return true;
	}
}


void	CSSLSession::Close()
{
	if (m_session == NULL || m_sock == nullptr)
		return;

	int ret = gnutls_bye(m_session, GNUTLS_SHUT_RDWR);

	try {
		m_sock->Close();
	} catch (SocketException& e) {
		e;
	}
	m_sock = nullptr;

	gnutls_deinit(m_session);
	m_session = NULL;
}









