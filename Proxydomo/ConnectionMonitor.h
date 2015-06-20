/**
*	@file	ConnectionMonitor.h
*/

#pragma once

#include <list>
#include <string>
#include <functional>
#include <atomic>
#include <atlcrack.h>
#include <atlctrls.h>
#include <atlframe.h>
#include <atlsync.h>
#include <atlctrlx.h>
#include "resource.h"

enum class STEP;

struct ConnectionData
{
	CCriticalSection	cs;
	std::wstring	verb;
	std::wstring	url;
	STEP			outStep;
	STEP			inStep;

	std::list<ConnectionData>::iterator	itThis;

	ConnectionData();

	void	SetVerb(const std::wstring& verb);
	void	SetUrl(const std::wstring& url);
	void	SetOutStep(STEP out);
	void	SetInStep(STEP in);
};


class CConnectionManager
{
public:
	enum UpdateCategory {
		kAddConnection,
		kRemoveConnection,
		kUpdateConnection,
	};

	static	ConnectionData*	CreateConnectionData();
	static	void			RemoveConnectionData(ConnectionData* conData);

	// for CConnectionManagerWindow
	static void RegisterCallback(std::function<void(ConnectionData*, UpdateCategory)> callback);
	static void UnregisterCallback();

	// for ConnectionData
	static void UpdateNotify(ConnectionData* conData);

	static CCriticalSection				s_csList;
	static std::list<ConnectionData>	s_connectionDataList;
	static std::function<void(ConnectionData*, UpdateCategory)> s_funcCallback;
};


class CConnectionMonitorWindow :
	public CDialogImpl<CConnectionMonitorWindow>,
	public CDialogResize<CConnectionMonitorWindow>,
	public CCustomDraw<CConnectionMonitorWindow>
{
public:
	enum { IDD = IDD_CONNECTIONMONITOR };

	void	ShowWindow();

	BEGIN_DLGRESIZE_MAP(CConnectionMonitorWindow)
		DLGRESIZE_CONTROL(IDC_LIST_CONNECTION, DLSZ_SIZE_X | DLSZ_SIZE_Y)

	END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP_EX(CConnectionMonitorWindow)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_DESTROY(OnDestroy)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)

		MSG_WM_TIMER(OnTimer)

		NOTIFY_HANDLER_EX(IDC_LIST_CONNECTION, NM_RCLICK, OnConnectionListRClick)

		CHAIN_MSG_MAP(CDialogResize<CConnectionMonitorWindow>)
		CHAIN_MSG_MAP(CCustomDraw<CConnectionMonitorWindow>)
		END_MSG_MAP()

		BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
		void OnDestroy();
		void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);
		void OnTimer(UINT_PTR nIDEvent);
		LRESULT OnConnectionListRClick(LPNMHDR pnmh);

		// CCustomDraw
		DWORD OnPrePaint(int nID, LPNMCUSTOMDRAW lpnmcd);
		DWORD OnItemPrePaint(int nID, LPNMCUSTOMDRAW lpnmcd);

private:
	CListViewCtrl	m_connectionListView;

	CCriticalSection m_csConnectionCloseList;
	std::list<int>	m_connectionCloseList;
	std::atomic_bool	m_listActive;
};









