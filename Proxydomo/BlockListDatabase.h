#pragma once

#include <string>
#include <list>
#include <ctime>
#include <atlsync.h>
#include "proximodo\url.h"

struct sqlite3;
class CSocket;


class CBlockListDatabase
{

public:
	// CBlockListDatabase::GetInstance();
	static CBlockListDatabase* GetInstance();

	void	Init();

	void	lock();
	void	unlock();

	// lock -> AddList -> AddPatternToList... -> UpdateList -> DeleteOldPatternFromList -> unlock
	void	AddList(const std::string& listName);
	void	UpdateList(const std::string& listName, const std::string& format, int validLineCount);

	void	AddPatternToList(const std::string& listName, const std::wstring& pattern, int line, std::time_t updateTime);
	void	DeleteOldPatternFromList(const std::string& listName, std::time_t updateTime);

	void	DeleteNoExistList();

	void	IncrementHitPatternCount(const std::string& listName, int line);

	bool	ManageBlockListInfoAPI(const CUrl& url, CSocket* sockBrowser);

private:
	CBlockListDatabase();
	~CBlockListDatabase();

	struct PatternLineHitCount
	{
		int	line;
		std::string	pattern;
		int	hitCount;

		PatternLineHitCount(int line, const std::string& pattern, int hitCount)
			: line(line), pattern(pattern), hitCount(hitCount) {}
	};
	enum QueryOption
	{
		kQueryNormal = 1,
		kQueryDLexceptNoHits = 2,
	};
	std::list<PatternLineHitCount>	QueryPatternLineHitCountList(const std::string& listName, QueryOption query);

	struct BlockListInfo
	{
		std::string	fileName;
		std::string	format;
		int			Count;

		BlockListInfo(const std::string& fileName, const std::string& format, int Count) 
			: fileName(fileName), format(format), Count(Count) {}
	};
	std::list<BlockListInfo>	QueryBlockListInfo();

	bool	m_bSaveBlockListUsageSituation;
	sqlite3*	m_db;
	CCriticalSection	m_cs;
};

