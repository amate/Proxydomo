// Proxydomo.cpp : main source file for Proxydomo.exe
//

#include "stdafx.h"
#include <locale.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>

#include "resource.h"

#include "aboutdlg.h"
#include "MainDlg.h"
#include "Socket.h"
#include "Proxy.h"

// グローバル変数
CAppModule _Module;

#include <vector>
int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainDlg dlgMain;

	if(dlgMain.Create(NULL) == NULL)
	{
		ATLTRACE(_T("Main dialog creation failed!\n"));
		return 0;
	}

	dlgMain.ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

#ifdef _DEBUG
	// ATLTRACEで日本語を使うために必要
	_tsetlocale( LC_ALL, _T("japanese") );
#endif

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

    // リッチエディットコントロール初期化
    HINSTANCE hRich = LoadLibrary(CRichEditCtrl::GetLibraryName());
    if(hRich == NULL){
        AtlMessageBox(NULL, _T("リッチエディットコントロール初期化失敗"),
            _T("エラー"), MB_OK | MB_ICONERROR);
        return 0;
    }


	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	CSocket::Init();

	CProxy	proxy;
	proxy.OpenProxyPort();

	int nRet = Run(lpstrCmdLine, nCmdShow);

	proxy.CloseProxyPort();
	CSocket::Term();

	FreeLibrary(hRich);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
