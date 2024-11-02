/**
*	@file	ConnectionMonitor.h
*/

#pragma once

#include <list>
#include <string>
#include <functional>
#include <atomic>
#include <unordered_set>
#include <array>

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
	uint32_t		uniqueId;
	std::wstring	verb;
	std::wstring	url;
	STEP			outStep;
	STEP			inStep;

	struct Speed {
		std::chrono::seconds	recordingTimeSeconds;
		int	bytes;
	};
	std::array<Speed, 10>	uploadSpeed;
	std::array<Speed, 10>	downloadSpeed;

	std::function<void()>	funcKillConnection;

	std::list<ConnectionData>::iterator	itThis;

	ConnectionData(uint32_t uniqueId);
	ConnectionData(const ConnectionData& conData);
	ConnectionData& operator= (const ConnectionData& conData);

	void	SetVerb(const std::wstring& verb);
	void	SetUrl(const std::wstring& url);
	void	SetOutStep(STEP out);
	void	SetInStep(STEP in);

	void	SetUpload(int bytes);
	void	SetDownload(int bytes);
};


class CConnectionManager
{
public:
	enum UpdateCategory {
		kAddConnection,
		kRemoveConnection,
		kUpdateConnection,
	};

	static	ConnectionData*	CreateConnectionData(std::function<void()>	funcKillConnection);
	static	void			RemoveConnectionData(ConnectionData* conData);

	// for CConnectionManagerWindow
	static void RegisterCallback(std::function<void(ConnectionData*, UpdateCategory)> callback);
	static void UnregisterCallback();

	// for ConnectionData
	static void UpdateNotify(ConnectionData* conData);

	static CCriticalSection				s_csList;
	static std::list<ConnectionData>	s_connectionDataList;
	static std::function<void(ConnectionData*, UpdateCategory)> s_funcCallback;
	static std::atomic_uint32_t	s_uniqueIdGenerator;
};


class CConnectionMonitorWindow :
	public CDialogImpl<CConnectionMonitorWindow>,
	public CDialogResize<CConnectionMonitorWindow>,
	public CCustomDraw<CConnectionMonitorWindow>
{
public:
	enum { IDD = IDD_CONNECTIONMONITOR };

	enum { 
		WM_UPDATENOTIFY = WM_APP + 1,

		kUpdateSpeedTimerId = 1,
		kUpdateSpeedTimerInterval = 1000,
	};

	CConnectionMonitorWindow();

	void	ShowWindow();

	BEGIN_DLGRESIZE_MAP(CConnectionMonitorWindow)
		DLGRESIZE_CONTROL(IDC_LIST_CONNECTION, DLSZ_SIZE_X | DLSZ_SIZE_Y)

	END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP_EX(CConnectionMonitorWindow)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_DESTROY(OnDestroy)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
		MESSAGE_HANDLER_EX(WM_UPDATENOTIFY, OnUpdateNotify)
		MSG_WM_TIMER(OnTimer)

		NOTIFY_HANDLER_EX(IDC_LIST_CONNECTION, NM_RCLICK, OnConnectionListRClick)
		NOTIFY_HANDLER_EX(IDC_LIST_CONNECTION, LVN_KEYDOWN, OnConnectionListKeyDown)

		CHAIN_MSG_MAP(CDialogResize<CConnectionMonitorWindow>)
		CHAIN_MSG_MAP(CCustomDraw<CConnectionMonitorWindow>)
	END_MSG_MAP()

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnDestroy();
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);
	LRESULT OnUpdateNotify(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void OnTimer(UINT_PTR nIDEvent);
	LRESULT OnConnectionListRClick(LPNMHDR pnmh);
	LRESULT OnConnectionListKeyDown(LPNMHDR pnmh);

	// CCustomDraw
	DWORD OnPrePaint(int nID, LPNMCUSTOMDRAW lpnmcd);
	DWORD OnItemPrePaint(int nID, LPNMCUSTOMDRAW lpnmcd);

private:

	void	_UpdateSpeedTimer();
	ConnectionData* _GetConnectionDataFromUniqueId(uint32_t uniqueId);


	CListViewCtrl	m_connectionListView;

	struct ConnectionDataOperation
	{
		ConnectionData conData;
		CConnectionManager::UpdateCategory	updateCategory;

		ConnectionDataOperation(ConnectionData* conData, CConnectionManager::UpdateCategory	updateCategory) :
			conData(*conData), updateCategory(updateCategory)
		{}
	};
	CCriticalSection m_csConnectionDataOperationList;
	std::list<ConnectionDataOperation>	m_connectionDataOperationList;

	std::list<uint32_t>	m_connectionCloseList;
	std::atomic_bool	m_listActive;
	std::unordered_set<uint32_t>	m_idleConnections;
};









