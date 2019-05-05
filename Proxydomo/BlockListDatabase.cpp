
#include "stdafx.h"
#include "BlockListDatabase.h"
#include <mutex>
#include <unordered_set>
#include <filesystem>
#include <boost\format.hpp>
#include <boost\algorithm\string.hpp>
#include "sqlite\sqlite3.h"
#pragma comment(lib, "sqlite3.lib")
#include "Logger.h"
#include "Misc.h"
#include "CodeConvert.h"
using namespace CodeConvert;
#include "Settings.h"
#include "Socket.h"
#include "proximodo\util.h"

using namespace boost;
using boost::io::str;
using namespace std::experimental::filesystem;

namespace {

	const char* kDatabaseName = "BlockList.db";

	class CSQLiteStatement
	{
	public:
		CSQLiteStatement(sqlite3* db, const std::string& sql) : m_stmt(nullptr)
		{
			int err = sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.length()), &m_stmt, nullptr);
			if (err != SQLITE_OK) {
				throw std::runtime_error("sqlite3_prepare_v2 failed");
			}
		}

		~CSQLiteStatement()
		{
			sqlite3_finalize(m_stmt);
		}

		// Bindxxx ‚Ì placePos ‚Í 1 ‚©‚çŽw’è‚·‚é
		void	BindText(int placePos, const std::string& text)
		{
			int err = sqlite3_bind_text(m_stmt, placePos, text.c_str(), static_cast<int>(text.length()), SQLITE_TRANSIENT);
			if (err != SQLITE_OK) {
				throw std::runtime_error("sqlite3_bind_text failed");
			}
		}

		void	BindBlob(int placePos, const char* data, int size)
		{
			int err = sqlite3_bind_blob(m_stmt, placePos, (const void*)data, size, SQLITE_TRANSIENT);
			if (err != SQLITE_OK) {
				throw std::runtime_error("sqlite3_bind_text failed");
			}
		}

		void	BindInt(int placePos, int num)
		{
			int err = sqlite3_bind_int(m_stmt, placePos, num);
			if (err != SQLITE_OK) {
				throw std::runtime_error("sqlite3_bind_int failed");
			}
		}

		void	BindInt64(int placePos, int64_t num)
		{
			int err = sqlite3_bind_int64(m_stmt, placePos, num);
			if (err != SQLITE_OK) {
				throw std::runtime_error("sqlite3_bind_int64 failed");
			}
		}

		int		Step()
		{
			int err = sqlite3_step(m_stmt);
			return err;
		}

		// Columnxxx ‚Ì iCol ‚Í 0 ‚©‚çŽw’è‚·‚é
		std::string		ColumnText(int iCol)
		{
			std::string text = (const char*)sqlite3_column_text(m_stmt, iCol);
			return text;
		}

		std::wstring	ColumnText16(int iCol)
		{
			const wchar_t* text = (const wchar_t*)sqlite3_column_text16(m_stmt, iCol);
			return text;
		}

		int		ColumnInt(int iCol)
		{
			int n = sqlite3_column_int(m_stmt, iCol);
			return n;
		}

		int64_t	ColumnInt64(int iCol)
		{
			int64_t n = sqlite3_column_int64(m_stmt, iCol);
			return n;
		}

		std::vector<BYTE>	ColumnBlob(int iCol)
		{
			std::vector<BYTE> vec;
			int bytes = sqlite3_column_bytes(m_stmt, iCol);
			if (bytes > 0) {
				const char* data = (const char*)sqlite3_column_blob(m_stmt, iCol);
				vec.resize(bytes);
				memcpy_s(vec.data(), bytes, data, bytes);
			}
			return vec;
		}

		bool	ColumnIsNull(int iCol)
		{
			bool isNull = sqlite3_column_bytes(m_stmt, iCol) == 0;
			return isNull;
		}

	private:
		sqlite3_stmt* m_stmt;
	};

}	// namespace


///////////////////////////////////////////////////////////////////
// CBlockListDatabase

CBlockListDatabase* CBlockListDatabase::GetInstance()
{
	static CBlockListDatabase s_instance;
	if (s_instance.m_bSaveBlockListUsageSituation) {
		return &s_instance;

	} else {
		return nullptr;
	}
}

CBlockListDatabase::CBlockListDatabase() : m_bSaveBlockListUsageSituation(true), m_db(nullptr)
{

}

CBlockListDatabase::~CBlockListDatabase()
{
	if (m_db) {
		int err = sqlite3_close(m_db);
		if (err != SQLITE_OK) {
			ERROR_LOG << L" sqlite3_close failed";
		}
	}
}

void CBlockListDatabase::Init()
{
	m_bSaveBlockListUsageSituation = CSettings::s_saveBlockListUsageSituation;
	if (m_bSaveBlockListUsageSituation == false) {
		return;
	}

	CString dbPath = Misc::GetExeDirectory() + kDatabaseName;
	int err = sqlite3_open16((LPCWSTR)dbPath, &m_db);
	if (err != SQLITE_OK) {
		ERROR_LOG << L"sqlite3_open failed";
		throw std::runtime_error("sqlite3_open failed");
	}

	char* errmsg = nullptr;
	err = sqlite3_exec(m_db, "DROP TABLE ':tableinfo:'", nullptr, nullptr, &errmsg);
	err = sqlite3_exec(m_db,
		R"(CREATE TABLE ':tableinfo:'(
						listName TEXT PRIMARY KEY, 
						format TEXT, 
						validLineCount INTEGER
						);)",
		nullptr, nullptr, &errmsg);
	if (err != SQLITE_OK) {
		ERROR_LOG << L"sqlite3_exec failed : " << errmsg;
		sqlite3_free(errmsg);
		throw std::runtime_error("sqlite3_exec failed");
	}
}

void	CBlockListDatabase::lock()
{
	m_cs.Enter(); 
	char* errmsg = nullptr;
	int err = sqlite3_exec(m_db, "BEGIN;", nullptr, nullptr, &errmsg);
	if (err != SQLITE_OK) {
		ERROR_LOG << L"sqlite3_exec BEGIN failed : " << errmsg;
		sqlite3_free(errmsg);
	}
}

void	CBlockListDatabase::unlock()
{
	char* errmsg = nullptr;
	int err = sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, &errmsg);
	if (err != SQLITE_OK) {
		ERROR_LOG << L"sqlite3_exec COMMIT failed : " << errmsg;
		sqlite3_free(errmsg);
	}

	m_cs.Leave();
}

void	CBlockListDatabase::AddList(const std::string& listName)
{
	CSQLiteStatement stmt(m_db, "INSERT INTO ':tableinfo:'(listName) VALUES(?);");
	stmt.BindText(1, listName);
	int err = stmt.Step();
	if (err != SQLITE_DONE) {
		return;
	}

	{
		CSQLiteStatement stmt(m_db, "SELECT count(*) FROM sqlite_master WHERE type = 'table' AND name = ?;");
		stmt.BindText(1, listName);
		int err = stmt.Step();
		if (err == SQLITE_ROW && stmt.ColumnInt(0) == 1) {
			return;	// table‚ÍŠù‚É‘¶Ý‚·‚é
		}
	}

	char* errmsg = nullptr;
	std::string sql = boost::io::str(boost::format("CREATE TABLE '%1%'(pattern TEXT PRIMARY KEY, hitCount INTEGER DEFAULT 0, line INTEGER, updateTime INTEGER);") % listName);
	err = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errmsg);
	if (err != SQLITE_OK) {
		ATLASSERT(FALSE);
		ERROR_LOG << L"CBlockListDatabase::AddList CREATE TABLE failed : " << errmsg << L" sql : " << sql;
		sqlite3_free(errmsg);
		return;
	}

	CSQLiteStatement stmt2(m_db, str(format("CREATE INDEX '%1%' ON '%2%' (pattern);") % (listName + "_index") % listName));
	err = stmt2.Step();
	if (err != SQLITE_DONE) {
		ATLASSERT(FALSE);
		ERROR_LOG << L"CBlockListDatabase::AddList CREATE INDEX failed";
	}
}

void	CBlockListDatabase::UpdateList(const std::string& listName, const std::string& format, int validLineCount)
{
	CSQLiteStatement stmt(m_db, "UPDATE ':tableinfo:' SET format = ?, validLineCount = ? WHERE listName = ?;");
	stmt.BindText(1, format);
	stmt.BindInt(2, validLineCount);
	stmt.BindText(3, listName);
	int err = stmt.Step();
	if (err != SQLITE_DONE) {
		ERROR_LOG << L"CBlockListDatabase::UpdateList failed";
	}
}

void CBlockListDatabase::AddPatternToList(const std::string & listName, const std::wstring & pattern, int line, std::time_t updateTime)
{
	std::string u8pattern = UTF8fromUTF16(pattern);
	CSQLiteStatement stmt(m_db, str(format("SELECT count(*) FROM '%1%' WHERE pattern = ?;") % listName));
	stmt.BindText(1, u8pattern);
	int err = stmt.Step();
	int count = stmt.ColumnInt(0);
	if (count == 0) {	// ‘¶Ý‚µ‚È‚¢A’Ç‰Á‚·‚é
		CSQLiteStatement stmt2(m_db, str(format("INSERT INTO '%1%' (pattern, line, updateTime) VALUES(?, ?, ?);") % listName));
		stmt2.BindText(1, u8pattern);
		stmt2.BindInt(2, line);
		stmt2.BindInt64(3, updateTime);
		err = stmt2.Step();
		if (err != SQLITE_DONE) {
			ERROR_LOG << L"CBlockListDatabase::AddPatternToList INSERT INTO failed";
		}
	} else {
		CSQLiteStatement stmt2(m_db, str(format("UPDATE '%1%' SET line = ?, updateTime = ? WHERE pattern = ?;") % listName));
		stmt2.BindInt(1, line);
		stmt2.BindInt64(2, updateTime);
		stmt2.BindText(3, u8pattern);
		err = stmt2.Step();
		if (err != SQLITE_DONE) {
			ERROR_LOG << L"CBlockListDatabase::AddPatternToList UPDATE failed";
		}
	}
}

void	CBlockListDatabase::DeleteOldPatternFromList(const std::string& listName, std::time_t updateTime)
{
	CSQLiteStatement stmt(m_db, str(format("DELETE FROM '%1%' WHERE updateTime != ?;") % listName));
	stmt.BindInt64(1, updateTime);
	int err = stmt.Step();
	if (err != SQLITE_DONE) {
		ERROR_LOG << L"CBlockListDatabase::DeleteOldPatternFromList failed";
	}
}

void CBlockListDatabase::DeleteNoExistList()
{
	std::unordered_set<std::string>	existList;
	{
		CSQLiteStatement stmt(m_db, "SELECT listName FROM ':tableinfo:';");
		int err;
		while ((err = stmt.Step()) == SQLITE_ROW) {
			std::string listName = stmt.ColumnText(0);
			existList.emplace(listName);
		}
		ATLASSERT(err == SQLITE_DONE);
	}
	std::list<std::string>	eraseList;
	{
		CSQLiteStatement stmt(m_db, "SELECT * FROM sqlite_master WHERE type = 'table' AND name != ':tableinfo:';");
		int err;
		while ((err = stmt.Step()) == SQLITE_ROW) {
			std::string tableName = stmt.ColumnText(1);
			if (existList.find(tableName) == existList.end()) {
				eraseList.emplace_back(tableName);
			}
		}
		ATLASSERT(err == SQLITE_DONE);
	}
	for (const std::string& tableName : eraseList) {
		char* errmsg = nullptr;
		int err = sqlite3_exec(m_db, str(format("DROP TABLE '%1%';") % tableName).c_str(), nullptr, nullptr, &errmsg);
		ATLASSERT(err == SQLITE_OK);
		if (errmsg) {
			sqlite3_free(errmsg);
		}
	}
}

void CBlockListDatabase::IncrementHitPatternCount(const std::string & listName, int line)
{
	std::lock_guard<CBlockListDatabase> lock(*this);

	CSQLiteStatement stmt(m_db, str(format("UPDATE '%1%' SET hitCount = hitCount + 1 WHERE line = ?;") % listName));
	stmt.BindInt(1, line);
	int err = stmt.Step();
	ATLASSERT(err == SQLITE_DONE);
	if (err != SQLITE_DONE) {
		ERROR_LOG << L"CBlockListDatabase::IncrementHitPatternCount failed";
	}
}

bool CBlockListDatabase::ManageBlockListInfoAPI(const CUrl& url, SocketIF* sockBrowser)
{
	if (url.getPath() == L"/blocklistinfo/" || url.getPath() == L"/blocklistinfo/index.html") {
		std::string contentType = "text/html";
		std::string content = CUtil::getFile(L".\\html\\blocklistinfo\\index.html");
		if (content.empty())
			return false;

		auto blockListInfoList = QueryBlockListInfo();
		std::string tablebody;
		for (const auto& blockListInfo : blockListInfoList) {
			tablebody += str(format(R"(<tr> 
										<td><a href="./%1%.txt">%1%.txt</a></td> 
										<td>%2%</td>
										<td>%3%</td>
									   </tr>)") % blockListInfo.fileName % blockListInfo.format % blockListInfo.Count);
		}

		content = CUtil::replaceAll(content, "%%BlockListTableBody%%", tablebody);


#define CRLF "\r\n"

		std::string sendInBuf =
			"HTTP/1.1 200 OK" CRLF
			"Content-Type: " + contentType + CRLF
			"Content-Length: " + boost::lexical_cast<std::string>(content.size()) + CRLF
			"Connection: close" + CRLF CRLF;
		sendInBuf += content;

		while (sockBrowser->Write(sendInBuf.data(), sendInBuf.length()) > 0);
		sockBrowser->Close();

		return true;

	} else if (boost::starts_with(url.getPath(), "/blocklistinfo/")) {
		path urlPath = url.getPath();
		if (urlPath.extension() == L".txt") {
			std::string listName =  urlPath.stem().u8string();
			std::string contentType;
			std::string content;
			if (url.getQuery() == L"?dl") {
				contentType = R"(text/plain; charset="UTF-8")";
				auto patternLineHitCountList = QueryPatternLineHitCountList(listName, kQueryDLexceptNoHits);
				for (const auto& patternLineHitCount : patternLineHitCountList) {
					content += patternLineHitCount.pattern + CRLF;
				}

			} else {
				contentType = "text/html";
				bool bHide0hits = url.getQuery() == L"?hide0hits";
				auto patternLineHitCountList = QueryPatternLineHitCountList(listName, bHide0hits ? kQueryDLexceptNoHits : kQueryNormal);

				content = CUtil::getFile(".\\html\\blocklistinfo\\list_template.html");
				if (content.empty()) {
					ATLASSERT(FALSE);
					return false;
				}

				content = CUtil::replaceAll(content, "%%title%%", urlPath.filename().u8string());

				std::string tablebody;
				for (const auto& patternLineHitCount : patternLineHitCountList) {
					tablebody += str(format(R"(<tr> <td>%1%</td> <td class="pattern">%2%</td> <td>%3%</td> </tr>)")
						% patternLineHitCount.line % patternLineHitCount.pattern % patternLineHitCount.hitCount);
				}
				content = CUtil::replaceAll(content, "%%BlockListTableBody%%", tablebody);
			}

			std::string sendInBuf =
				"HTTP/1.1 200 OK" CRLF
				"Content-Type: " + contentType + CRLF
				"Content-Length: " + boost::lexical_cast<std::string>(content.size()) + CRLF
				"Connection: close" + CRLF CRLF;
			sendInBuf += content;

			while (sockBrowser->Write(sendInBuf.data(), sendInBuf.length()) > 0);
			sockBrowser->Close();
			return true;
		}
	}
	return false;
}

std::list<CBlockListDatabase::PatternLineHitCount> CBlockListDatabase::QueryPatternLineHitCountList(const std::string & listName, QueryOption query)
{
	std::list<PatternLineHitCount> patternlineHitCountList;

	std::lock_guard<CBlockListDatabase> lock(*this);

	std::string sql;
	switch (query)
	{
	case kQueryNormal:
		sql = str(format("SELECT line, pattern, hitCount FROM '%1%' ORDER BY line ASC;") % listName);
		break;

	case kQueryDLexceptNoHits:
		sql = str(format("SELECT line, pattern, hitCount FROM '%1%' WHERE hitCount > 0 ORDER BY line ASC;") % listName);
		break;

	default:
		ATLASSERT(FALSE);
		throw std::runtime_error("no QueryOption");
	}

	CSQLiteStatement stmt(m_db, sql);
	int err;
	while ((err = stmt.Step()) == SQLITE_ROW) {
		int line  = stmt.ColumnInt(0);
		std::string pattern = stmt.ColumnText(1);
		int hitCount = stmt.ColumnInt(2);
		patternlineHitCountList.emplace_back(line, pattern, hitCount);
	}
	ATLASSERT(err == SQLITE_DONE);

	return patternlineHitCountList;
}

std::list<CBlockListDatabase::BlockListInfo> CBlockListDatabase::QueryBlockListInfo()
{
	std::list<BlockListInfo> blockListInfoList;

	std::lock_guard<CBlockListDatabase> lock(*this);

	CSQLiteStatement stmt(m_db, "SELECT listName, format, validLineCount FROM ':tableinfo:' ORDER BY listName ASC;");
	int err;
	while ((err = stmt.Step()) == SQLITE_ROW) {
		std::string listName = stmt.ColumnText(0);
		std::string format = stmt.ColumnText(1);
		int Count = stmt.ColumnInt(2);
		blockListInfoList.emplace_back(listName, format, Count);
	}
	ATLASSERT(err == SQLITE_DONE);

	return blockListInfoList;
}


#ifdef UNIT_TEST
#include <gtest\gtest.h>

///////////////////////////////////////////////////////////////////////////////////
// BlockListDatabase Test

TEST(BlockListDatabase, Test)
{
	CString dbPath = Misc::GetExeDirectory() + kDatabaseName;
	//::DeleteFile(dbPath);

	CSettings::s_saveBlockListUsageSituation = true;
	auto& db = *CBlockListDatabase::GetInstance();
	db.Init();

	std::string listName = "test2";
	{
		std::lock_guard<CBlockListDatabase> lock(db);

		db.AddList(listName);

		std::time_t updateTime = std::time(nullptr);
		db.AddPatternToList(listName, L"test_pattern", 1, updateTime);

		db.UpdateList(listName, "normal", 1);
		db.DeleteOldPatternFromList(listName, updateTime);
	}
	db.DeleteNoExistList();

#if 0
	auto list = db.QueryPatternLineHitCountList(listName);
	EXPECT_TRUE(list.front().line == 1);
	EXPECT_TRUE(list.front().pattern == "test_pattern");
	EXPECT_TRUE(list.front().hitCount == 0);

	db.IncrementHitPatternCount(listName, 1);
	list = db.QueryPatternLineHitCountList(listName);
	EXPECT_TRUE(list.front().hitCount == 1);

	Sleep(1000);
	std::time_t updateTime = std::time(nullptr);
	db.AddPatternToList(listName, L"test_pattern2", 2, updateTime);
	db.DeleteOldPatternFromList(listName, updateTime);
	list = db.QueryPatternLineHitCountList(listName);
	EXPECT_TRUE(list.size() == 1);
	EXPECT_TRUE(list.front().line == 2);
	EXPECT_TRUE(list.front().pattern == "test_pattern2");
	EXPECT_TRUE(list.front().hitCount == 0);
#endif

}

#endif	// UNIT_TEST

























