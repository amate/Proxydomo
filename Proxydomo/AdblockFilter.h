#pragma once

#include <istream>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include "Node.h"


struct IOptionType
{
	virtual ~IOptionType() {};
	virtual bool match(Proxydomo::MatchData* pMatch) = 0;
};


class CAdblockFilter
{
public:
	CAdblockFilter(const std::string& name = "", bool top = true);
	~CAdblockFilter();

	const UChar* match(const UChar* start, const UChar* stop, Proxydomo::MatchData* pMatch, Proxydomo::CNode* nextNode);

	bool	AddPattern(const std::wstring& text, int listLine = -1);

private:
	std::string	name;

	std::vector<std::unique_ptr<IOptionType>> _ParseOption(const std::wstring& option);

	struct NodeOption
	{
		std::wstring tail;
		std::vector<std::unique_ptr<IOptionType>>	vecOptionType;
		int	listLine;

		NodeOption(const std::wstring& tail, 
					std::vector<std::unique_ptr<IOptionType>>&& vecOptionType, int listLine)
			: tail(tail), vecOptionType(std::move(vecOptionType)), listLine(listLine) {}

		const UChar*	TailCmp(const UChar* start, const UChar* stop);
		bool			OptionMatch(Proxydomo::MatchData* pMatch);
	};

	// 固定プレフィックスリスト
	struct PreHashWord {
		std::unordered_map<wchar_t, std::unique_ptr<PreHashWord>>	mapChildPreHashWord;
		std::vector<NodeOption>	vecNode;
	};
	std::unordered_map<wchar_t, std::unique_ptr<PreHashWord>>	PreHashWordList;

	// URLハッシュリスト
	struct URLHash {
		std::unordered_map<std::wstring, std::unique_ptr<URLHash>>	mapChildURLHash;
		std::vector<NodeOption>	vecNode;
	};
	std::unordered_map<std::wstring, std::unique_ptr<URLHash>>	URLHashList;

	// start_with
	std::deque<std::pair<std::wstring, NodeOption>>	StartWithList;

	// $ only
	std::deque<NodeOption>	NodeOptionList;

	// * 入り
	std::deque<std::pair<std::wstring, NodeOption>>	URLHashSpecialCharacterList;
	
	std::unique_ptr<CAdblockFilter>	whilteListFilter;
};

std::unique_ptr<CAdblockFilter>	LoadAdblockFilter(std::wistream& fs, const std::string& filename);

