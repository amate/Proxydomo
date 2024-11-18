/**
*	@file	ConnectionMonitor.cpp
*/

#include "stdafx.h"
#include "ConnectionMonitor.h"
#include <thread>
#include <boost\property_tree\ini_parser.hpp>
#include <boost\property_tree\ptree.hpp>
#include <boost\algorithm\string.hpp>
#include "RequestManager.h"
#include "Misc.h"
#include "Logger.h"

namespace {

	std::chrono::seconds GetNowSeconds()
	{
		auto nowSeconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch());
		return nowSeconds;
	}

}


/////////////////////////////////////////////////////////
// ConnectionData

ConnectionData::ConnectionData(uint32_t uniqueId) : uniqueId(uniqueId), inStep(STEP::STEP_START), outStep(STEP::STEP_START)
{
}

ConnectionData::ConnectionData(const ConnectionData& conData)
{
	*this = conData;
}

ConnectionData& ConnectionData::operator= (const ConnectionData& conData)
{
	uniqueId = conData.uniqueId;
	verb = conData.verb;
	url = conData.url;
	inStep = conData.inStep;
	outStep = conData.outStep;
	trafficData = conData.trafficData;
	return *this;
}

void	ConnectionData::SetVerb(const std::wstring& verb)
{
	CCritSecLock lock(cs);
	this->verb = verb;
	ConnectionData conData(*this);
	lock.Unlock();
	CConnectionManager::UpdateNotify(&conData, CConnectionManager::kUpdateConnection);
}

void	ConnectionData::SetUrl(const std::wstring& url)
{
	CCritSecLock lock(cs);
	this->url = url;
	ConnectionData conData(*this);
	lock.Unlock();
	CConnectionManager::UpdateNotify(&conData, CConnectionManager::kUpdateConnection);
}

void	ConnectionData::SetInStep(STEP in)
{
	inStep = in;
	CConnectionManager::UpdateNotify(this, CConnectionManager::kUpdateConnection);
}

void	ConnectionData::SetOutStep(STEP out)
{
	outStep = out;
	CConnectionManager::UpdateNotify(this, CConnectionManager::kUpdateConnection);
}

bool	_setSpeed(std::array<ConnectionData::TrafficData::Speed, 10>& speed, int bytes)
{
	bool timeChanged = false;
	const auto nowSeconds = GetNowSeconds();
	const int indexSeconds = nowSeconds.count() % 10;
	if (speed[indexSeconds].recordingTimeSeconds != nowSeconds) {
		speed[indexSeconds].recordingTimeSeconds = nowSeconds;
		speed[indexSeconds].bytes = 0;
		timeChanged = true;
	}
	speed[indexSeconds].bytes += bytes;
	return timeChanged;
}

void ConnectionData::SetUpload(int bytes)
{
	CCritSecLock lock(cs);
	if (_setSpeed(trafficData.uploadSpeed, bytes)) {
		CConnectionManager::UpdateNotify(this, CConnectionManager::kUpdateTraffic);
	}
}

void ConnectionData::SetDownload(int bytes)
{
	CCritSecLock lock(cs);
	if (_setSpeed(trafficData.downloadSpeed, bytes)) {
		CConnectionManager::UpdateNotify(this, CConnectionManager::kUpdateTraffic);
	}
}




/////////////////////////////////////////////////////////
// CConnectionManager

CCriticalSection			CConnectionManager::s_csList;
std::list<ConnectionData>	CConnectionManager::s_connectionDataList;
std::function<void(ConnectionData*, CConnectionManager::UpdateCategory)> CConnectionManager::s_funcCallback;
std::atomic_uint32_t	CConnectionManager::s_uniqueIdGenerator = 10;

ConnectionData*	CConnectionManager::CreateConnectionData(std::function<void()>	funcKillConnection)
{
	CCritSecLock lock(s_csList);
	s_connectionDataList.emplace_front(s_uniqueIdGenerator++);
	ConnectionData* conData = &s_connectionDataList.front();
	conData->itThis = s_connectionDataList.begin();
	conData->funcKillConnection = funcKillConnection;
	if (s_funcCallback) {
		s_funcCallback(conData, kAddConnection);
	}
	return conData;
}
void			CConnectionManager::RemoveConnectionData(ConnectionData* conData)
{
	CCritSecLock lock(s_csList);
	if (s_funcCallback) {
		s_funcCallback(conData, kRemoveConnection);
	}
	s_connectionDataList.erase(conData->itThis);

}

// for CConnectionManagerWindow
void CConnectionManager::RegisterCallback(std::function<void(ConnectionData*, UpdateCategory)> callback)
{
	CCritSecLock lock(s_csList);
	s_funcCallback = callback;
	for (auto rit = s_connectionDataList.rbegin(); rit != s_connectionDataList.rend(); ++rit) {
		CCritSecLock lock(rit->cs);
		auto conData = *rit;
		lock.Unlock();
		s_funcCallback(&conData, kAddConnection);
	}

}
void CConnectionManager::UnregisterCallback()
{
	CCritSecLock lock(s_csList);
	s_funcCallback = nullptr;
}

// for ConnectionData
void CConnectionManager::UpdateNotify(ConnectionData* conData, UpdateCategory category)
{
	CCritSecLock lock(s_csList);
	if (s_funcCallback) {
		s_funcCallback(conData, category);
	}
}




///////////////////////////////////////////////////////
// CConnectionMonitorWindow

CConnectionMonitorWindow::CConnectionMonitorWindow()
{
}

void	CConnectionMonitorWindow::ShowWindow()
{
	if (IsWindowVisible() == FALSE) {
		m_listActive = true;
		CConnectionManager::RegisterCallback(
			[this](ConnectionData* conData, CConnectionManager::UpdateCategory updateCategory)
		{
			if (m_listActive == false)
				return;

			int connectionCount = -1;
			{
				CCritSecLock lock(m_csConnectionDataOperationList);
				if (updateCategory == CConnectionManager::kUpdateConnection || updateCategory == CConnectionManager::kUpdateTraffic) {
					for (auto& conDataOperation : m_connectionDataOperationList) {
						if (conDataOperation.conData.uniqueId == conData->uniqueId
							&& conDataOperation.updateCategory == updateCategory)
						{
							conDataOperation.conData = *conData;
							return;	// ÇQèdÇ…ÇÕìoò^ÇµÇ»Ç¢
						}
					}
				} else {
					connectionCount = (int)CConnectionManager::s_connectionDataList.size();
					if (updateCategory == CConnectionManager::kRemoveConnection) {
						--connectionCount;
					}
				}
				m_connectionDataOperationList.emplace_back(conData, updateCategory);
				PostMessage(WM_UPDATENOTIFY, connectionCount);
			}
		});
	}

	{
		CCritSecLock lock(CConnectionManager::s_csList);
		m_lastConnectionCount = static_cast<int>(CConnectionManager::s_connectionDataList.size());
		lock.Unlock();

		_UpdateTitleText();
	}

	SetTimer(kUpdateSpeedTimerId, kUpdateSpeedTimerInterval);

	__super::ShowWindow(TRUE);
}


BOOL CConnectionMonitorWindow::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	m_connectionListView = GetDlgItem(IDC_LIST_CONNECTION);
	m_connectionListView.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER);

	struct Column {
		LPCTSTR	columnName;
		int		columnFormat;
		int		defaultcolumnWidth;
	};
	Column	clm[] = {
		{ _T("outStep"), LVCFMT_LEFT, 60 },
		{ _T("InStep"), LVCFMT_LEFT, 60 },
		{ _T("Verb"), LVCFMT_LEFT, 50 },
		{ _T("URL"), LVCFMT_LEFT, 300 },
		{ _T("Upload"), LVCFMT_LEFT, 100 },
		{ _T("Download"), LVCFMT_LEFT, 100 },
	};

	int nCount = _countof(clm);
	for (int i = 0; i < nCount; ++i) {
		m_connectionListView.InsertColumn(i, clm[i].columnName, clm[i].columnFormat, clm[i].defaultcolumnWidth);
	}

	DlgResize_Init(true, false, WS_THICKFRAME);

	using namespace boost::property_tree;

	CString settingsPath = Misc::GetExeDirectory() + _T("settings.ini");
	std::ifstream fs(settingsPath);
	if (fs) {
		ptree pt;
		try {
			read_ini(fs, pt);
		}
		catch (...) {
			ERROR_LOG << L"CConnectionMonitorWindow::OnInitDialog : settings.iniÇÃì«Ç›çûÇ›Ç…é∏îs";
			pt.clear();
		}
		CRect rcWindow;
		rcWindow.top = pt.get("ConnectionMonitorWindow.top", 0);
		rcWindow.left = pt.get("ConnectionMonitorWindow.left", 0);
		rcWindow.right = pt.get("ConnectionMonitorWindow.right", 0);
		rcWindow.bottom = pt.get("ConnectionMonitorWindow.bottom", 0);
		if (rcWindow != CRect()) {
			MoveWindow(&rcWindow);
		}

		if (auto value = pt.get_optional<std::string>("ConnectionMonitorWindow.ColumnWidth")) {
			std::vector<std::string> columnWidth;
			boost::algorithm::split(columnWidth, *value, boost::is_any_of(","));
			const int size = static_cast<int>(columnWidth.size());
			for (int i = 0; i < size; ++i) {
				m_connectionListView.SetColumnWidth(i, std::stoi(columnWidth[i]));
			}
		}
		if (auto value = pt.get_optional<std::string>("ConnectionMonitorWindow.ColumnOrder")) {
			std::vector<std::string> columnOrder;
			boost::algorithm::split(columnOrder, *value, boost::is_any_of(","));
			const int size = static_cast<int>(columnOrder.size());
			auto order = std::make_unique<int[]>(size);
			for (int i = 0; i < size; ++i) {
				order[i] = std::stoi(columnOrder[i]);
			}
			m_connectionListView.SetColumnOrderArray(size, order.get());
		}
	}

	return TRUE;
}

void CConnectionMonitorWindow::OnDestroy()
{
	OnCancel(0, 0, NULL);

	using namespace boost::property_tree;

	std::string settingsPath = CT2A(Misc::GetExeDirectory() + _T("settings.ini"));
	ptree pt;
	try {
		read_ini(settingsPath, pt);
	}
	catch (...) {
		ERROR_LOG << L"CConnectionMonitorWindow::OnDestroy : settings.iniÇÃì«Ç›çûÇ›Ç…é∏îs";
		pt.clear();
	}

	CRect rcWindow;
	GetWindowRect(&rcWindow);

	pt.put("ConnectionMonitorWindow.top", rcWindow.top);
	pt.put("ConnectionMonitorWindow.left", rcWindow.left);
	pt.put("ConnectionMonitorWindow.right", rcWindow.right);
	pt.put("ConnectionMonitorWindow.bottom", rcWindow.bottom);

	const int columnCount = m_connectionListView.GetHeader().GetItemCount();
	std::string columnWidth;
	for (int i = 0; i < columnCount; ++i) {
		int width = m_connectionListView.GetColumnWidth(i);
		columnWidth += std::to_string(width);
		if (i + 1 != columnCount)
			columnWidth += ",";
	}
	pt.put("ConnectionMonitorWindow.ColumnWidth", columnWidth);

	auto order = std::make_unique<int[]>(columnCount);
	m_connectionListView.GetColumnOrderArray(columnCount, order.get());
	std::string columnOrder;
	for (int i = 0; i < columnCount; ++i) {
		columnOrder += std::to_string(order[i]);
		if (i + 1 != columnCount)
			columnOrder += ",";
	}
	pt.put("ConnectionMonitorWindow.ColumnOrder", columnOrder);

	write_ini(settingsPath, pt);
}

void CConnectionMonitorWindow::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	m_listActive = false;
	CConnectionManager::UnregisterCallback();

	KillTimer(kUpdateSpeedTimerId);

	__super::ShowWindow(FALSE);

	{
		CCritSecLock lock(m_csConnectionDataOperationList);
		m_connectionDataOperationList.clear();
	}

	m_connectionListView.DeleteAllItems();

	m_connectionTrafficDataList.clear();

	for (uint32_t closeItem : m_connectionCloseList) {
		KillTimer((UINT_PTR)closeItem);
	}
	m_connectionCloseList.clear();

	m_idleConnections.clear();

	m_lastUploadDownloadTrafficText.Empty();
}


LRESULT CConnectionMonitorWindow::OnUpdateNotify(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const int connectionCount = (int)wParam;
	if (connectionCount != -1) {
		m_lastConnectionCount = connectionCount;
		_UpdateTitleText();
	}

	CCritSecLock lock(m_csConnectionDataOperationList);
	for (auto& conDataOperation : m_connectionDataOperationList) {
		auto conData = &conDataOperation.conData;
		auto updateCategory = conDataOperation.updateCategory;

		auto funcSetItem = [this, conData](int iItem) {
			LVITEM	Item = {};
			Item.mask = LVIF_TEXT;
			Item.iItem = iItem;

			Item.iSubItem = 0;
			Item.pszText = (LPWSTR)STEPtoString(conData->outStep);
			m_connectionListView.SetItem(&Item);

			Item.iSubItem = 1;
			Item.pszText = (LPWSTR)STEPtoString(conData->inStep);
			m_connectionListView.SetItem(&Item);

			Item.iSubItem = 2;
			Item.pszText = (LPWSTR)conData->verb.c_str();
			m_connectionListView.SetItem(&Item);

			Item.iSubItem = 3;
			Item.pszText = (LPWSTR)conData->url.c_str();
			m_connectionListView.SetItem(&Item);
		};

		auto funcGetIndex = [this, conData]() -> int {
			int count = m_connectionListView.GetItemCount();
			for (int i = 0; i < count; ++i) {
				if (conData->uniqueId == (uint32_t)m_connectionListView.GetItemData(i)) {
					return i;
				}
			}
			ATLASSERT(FALSE);
			return -1;
		};

		switch (updateCategory) {
		case CConnectionManager::kAddConnection:
		{
			if (conData->inStep == STEP::STEP_START && conData->outStep == STEP::STEP_START) {
				m_idleConnections.insert(conData->uniqueId);
			} else {
				m_idleConnections.erase(conData->uniqueId);
			}
			m_connectionListView.InsertItem(0, _T(""));
			m_connectionListView.SetItemData(0, (DWORD_PTR)conData->uniqueId);
			funcSetItem(0);
		}
		break;

		case CConnectionManager::kUpdateConnection:
		{
			int i = funcGetIndex();
			if (i != -1) {
				if (conData->inStep == STEP::STEP_START && conData->outStep == STEP::STEP_START) {
					m_idleConnections.insert(conData->uniqueId);
				} else {
					m_idleConnections.erase(conData->uniqueId);
				}
				funcSetItem(i);
			}
		}
		break;

		case CConnectionManager::kUpdateTraffic:
		{
			m_connectionTrafficDataList[conData->uniqueId] = conData->trafficData;
		}
		break;

		case CConnectionManager::kRemoveConnection:
		{
			int i = funcGetIndex();
			if (i != -1) {
				m_connectionCloseList.emplace_front(conData->uniqueId);
				m_idleConnections.erase(conData->uniqueId);

				RECT rcItem;
				m_connectionListView.GetItemRect(i, &rcItem, LVIR_BOUNDS);
				m_connectionListView.InvalidateRect(&rcItem);
				SetTimer(conData->uniqueId, 1500);	// íxâÑÇ≥ÇπÇƒÇ©ÇÁè¡Ç∑

				m_connectionTrafficDataList.erase(conData->uniqueId);
			}
		}
		break;

		}
	}
	m_connectionDataOperationList.clear();
	return 0;
}

void CConnectionMonitorWindow::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == kUpdateSpeedTimerId) {
		_UpdateSpeedTimer();
		return;
	}

	KillTimer(nIDEvent);

	uint32_t	uniqueId = (uint32_t)nIDEvent;
	int count = m_connectionListView.GetItemCount();
	for (int i = 0; i < count; ++i) {
		if (m_connectionListView.GetItemData(i) == (DWORD_PTR)uniqueId) {
			m_connectionListView.DeleteItem(i);
			break;
		}
	}

	for (auto it = m_connectionCloseList.begin(); it != m_connectionCloseList.end(); ++it) {
		if (uniqueId == *it) {
			m_connectionCloseList.erase(it);
			break;
		}
	}
}

LRESULT CConnectionMonitorWindow::OnConnectionListRClick(LPNMHDR pnmh)
{
	auto lpnmitem = (LPNMITEMACTIVATE)pnmh;
	CString url;
	m_connectionListView.GetItemText(lpnmitem->iItem, 3, url);
	Misc::SetClipboardText(url);
	return 0;
}

LRESULT CConnectionMonitorWindow::OnConnectionListKeyDown(LPNMHDR pnmh)
{
	auto pnkd = (LPNMLVKEYDOWN)pnmh;
	if (pnkd->wVKey == VK_DELETE) {
		std::vector<int> selectedIndexList;
		const int count = m_connectionListView.GetItemCount();
		for (int i = 0; i < count; ++i) {
			if (m_connectionListView.GetItemState(i, LVIS_SELECTED) & LVIS_SELECTED) {
				selectedIndexList.emplace_back(i);
				m_connectionListView.SetItemState(i, 0, LVIS_SELECTED);	// ëIëâèú
			}
		}

		CCritSecLock	lock(CConnectionManager::s_csList);
		for (const int selectedIndex : selectedIndexList) {
			uint32_t uniqueId = static_cast<uint32_t>(m_connectionListView.GetItemData(selectedIndex));
			auto connectionData = _GetConnectionDataFromUniqueId(uniqueId);
			if (!connectionData) {
				continue;
			}
			connectionData->funcKillConnection();
		}
	}
	return LRESULT();
}

DWORD CConnectionMonitorWindow::OnPrePaint(int nID, LPNMCUSTOMDRAW lpnmcd)
{
	if (lpnmcd->hdr.idFrom == IDC_LIST_CONNECTION)
		return CDRF_NOTIFYITEMDRAW;
	else
		return CDRF_DODEFAULT;
}

DWORD CConnectionMonitorWindow::OnItemPrePaint(int nID, LPNMCUSTOMDRAW lpnmcd)
{
	if (lpnmcd->hdr.idFrom == IDC_LIST_CONNECTION) {
		LPNMLVCUSTOMDRAW lpnmlv = (LPNMLVCUSTOMDRAW)lpnmcd;
		uint32_t uniqueId = (uint32_t)lpnmcd->lItemlParam;//m_connectionListView.GetItemData((int)lpnmcd->dwItemSpec);

		if (m_idleConnections.find(uniqueId) != m_idleConnections.end()) {
			lpnmlv->clrText = RGB(0x80, 0x80, 0x80);
		}
		for (uint32_t dlItem : m_connectionCloseList) {
			if (uniqueId == dlItem) {
				lpnmlv->clrText = RGB(255, 255, 255);
				lpnmlv->clrTextBk = RGB(0xDF, 0x33, 0x4E);
				break;
			}
		}
	}
	return CDRF_DODEFAULT;
}

void CConnectionMonitorWindow::_UpdateSpeedTimer()
{
	const auto nowSeconds = GetNowSeconds();
	const int indexSeconds = nowSeconds.count() % 10;
	const auto validFirstSeconds = nowSeconds - std::chrono::seconds(5);

	auto funcCalculateSpeed = [=](const std::array<ConnectionData::TrafficData::Speed, 10>& speedData) -> int {
		int totalBytes = 0;
		long long minSeconds = nowSeconds.count() - 1;
		for (const auto& speed : speedData) {
			if (validFirstSeconds <= speed.recordingTimeSeconds && speed.recordingTimeSeconds < nowSeconds) {
				minSeconds = std::min(minSeconds, speed.recordingTimeSeconds.count());
				totalBytes += speed.bytes;
			}
		}
		int totalSeconds = static_cast<int>((nowSeconds.count() - 1) - minSeconds);
		if (totalSeconds > 0) {
			const int bytesSeconds = totalBytes / totalSeconds;
			return bytesSeconds;
		}
		return 0;
	};

	auto funcTextFromTrafficSpeed = [](int bytesSeconds) -> CString {
		if (bytesSeconds > 0) {
#if 0
			const int Kbps = bytesSeconds * 8 / 1000;	// Kbps
			CString text;
			if (Kbps > 0) {
				text.Format(L"%d Kbps", Kbps);
			} else {
				text.Format(L"%d bps", bytesSeconds * 8);
			}
#endif
#if 1
			const int KBSeconds = bytesSeconds / 1024;	// KB/s
			CString text;
			if (KBSeconds > 0) {
				text.Format(L"%4d KB/s", KBSeconds);
			} else {
				text.Format(L"%4d bytes/s", bytesSeconds);
			}
#endif
			return text;
		}
		return L"";
	};

	auto funcSetItem = [this](int iItem, int iSubItem, LPCWSTR text) {
		LVITEM	Item = {};
		Item.mask = LVIF_TEXT;
		Item.iItem = iItem;
		Item.iSubItem = iSubItem;
		Item.pszText = (LPWSTR)text;
		m_connectionListView.SetItem(&Item);
	};

	int totalUploadBytesSeconds = 0;
	int totalDownloadBytesSeconds = 0;
	const int count = m_connectionListView.GetItemCount();
	for (int i = 0; i < count; ++i) {
		uint32_t uniqueId = static_cast<uint32_t>(m_connectionListView.GetItemData(i));
		auto it = m_connectionTrafficDataList.find(uniqueId);
		if (it == m_connectionTrafficDataList.end()) {
			continue;
		}

		const int uploadBytesSeconds = funcCalculateSpeed(it->second.uploadSpeed);
		const int downloadBytesSeconds = funcCalculateSpeed(it->second.downloadSpeed);

		totalUploadBytesSeconds += uploadBytesSeconds;
		totalDownloadBytesSeconds += downloadBytesSeconds;

		const CString uploadSpeedText = funcTextFromTrafficSpeed(uploadBytesSeconds);
		const CString downloadSpeedText = funcTextFromTrafficSpeed(downloadBytesSeconds);

		enum {
			kUploadSubItem = 4,
			kDownloadSubItem = 5,
		};
		funcSetItem(i, kUploadSubItem, uploadSpeedText);
		funcSetItem(i, kDownloadSubItem, downloadSpeedText);
	}

	//  Upload [ 12 KB/s ]  Download [ 34 bytes/s ]
	m_lastUploadDownloadTrafficText.Format(L" Upload [ %s ] Download [ %s ]", (LPCWSTR)funcTextFromTrafficSpeed(totalUploadBytesSeconds), (LPCWSTR)funcTextFromTrafficSpeed(totalDownloadBytesSeconds));
	_UpdateTitleText();
}

ConnectionData* CConnectionMonitorWindow::_GetConnectionDataFromUniqueId(uint32_t uniqueId)
{
	for (auto& connectionData : CConnectionManager::s_connectionDataList) {
		if (connectionData.uniqueId == uniqueId) {
			return &connectionData;
		}
	}
	return nullptr;
}

void CConnectionMonitorWindow::_UpdateTitleText()
{
	SetWindowText((boost::wformat(L"Connection Monitor - [%02d] %s")
		% m_lastConnectionCount % (LPCWSTR)m_lastUploadDownloadTrafficText).str().c_str());
}
















