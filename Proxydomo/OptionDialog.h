/**
*	@file	OptionDialog.h
*	@brief	オプション
*/

#pragma once

#include <atlwin.h>
#include <atlddx.h>
#include "resource.h"
#include "Settings.h"
#include "ssl.h"


class COptionDialog : public CDialogImpl<COptionDialog>, public CWinDataExchange<COptionDialog>
{
public:
	enum { IDD = IDD_OPTION };

	BEGIN_DDX_MAP(CMainDlg)
		DDX_UINT_RANGE(IDC_EDIT_PROXYPORT, CSettings::s_proxyPort, (uint16_t)1024, (uint16_t)65535)

	END_DDX_MAP()

	BEGIN_MSG_MAP(CAboutDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_GENERATE_CACERTIFICATE, OnGenerateCACertificate)
	END_MSG_MAP()

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CenterWindow(GetParent());

		DoDataExchange(DDX_LOAD);
		return TRUE;
	}

	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if (!DoDataExchange(DDX_SAVE))
			return 0;

		CSettings::SaveSettings();

		EndDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
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

private:
	// Data members
};





















