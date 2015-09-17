/**
*	@file	OptionDialog.h
*	@brief	ƒIƒvƒVƒ‡ƒ“
*/

#pragma once

#include <memory>
#include <thread>
#include <chrono>
#include <list>
#include <atlwin.h>
#include <atlddx.h>
#include "resource.h"
#include "Settings.h"
#include "ssl.h"
#include "WinHTTPWrapper.h"
#include "UITranslator.h"
#include "Misc.h"


class COptionDialog : public CDialogImpl<COptionDialog>, public CWinDataExchange<COptionDialog>
{
public:
	enum { IDD = IDD_OPTION };

	BEGIN_DDX_MAP(CMainDlg)
		DDX_UINT_RANGE(IDC_EDIT_PROXYPORT, CSettings::s_proxyPort, (uint16_t)1024, (uint16_t)65535)
		DDX_CHECK(IDC_CHECK_PRIVATECONNECTION, CSettings::s_privateConnection)

		DDX_CONTROL_HANDLE(IDC_COMBO_REMOTEPROXY, m_cmbRemoteProxy)
		DDX_CONTROL_HANDLE(IDC_COMBO_LANG, m_cmbLang)

		DDX_CHECK(IDC_CHECK_TASKTRAYONCLOSEBOTTON, CSettings::s_tasktrayOnCloseBotton)
	END_DDX_MAP()

	BEGIN_MSG_MAP(CAboutDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_GENERATE_CACERTIFICATE, OnGenerateCACertificate)

		COMMAND_ID_HANDLER(IDC_BUTTON_ADDPROXYLIST, OnAddProxyList)
		COMMAND_ID_HANDLER(IDC_BUTTON_DELETEFROMPROXYLIST, OnDeleteFromProxyList)
		COMMAND_ID_HANDLER(IDC_BUTTON_TESTREMOTEPROXY, OnTestRemoteProxy)
	END_MSG_MAP()

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		SetWindowText(UITranslator::GetTranslateMessage(IDD_OPTION).c_str());

		UITranslator::ChangeControlTextForTranslateMessage(m_hWnd, IDC_GROUP_PROXYPORT);
		UITranslator::ChangeControlTextForTranslateMessage(m_hWnd, IDC_CHECK_PRIVATECONNECTION);
		UITranslator::ChangeControlTextForTranslateMessage(m_hWnd, IDC_STATIC_ENABLEONREBOOT);
		UITranslator::ChangeControlTextForTranslateMessage(m_hWnd, IDC_GROUP_SSLFILTERING);
		UITranslator::ChangeControlTextForTranslateMessage(m_hWnd, IDC_GENERATE_CACERTIFICATE);
		UITranslator::ChangeControlTextForTranslateMessage(m_hWnd, IDC_GROUP_REMOTEHTTPPROXY);
		UITranslator::ChangeControlTextForTranslateMessage(m_hWnd, IDC_BUTTON_ADDPROXYLIST);
		UITranslator::ChangeControlTextForTranslateMessage(m_hWnd, IDC_BUTTON_DELETEFROMPROXYLIST);
		UITranslator::ChangeControlTextForTranslateMessage(m_hWnd, IDC_BUTTON_TESTREMOTEPROXY);
		UITranslator::ChangeControlTextForTranslateMessage(m_hWnd, IDC_STATIC_TESTREMOTEPROXY);
		UITranslator::ChangeControlTextForTranslateMessage(m_hWnd, IDC_GROUP_LANGSETTING);
		UITranslator::ChangeControlTextForTranslateMessage(m_hWnd, IDC_STATIC_ENABLEONREBOOT2);
		UITranslator::ChangeControlTextForTranslateMessage(m_hWnd, IDC_CHECK_TASKTRAYONCLOSEBOTTON);		
		UITranslator::ChangeControlTextForTranslateMessage(m_hWnd, IDOK);
		UITranslator::ChangeControlTextForTranslateMessage(m_hWnd, IDCANCEL);
		
		CenterWindow(GetParent());

		DoDataExchange(DDX_LOAD);

		m_cmbRemoteProxy.SetWindowText(CA2W(CSettings::s_defaultRemoteProxy.c_str()));
		m_setRemoteProxy = CSettings::s_setRemoteProxy;
		for (auto& remoteproxy : CSettings::s_setRemoteProxy)
			m_cmbRemoteProxy.AddString(CA2W(remoteproxy.c_str()));

		std::list<std::wstring> langList;
		ForEachFile(Misc::GetExeDirectory() + L"lang\\", [&langList](const CString& filePath) {
			CString ext = Misc::GetFileExt(filePath);
			ext.MakeLower();
			if (ext == L"lng")
				langList.push_back((LPCWSTR)Misc::GetFileBaseNoExt(filePath));
		});
		langList.sort();
		for (auto& lang : langList) {
			int sel = m_cmbLang.AddString(lang.c_str());
			if (::_wcsicmp(CSettings::s_language.c_str(), lang.c_str()) == 0)
				m_cmbLang.SetCurSel(sel);
		}
		return TRUE;
	}

	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if (!DoDataExchange(DDX_SAVE))
			return 0;

		CString defaultRemoteProxy;
		m_cmbRemoteProxy.GetWindowText(defaultRemoteProxy.GetBuffer(256), 256);
		defaultRemoteProxy.ReleaseBuffer();
		CSettings::s_defaultRemoteProxy = (LPCSTR)CW2A(defaultRemoteProxy);
		CSettings::s_setRemoteProxy = m_setRemoteProxy;

		int nSel = m_cmbLang.GetCurSel();
		if (nSel != -1) {
			CString lang;
			m_cmbLang.GetLBText(nSel, lang);
			CSettings::s_language = (LPCWSTR)lang;
		}

		CSettings::SaveSettings();

		CloseDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CloseDialog(wID);
		return 0;
	}

	LRESULT OnGenerateCACertificate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		int ret = MessageBox(UITranslator::GetTranslateMessage(ID_CREATECACONFIRMMESSAGE).c_str(), UITranslator::GetTranslateMessage(ID_TRANS_CONFIRM).c_str(), MB_OKCANCEL);
		if (ret == IDCANCEL)
			return 0;

		ret = MessageBox(UITranslator::GetTranslateMessage(ID_CHOICEPRIVATEKEYMESSAGE).c_str(), UITranslator::GetTranslateMessage(ID_TRANS_CONFIRM).c_str(), MB_YESNOCANCEL);
		if (ret == IDCANCEL)
			return 0;

		bool rsa = (ret != IDYES);
		try {
			GenerateCACertificate(rsa);
		} catch (std::exception& e) {
			CString errormsg = UITranslator::GetTranslateMessage(ID_CREATECAFAILEDMESSAGE).c_str();
			errormsg += e.what();
			MessageBox(errormsg, UITranslator::GetTranslateMessage(ID_TRANS_ERROR).c_str(), MB_ICONERROR);
			return 0;
		}
		MessageBox(UITranslator::GetTranslateMessage(ID_CREATECASUCCEEDEDMESSAGE).c_str(), UITranslator::GetTranslateMessage(ID_TRANS_SUCCESS).c_str());
		return 0;
	}

	LRESULT OnAddProxyList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CString remoteProxy;
		m_cmbRemoteProxy.GetWindowText(remoteProxy.GetBuffer(256), 256);
		remoteProxy.ReleaseBuffer();
		auto it = m_setRemoteProxy.insert((LPCSTR)CW2A(remoteProxy));
		if (it.second)
			m_cmbRemoteProxy.AddString(remoteProxy);
		return 0;
	}

	LRESULT OnDeleteFromProxyList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		int nSel = m_cmbRemoteProxy.GetCurSel();
		if (nSel == -1)
			return 0;

		CString remoteProxy;
		m_cmbRemoteProxy.GetLBText(nSel, remoteProxy);
		remoteProxy.ReleaseBuffer();
		m_cmbRemoteProxy.DeleteString(nSel);

		m_setRemoteProxy.erase((LPCSTR)CW2A(remoteProxy));
		return 0;
	}

	LRESULT OnTestRemoteProxy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CStatic testLog = GetDlgItem(IDC_STATIC_TESTREMOTEPROXY);

		CString remoteProxy;
		m_cmbRemoteProxy.GetWindowText(remoteProxy.GetBuffer(256), 256);
		remoteProxy.ReleaseBuffer();
		if (remoteProxy.IsEmpty()) {
			testLog.SetWindowText(UITranslator::GetTranslateMessage(ID_TESTREMOTEPROXYEMPTYMESSAGE).c_str());
			return 0;
		}

		if (m_threadTestProxy.joinable()) {
			*m_spThreadActive = false;
			m_threadTestProxy.detach();
		}
		testLog.SetWindowText(UITranslator::GetTranslateMessage(ID_PROXYTESTSTARTMESSAGE).c_str());

		auto spThreadActive = std::make_shared<bool>(true);
		m_spThreadActive = spThreadActive;
		m_threadTestProxy = std::thread([testLog, remoteProxy, spThreadActive]() {
			{
				using namespace std::chrono;
				auto start = steady_clock::now();
				CStatic resultLog = testLog;
				bool bSuccess = false;
				if (*spThreadActive) {
					auto funcHttpDownloadData = [remoteProxy]() -> boost::optional<std::string> {
						try {
							WinHTTPWrapper::CUrl	downloadUrl("http://www.google.com/");
							auto hConnect = WinHTTPWrapper::HttpConnect(downloadUrl);
							auto hRequest = WinHTTPWrapper::HttpOpenRequest(downloadUrl, hConnect);
							WinHTTPWrapper::HttpSetProxy(hRequest, remoteProxy);
							if (WinHTTPWrapper::HttpSendRequestAndReceiveResponse(hRequest))
								return HttpReadData(hRequest);
						}
						catch (boost::exception& e) {
							std::string expText = boost::diagnostic_information(e);
							int a = 0;
						}
						return boost::none;
					};

					if (auto downloadData = funcHttpDownloadData()) {
						bSuccess = true;
						if (*spThreadActive) {
							long long processingTime = duration_cast<milliseconds>(steady_clock::now() - start).count();
							resultLog.SetWindowText(UITranslator::GetTranslateMessage(ID_PROXYTESTSUCCEEDEDMESSAGE, processingTime).c_str());
						}
					}
				}

				if (bSuccess == false) {
					if (*spThreadActive) {
						resultLog.SetWindowText(UITranslator::GetTranslateMessage(ID_PROXYTESTFAILEDMESSAGE).c_str());
					}
				}
			}
		});


		return 0;
	}

private:
	void	CloseDialog(WORD wID)
	{
		if (m_threadTestProxy.joinable()) {
			*m_spThreadActive = false;
			m_threadTestProxy.detach();
		}
		EndDialog(wID);
	}

	// Data members
	CComboBox	m_cmbRemoteProxy;
	CComboBox	m_cmbLang;
	std::set<std::string>	m_setRemoteProxy;
	std::shared_ptr<bool>	m_spThreadActive;
	std::thread	m_threadTestProxy;
};





















