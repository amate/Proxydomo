/**
*	@file	OptionDialog.h
*	@brief	オプション
*/

#pragma once

#include <memory>
#include <thread>
#include <chrono>
#include <atlwin.h>
#include <atlddx.h>
#include "resource.h"
#include "Settings.h"
#include "ssl.h"
#include "WinHTTPWrapper.h"


class COptionDialog : public CDialogImpl<COptionDialog>, public CWinDataExchange<COptionDialog>
{
public:
	enum { IDD = IDD_OPTION };

	BEGIN_DDX_MAP(CMainDlg)
		DDX_UINT_RANGE(IDC_EDIT_PROXYPORT, CSettings::s_proxyPort, (uint16_t)1024, (uint16_t)65535)

		DDX_CONTROL_HANDLE(IDC_COMBO_REMOTEPROXY, m_cmbRemoteProxy)
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
		CenterWindow(GetParent());

		DoDataExchange(DDX_LOAD);

		m_cmbRemoteProxy.SetWindowText(CA2W(CSettings::s_defaultRemoteProxy.c_str()));
		m_setRemoteProxy = CSettings::s_setRemoteProxy;
		for (auto& remoteproxy : CSettings::s_setRemoteProxy)
			m_cmbRemoteProxy.AddString(CA2W(remoteproxy.c_str()));
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
		int ret = MessageBox(_T("SSLフィルタリングで使用するCA証明書を作成します。"), _T("確認"), MB_OKCANCEL);
		if (ret == IDCANCEL)
			return 0;

		try {
			GenerateCACertificate();
		} catch (std::exception& e) {
			CString errormsg = L"証明書の作成に失敗しました。: ";
			errormsg += e.what();
			MessageBox(errormsg, _T("エラー"), MB_ICONERROR);
			return 0;
		}
		MessageBox(_T("CA証明書の作成に成功しました！\nSSLのフィルタリングはProxydomo再起動後に有効になります\nSSLフィルタリングを無効にしたい場合はフォルダ内に生成された[ca.pem.crt]と[ca-key.pem]を削除してください。"), _T("成功"));
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
			testLog.SetWindowText(_T("テストするProxyをセットしてください。"));
			return 0;
		}

		if (m_threadTestProxy.joinable()) {
			*m_spThreadActive = false;
			m_threadTestProxy.detach();
		}
		testLog.SetWindowText(_T("Proxyのテストを開始します..."));

		auto spThreadActive = std::make_shared<bool>(true);
		m_spThreadActive = spThreadActive;
		m_threadTestProxy = std::thread([testLog, remoteProxy, spThreadActive]() {
			{
				using namespace std::chrono;
				auto start = steady_clock::now();
				CStatic resultLog = testLog;
				bool bSuccess = false;
				if (WinHTTPWrapper::InitWinHTTP(boost::none, remoteProxy)) {
					if (*spThreadActive) {
						if (auto downloadData = WinHTTPWrapper::HttpDownloadData(L"http://www.google.co.jp/")) {
							bSuccess = true;
							if (*spThreadActive) {
								long long processingTime = duration_cast<milliseconds>(steady_clock::now() - start).count();
								CString result;
								result.Format(_T("成功しました！ : %lld ms"), processingTime);
								resultLog.SetWindowText(result);
							}
						}
					}
					WinHTTPWrapper::TermWinHTTP();
				}
				if (bSuccess == false) {
					if (*spThreadActive) {
						resultLog.SetWindowText(_T("失敗しました..."));
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
	std::set<std::string>	m_setRemoteProxy;
	std::shared_ptr<bool>	m_spThreadActive;
	std::thread	m_threadTestProxy;
};





















