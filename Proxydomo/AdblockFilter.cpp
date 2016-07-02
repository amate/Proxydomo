#include "stdafx.h"
#include "AdblockFilter.h"
#include <boost/algorithm/string.hpp> 
#include <boost/utility/string_ref.hpp>
#include <boost/optional.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/algorithm/string/join.hpp>
#include "Nodes.h"
#include "FilterOwner.h"
#include "Log.h"
#include "Logger.h"
#include "DomainJudge.h"

using namespace Proxydomo;

namespace {

enum { kMaxDomainLength = 255 };

bool IsDomainMatch(const std::wstring& urlHost, const std::wstring& domain)
{
	if (boost::iends_with(urlHost, domain)) {
		if (urlHost.length() == domain.length()) {
			return true;
		} else {
			size_t dotPos = urlHost.length() - domain.length() - 1;
			if (urlHost[dotPos] == L'.') {
				return true;
			}
		}
	}
	return false;
};


}	// namespace


std::deque<std::wstring>	SpliteDomain(const std::wstring& urlHost)
{
	std::deque<std::wstring> deqDomain;
	std::wstring domain;
	for (auto it = urlHost.cbegin(); it != urlHost.cend(); ++it) {
		bool bEnd = std::next(it) == urlHost.cend();
		if (*it == L'.' || bEnd) {
			if (bEnd) {
				domain.push_back(tolower(*it));
			}
			if (domain.empty() || domain.length() >= kMaxDomainLength || (*it == L'.' && bEnd)) {
				deqDomain.clear();
				return deqDomain;	// 
			}

			deqDomain.push_back(domain);
			domain.clear();

		} else {
			domain.push_back(tolower(*it));
		}
	}
	//ATLASSERT(deqDomain.size());
	return deqDomain;
}

const UChar* StrMatchWildcard(const UChar* pattern, const UChar* start, const UChar* stop)
{
	switch (*pattern) {
	case L'\0':
	{
		return start;
	}
	break;

	case L'^':
	{
		const UChar c = *start;
		auto funcIsSepChar = [c]() -> bool {
			if (L'a' <= c && c <= L'z') {
				return false;
			} else if (L'0' <= c && c <= L'9') {
				return false;
			} else if (c == L'_' || c == L'-' || c == L'%') {
				return false;
			} else {
				return true;
			}
		};
		if (funcIsSepChar() == false) {
			return nullptr;
		}
		return StrMatchWildcard(pattern + 1, start + 1, stop);
	}
	break;

	case L'*':
	{
		if (*(pattern + 1) == L'\0') {
			return stop;
		} else {
			const UChar* ret = StrMatchWildcard(pattern + 1, start, stop);
			if (ret) {
				return ret;
			} else {
				if (start == stop) {
					return nullptr;
				} else {
					ret = StrMatchWildcard(pattern, start + 1, stop);
					return ret;
				}
			}
		}
	}
	break;

	case L'|':
	{
		ATLASSERT(*(pattern + 1) == L'\0');
		if (start == stop) {
			return start;
		} else {
			return nullptr;
		}
	}
	break;

	default:
	{
		if (*pattern == tolower(*start)) {
			return StrMatchWildcard(pattern + 1, start + 1, stop);
		} else {
			return nullptr;
		}
	}
	break;
	}
}


CAdblockFilter::CAdblockFilter(const std::string& name, bool top /*= true*/) : name(name)
{
	if (top) {
		whilteListFilter.reset(new CAdblockFilter(name, false));
	}
}


CAdblockFilter::~CAdblockFilter()
{
}


const UChar*	CAdblockFilter::NodeOption::TailCmp(const UChar* start, const UChar* stop)
{
#if 0
	const UChar* ptr = tail.c_str();
	const UChar* max = (stop < start + tail.length()) ? stop : start + tail.length();

	while (start < max && *ptr == towlower(*start)) { ptr++; start++; }

	// 全部消費されなかった
	if (ptr < tail.c_str() + tail.length()) {
		return nullptr;
	}
	return start;
#endif
	const UChar* ret = StrMatchWildcard(tail.c_str(), start, stop);
	return ret;
}

bool			CAdblockFilter::NodeOption::OptionMatch(Proxydomo::MatchData* pMatch)
{
	for (auto& optionType : vecOptionType) {
		if (optionType->match(pMatch) == false)
			return false;
	}
	return true;
}

const UChar* CAdblockFilter::match(const UChar* start, const UChar* stop, Proxydomo::MatchData* pMatch, Proxydomo::CNode* nextNode)
{
	const UChar* startOrigin = start;

	CNode_Star nodeStar;
	nodeStar.setNextNode(nextNode);

	// URLハッシュ
	const UChar* slashPos = stop;
	const UChar* urlStart = start;
	boost::wstring_ref content(start, stop - start);
	if (boost::istarts_with(content, L"http://")) {
		urlStart += 7;
		start = urlStart;
	} else if (boost::istarts_with(content, L"https://")) {
		urlStart += 8;
		start = urlStart;
	}
	for (int i = 0; start < stop && i < kMaxDomainLength; ++i, ++start) {
		if (*start == L'/' || *start == L':') {
			slashPos = start;
			break;
		}
	}
	std::wstring urlHost(urlStart, slashPos);
	std::deque<std::wstring> deqDomain = SpliteDomain(urlHost);
	
	auto	pmapChildURLHash = &URLHashList;
	for (auto it = deqDomain.rbegin(); it != deqDomain.rend(); ++it) {
		auto itfound = pmapChildURLHash->find(*it);
		if (itfound == pmapChildURLHash->end())
			break;

		for (auto& node : itfound->second->vecNode) {
			const UChar* ptr = node.TailCmp(slashPos, stop);
			if (ptr && node.OptionMatch(pMatch)) {
				const UChar* ret = nodeStar.match(ptr, stop, pMatch);
				if (ret) {
					if (whilteListFilter && whilteListFilter->match(startOrigin, stop, pMatch, nextNode)) {
						return nullptr;
					}
					pMatch->matchListLog.emplace_back(name, node.listLine);
					return ret;
				}
			}
		}
		pmapChildURLHash = &itfound->second->mapChildURLHash;
	}

	auto funcListMatcher = 
		[&](const UChar* begin, 
			std::deque<std::pair<std::wstring, NodeOption>>& matchList) 
													-> boost::optional<const UChar*>
	{
		for (auto& node : matchList) {
			const UChar* ptr = StrMatchWildcard(node.first.c_str(), begin, stop);
			if (ptr && node.second.OptionMatch(pMatch)) {
				const UChar* ret = nodeStar.match(ptr, stop, pMatch);
				if (ret) {
					if (whilteListFilter && whilteListFilter->match(startOrigin, stop, pMatch, nextNode)) {
						return nullptr;
					}
					pMatch->matchListLog.emplace_back(name, node.second.listLine);
					return ret;
				}
			}
		}
		return boost::none;
	};

	// *
	bool bAllowSearch = true;
	// . 区切りごとに検索していく、 / が出たら終わり
	for (const UChar* hostBegin = urlStart; hostBegin != stop; ++hostBegin) {
		if (bAllowSearch) {
			if (auto value = funcListMatcher(hostBegin, URLHashSpecialCharacterList)) {
				return *value;
			}
		}
		if (*hostBegin == L'.') {
			bAllowSearch = true;

		} else if (*hostBegin == L'/') {
			break;

		} else {
			bAllowSearch = false;
		}
	}

	start = startOrigin;
	
	// start_with
	if (auto value = funcListMatcher(start, StartWithList)) {
		return *value;
	}

	enum { kMaxPreWildcardRange = 256 };
	for (int i = 0; (start < stop && i < kMaxPreWildcardRange); ++start, ++i) {
		// 固定プレフィックス
		auto pmapPreHashWord = &PreHashWordList;
		const UChar* first = start;
		while (first < stop) {
			auto itfound = pmapPreHashWord->find(towlower(*first));
			if (itfound == pmapPreHashWord->end())
				break;

			++first;
			for (auto& node : itfound->second->vecNode) {
				const UChar* ptr = node.TailCmp(first, stop);
				if (ptr && node.OptionMatch(pMatch)) {
					const UChar* ret = nodeStar.match(ptr, stop, pMatch);
					if (ret) {
						if (whilteListFilter && whilteListFilter->match(startOrigin, stop, pMatch, nextNode)) {
							return nullptr;
						}
						pMatch->matchListLog.emplace_back(name, node.listLine);
						return ret;
					}
				}
			}
			pmapPreHashWord = &itfound->second->mapChildPreHashWord;
		}
	}

	// $ only
	for (auto& node : NodeOptionList) {
		if (node.OptionMatch(pMatch)) {
			if (whilteListFilter && whilteListFilter->match(startOrigin, stop, pMatch, nextNode)) {
				return nullptr;
			}
			return startOrigin;
		}
	}
	//UpdateReached(start, pMatch);

	return nullptr;
}

std::wstring CAdblockFilter::ElementHidingCssSelector(const std::wstring& urlHost)
{
	boost::container::flat_set<std::wstring> cssSelectors;
	for (const auto& selector : ElementHidingAllDomainList) {
		cssSelectors.emplace(selector);
	}

	std::vector<std::wstring>	exceptionCssSelectors;

	const wchar_t* urlBegin = urlHost.data();
	const wchar_t* urlEnd = urlBegin + urlHost.length();
	bool bSep = true;
	for (; urlBegin != urlEnd; ++urlBegin) {
		if (bSep) {
			std::wstring domain = urlBegin;
			auto range = ElementHidingList.equal_range(domain);
			for (auto it = range.first; it != range.second; ++it) {
				if (it->second->IsMatchWhiteDomain(urlHost) == false) {
					cssSelectors.emplace(it->second->cssSelector);
				}
			}

			auto exceptRange = ElementHidingExceptionList.equal_range(domain);
			for (auto it = exceptRange.first; it != exceptRange.second; ++it) {
				exceptionCssSelectors.emplace_back(it->second);
			}
		}

		if (*urlBegin == L'.') {
			bSep = true;
		} else {
			bSep = false;
		}
	}

	for (const auto& whiteDomainList : ElementHidingWhiteList) {
		if (whiteDomainList->IsMatchWhiteDomain(urlHost) == false) {
			cssSelectors.emplace(whiteDomainList->cssSelector);
		}
	}

	auto funcEraseCssSelector = [&cssSelectors](const std::vector<std::wstring>& eraseRuleList) {
		for (const auto& rule : eraseRuleList) {
			cssSelectors.erase(rule);
		}
	};
	funcEraseCssSelector(ElementHidingAllDomainExceptionList);
	funcEraseCssSelector(exceptionCssSelectors);

	std::wstring strJoinSelectors = boost::join(cssSelectors, L",");
	return strJoinSelectors;
}

void	EmplaceBackDomainCssSelector(const std::wstring& sep, const std::wstring& line, std::unordered_multimap<std::wstring, std::wstring>& list)
{
	size_t sepPos = line.find(sep);
	ATLASSERT(sepPos != std::wstring::npos);
	std::wstring cssSelector = line.substr(sepPos + sep.length());
	std::wstring domains = line.substr(0, sepPos);
	std::vector<std::wstring> vecDomains;
	boost::split(vecDomains, domains, boost::is_any_of(L","));
	ATLASSERT(vecDomains.size());
	for (const auto& domain : vecDomains) {
		list.emplace(domain, cssSelector);
	}
};

/////////////////////////////////////////////////////////////////////////////////
// WhiteDomainList

bool CAdblockFilter::WhiteDomainList::ParseLine(const std::wstring& strLine)
{
	size_t sepPos = strLine.find(L"##");
	ATLASSERT(sepPos != std::wstring::npos);
	std::wstring cssSelector = strLine.substr(sepPos + 2);
	std::wstring domains = strLine.substr(0, sepPos);
	std::vector<std::wstring> vecDomains;
	boost::split(vecDomains, domains, boost::is_any_of(L","));
	for (const auto& domain : vecDomains) {
		ATLASSERT(domain.length() > 0 && domain.front() == L'~');
		if (domain.empty())
			continue;

		if (domain.front() != L'~') {
			ATLASSERT(FALSE);
			WARN_LOG << L"WhiteDomainList : ~ not contain domain line : " << strLine;
			continue;
		}

		vecWhiteDomains.emplace_back(domain.substr(1));
	}
	if (vecWhiteDomains.empty()) {
		ATLASSERT(FALSE);
		return false;
	}
	this->cssSelector = cssSelector;
	return true;
}

bool CAdblockFilter::WhiteDomainList::IsMatchWhiteDomain(const std::wstring& urlHost) const
{
	for (const auto& domain : vecWhiteDomains) {
		if (IsDomainMatch(urlHost, domain)) {
			return true;
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// DomainCssSelector

std::vector<std::wstring> CAdblockFilter::DomainCssSelector::ParseLine(const std::wstring& strLine)
{
	std::vector<std::wstring>	targetDomains;

	size_t sepPos = strLine.find(L"##");
	ATLASSERT(sepPos != std::wstring::npos);
	std::wstring cssSelector = strLine.substr(sepPos + 2);
	std::wstring domains = strLine.substr(0, sepPos);
	std::vector<std::wstring> vecDomains;
	boost::split(vecDomains, domains, boost::is_any_of(L","));
	for (const auto& domain : vecDomains) {
		ATLASSERT(domain.length());
		if (domain.empty())
			continue;

		if (domain.front() == L'~') {
			vecWhiteDomains.emplace_back(domain.substr(1));
		} else {
			targetDomains.emplace_back(domain);
		}
	}
	ATLASSERT(targetDomains.size());
	this->cssSelector = cssSelector;
	return targetDomains;
}

bool CAdblockFilter::DomainCssSelector::IsMatchWhiteDomain(const std::wstring& urlHost) const
{
	for (const auto& domain : vecWhiteDomains) {
		if (IsDomainMatch(urlHost, domain)) {
			return true;
		}
	}
	return false;
}



bool	CAdblockFilter::AddPattern(const std::wstring& text, int listLine)
{
	if (boost::starts_with(text, L"@@")) {
		ATLASSERT(whilteListFilter);
		if (whilteListFilter == nullptr) {
			return false;
		}
		return whilteListFilter->AddPattern(text.substr(2), listLine);
	}

	if (boost::contains(text, L"#@#")) {
		if (boost::starts_with(text, L"#@#")) {
			ElementHidingAllDomainExceptionList.emplace_back(text.substr(3));
			return true;

		} else {
			EmplaceBackDomainCssSelector(L"#@#", text, ElementHidingExceptionList);
			return true;
		}
	} else if (boost::contains(text, L"##")) {
		if (boost::starts_with(text, L"##")) {
			ElementHidingAllDomainList.emplace_back(text.substr(2));
			return true;

		} else {
			if (text.front() == L'~') {
				auto whiteDomainList = std::make_unique<WhiteDomainList>();
				if (whiteDomainList->ParseLine(text)) {
					ElementHidingWhiteList.emplace_back(std::move(whiteDomainList));
					return true;

				} else {
					return false;
				}
			} else {
				auto domainCssSelector = std::make_shared<DomainCssSelector>();
				auto targetDomains = domainCssSelector->ParseLine(text);
				for (const auto& domain : targetDomains) {
					ElementHidingList.emplace(domain, domainCssSelector);
				}
				if (targetDomains.empty()) {
					return false;

				} else {
					return true;
				}
			}
			return true;
		}
	}

	std::wstring pattern;
	std::wstring option;
	std::vector<std::unique_ptr<IOptionType>> vecOptionType;
	auto optPos = text.find(L'$');
	if (optPos != std::wstring::npos) {
		pattern = text.substr(0, optPos);
		option = boost::to_lower_copy(text.substr(optPos + 1));
		vecOptionType = _ParseOption(option);
		if (pattern.empty()) {
			ATLASSERT(vecOptionType.size());
			if (vecOptionType.empty()) {
				return false;
			}
			NodeOptionList.emplace_back(L"", std::move(vecOptionType), listLine);
			return true;
		}
	} else {
		pattern = text;
	}
	ATLASSERT(pattern.length());
	if (pattern.length() > 0 && pattern.back() == L'*') {
		pattern.erase(pattern.begin() + pattern.length() - 1);
	}
	if (pattern.length() > 0 && pattern.front() == L'*') {
		pattern.erase(pattern.begin());
	}

	UnicodeString pat(pattern.c_str(), (int32_t)pattern.length());
	StringCharacterIterator patternIt(pat);
	const UChar firstChar = patternIt.current();
	const UChar secondChar = patternIt.next();
	patternIt.previous();

	if (firstChar == L'|') {
		// ||
		if (secondChar == L'|') {
			// URLハッシュ
			auto slashPos = pattern.find_first_of(L"^/", 2);
			if (slashPos == std::wstring::npos) {
				slashPos = pattern.length();
			}
			std::wstring urlHost = pattern.substr(2, slashPos - 2);
			ATLASSERT(urlHost.length());

			std::deque<std::wstring> deqDomain = SpliteDomain(urlHost);

			if (boost::contains(urlHost, L"*") || deqDomain.empty()) {
				URLHashSpecialCharacterList.emplace_back(std::piecewise_construct,
					std::forward_as_tuple(pattern.substr(2)),
					std::forward_as_tuple(L"", std::move(vecOptionType), listLine));
				return true;
			}

			std::wstring lastWildcard = pattern.substr(slashPos);
			auto	pmapChildURLHash = &URLHashList;
			for (auto it = deqDomain.rbegin(); it != deqDomain.rend(); ++it) {
				auto& pURLHash = (*pmapChildURLHash)[*it];
				if (pURLHash == nullptr)
					pURLHash.reset(new URLHash);

				if (std::next(it) == deqDomain.rend()) {
					pURLHash->vecNode.emplace_back(lastWildcard, std::move(vecOptionType), listLine);
					return true;
				}
				pmapChildURLHash = &pURLHash->mapChildURLHash;
			}
			ATLASSERT(FALSE);

		} else {	// |
			pattern.erase(pattern.begin());
			StartWithList.emplace_back(std::piecewise_construct, 
				std::forward_as_tuple(pattern), 
				std::forward_as_tuple(L"", std::move(vecOptionType), listLine) );
			return true;
		}
	}

	// 固定プレフィックス
	enum { kMaxPreHashWordLength = 7 };
	auto pmapPreHashWord = &PreHashWordList;
	for (; patternIt.hasNext() && patternIt.getIndex() < kMaxPreHashWordLength; ) {
		const UChar currentChar = patternIt.current();
		const UChar nextChar = patternIt.next();
		ATLASSERT(currentChar != L'*');
		auto& pmapChildHashWord = (*pmapPreHashWord)[towlower(currentChar)];
		if (pmapChildHashWord == nullptr) {
			pmapChildHashWord.reset(new PreHashWord);
		}
		if (	nextChar == L'*' || nextChar == L'|'
			|| (patternIt.getIndex() == patternIt.getLength())
			|| (patternIt.getIndex() == kMaxPreHashWordLength) ) 
		{
			std::wstring tail;
			for (; patternIt.hasNext(); patternIt.next()) {
				tail += towlower(patternIt.current());
			}

			pmapChildHashWord->vecNode.emplace_back(tail, std::move(vecOptionType), listLine);
			
			return true;
		}
		pmapPreHashWord = &pmapChildHashWord->mapChildPreHashWord;
	}

	return true;
}

struct ScriptOption : public IOptionType
{
	virtual bool match(Proxydomo::MatchData* pMatch) override
	{
		return pMatch->pFilter->owner.fileType == "js";
	}
};

struct ImageOption : public IOptionType
{
	virtual bool match(Proxydomo::MatchData* pMatch) override
	{
		std::wstring contentType = pMatch->pFilter->owner.GetInHeader(L"Content-Type");
		bool b = boost::istarts_with(contentType, L"image");
		return b;
	}
};

struct CssOption : public IOptionType
{
	virtual bool match(Proxydomo::MatchData* pMatch) override
	{
		return pMatch->pFilter->owner.fileType == "css";
	}
};

struct XmlhttprequestOption : public IOptionType
{
	virtual bool match(Proxydomo::MatchData* pMatch) override
	{
		std::wstring xrequestwith = pMatch->pFilter->owner.GetOutHeader(L"X-Requested-With");
		if (boost::iequals(xrequestwith, L"XMLHttpRequest")) {
			return true;

		} else {
			return false;
		}
	}
};

struct ThirdPartyOption : public IOptionType
{
	virtual bool match(Proxydomo::MatchData* pMatch) override
	{
		std::wstring referer = pMatch->pFilter->owner.GetOutHeader(L"Referer");
		if (referer.empty()) {
			return false;
		}
		// 直接クリックしたものは無視する
		if (pMatch->pFilter->owner.fileType == "htm") {
			return false;
		}
		CUrl urlReferer(referer);
		std::wstring refHost = urlReferer.getHost();
		std::wstring refOriginHost = ParseOriginHostName(refHost);

		std::wstring requestHost = pMatch->pFilter->owner.url.getHost();
		std::wstring originHost = ParseOriginHostName(requestHost);
		if (boost::iequals(refOriginHost, originHost) == false) {
			return true;
		} else {
			return false;
		}
	}
};

struct DomainOption : public IOptionType
{
	DomainOption(const std::wstring& domains)
	{
		std::vector<std::wstring> vecDomains;
		boost::split(vecDomains, domains, boost::is_any_of(L"|"));
		for (const std::wstring& domain : vecDomains) {
			ATLASSERT(domain.length());
			if (domain.length() >= 2) {
				if (domain.front() == L'~') {
					m_vecNegativeDomains.emplace_back(domain.substr(1));
				} else {
					m_vecDomains.emplace_back(domain);
				}
			}
		}
	}

	virtual bool match(Proxydomo::MatchData* pMatch) override
	{
		std::wstring referer = pMatch->pFilter->owner.GetOutHeader(L"Referer");
		CUrl urlReferer(referer);
		std::wstring refHost = urlReferer.getHost();

		bool bMatch = false;
		for (const std::wstring& domain : m_vecDomains) {
			if (IsDomainMatch(refHost, domain)) {
				bMatch = true;
				break;
			}
		}
		if (m_vecDomains.empty()) {
			bMatch = true;
		}
		if (bMatch) {
			for (const std::wstring& negativeDomain : m_vecNegativeDomains) {
				if (IsDomainMatch(refHost, negativeDomain)) {
					return false;
				}
			}
		}
		return bMatch;
	}

	std::vector<std::wstring> m_vecDomains;
	std::vector<std::wstring> m_vecNegativeDomains;
};

struct NegativeOption : public IOptionType
{
	NegativeOption(IOptionType* optionType) : m_optionType(std::move(optionType)) {}

	virtual bool match(Proxydomo::MatchData* pMatch) override
	{
		bool b = m_optionType->match(pMatch);
		return !b;
	}

	std::unique_ptr<IOptionType>	m_optionType;
};


std::vector<std::unique_ptr<IOptionType>> 
		CAdblockFilter::_ParseOption(const std::wstring& option)
{
	std::vector<std::unique_ptr<IOptionType>> vecOptionType;

	std::vector<std::wstring> vecOptions;
	boost::split(vecOptions, option, boost::is_any_of(L","));
	for (const std::wstring& strOption : vecOptions) {
		if (strOption == L"script") {
			vecOptionType.emplace_back(new ScriptOption);
		} else if (strOption == L"~script") {
			vecOptionType.emplace_back(new NegativeOption(new ScriptOption));
		} else if (strOption == L"image") {
			vecOptionType.emplace_back(new ImageOption);			
		} else if (strOption == L"~image") {
			vecOptionType.emplace_back(new NegativeOption(new ImageOption));
		} else if (strOption == L"stylesheet") {
			vecOptionType.emplace_back(new CssOption);
		} else if (strOption == L"~stylesheet") {
			vecOptionType.emplace_back(new NegativeOption(new CssOption));
		} else if (strOption == L"xmlhttprequest") {
			vecOptionType.emplace_back(new XmlhttprequestOption);
		} else if (strOption == L"~xmlhttprequest") {
			vecOptionType.emplace_back(new NegativeOption(new XmlhttprequestOption));
		} else if (strOption == L"third-party" || strOption == L"subdocument") {
			vecOptionType.emplace_back(new ThirdPartyOption);
		} else if (strOption == L"~third-party" || strOption == L"~subdocument") {
			vecOptionType.emplace_back(new NegativeOption(new ThirdPartyOption));			
		}else if (boost::starts_with(strOption, L"domain")) {
			vecOptionType.emplace_back(new DomainOption(strOption.substr(7)));
		} else {
			// 未実装
		}
	}
	return vecOptionType;
}

std::unique_ptr<CAdblockFilter>	LoadAdblockFilter(std::wistream& fs, const std::string& filename)
{
	auto adblockFilter = std::make_unique<CAdblockFilter>(filename);
	int nLineCount = 1;
	int successLoadLineCount = 0;
	std::wstring strLine;
	while (std::getline(fs, strLine)) {
		++nLineCount;
		if (strLine.empty() == false && strLine[0] != L'!') {
			bool bSuccess = adblockFilter->AddPattern(strLine, nLineCount);
			if (bSuccess == false) {
				CLog::FilterEvent(kLogFilterListBadLine, nLineCount, filename, "");
			} else {
				++successLoadLineCount;
			}
		}
		if (fs.eof())
			break;
	}
	CLog::FilterEvent(kLogFilterListReload, successLoadLineCount, filename, "");

	return adblockFilter;
}





#ifdef UNIT_TEST
#include <gtest\gtest.h>

#include "FilterOwner.h"

///////////////////////////////////////////////////////////////////////////////////
// AdblockFilterTest

TEST(AdblockFilter, StrMatchWildcard)
{
	auto funcWildcardTest = [](const std::wstring& pattern, const std::wstring& test) -> bool {
		const UChar* ret = StrMatchWildcard(pattern.c_str(), test.c_str(), test.c_str() + test.length());
		return ret != nullptr;
	};
	EXPECT_TRUE(funcWildcardTest(L"test", L"test"));
	EXPECT_TRUE(funcWildcardTest(L"^", L"\\"));
	EXPECT_TRUE(funcWildcardTest(L"^", L":"));
	EXPECT_FALSE(funcWildcardTest(L"^", L"a"));
	EXPECT_FALSE(funcWildcardTest(L"^", L"%"));

	EXPECT_TRUE(funcWildcardTest(L"^*/pr.js", L"/test/pr.js"));
	EXPECT_TRUE(funcWildcardTest(L"^*/pr.js", L"//pr.js"));

	EXPECT_FALSE(funcWildcardTest(L"^*/p", L"/p"));

	EXPECT_FALSE(funcWildcardTest(L"^*/pr.js", L"/pr.js"));

	EXPECT_TRUE(funcWildcardTest(L"test*abc*", L"testabc"));
	EXPECT_TRUE(funcWildcardTest(L"test*abc*", L"TESTABC"));
	EXPECT_TRUE(funcWildcardTest(L"test*abc*", L"testabcd"));
	EXPECT_TRUE(funcWildcardTest(L"test*abc*", L"test_|_abc"));
	EXPECT_FALSE(funcWildcardTest(L"test*abc*", L"test"));
	EXPECT_FALSE(funcWildcardTest(L"test*abc*", L"testa"));

	EXPECT_TRUE(funcWildcardTest(L"test|", L"test"));
	EXPECT_FALSE(funcWildcardTest(L"test|", L"test!"));

}

TEST(AdblockFilter, IOptionType)
{
	CFilterOwner owner;
	owner.SetInHeader(L"Host", L"www.host.org");
	std::wstring url = L"http://www.host.org/path/page.html?query=true#anchor";
	owner.url.parseUrl(url);
	//owner.cnxNumber = 1;
	owner.fileType = "htm";
	CFilter filter(owner);
	Proxydomo::MatchData matchData(&filter);

	ScriptOption scriptOption;
	EXPECT_FALSE(scriptOption.match(&matchData));
	owner.fileType = "js";
	EXPECT_TRUE(scriptOption.match(&matchData));

	ImageOption imageOption;
	EXPECT_FALSE(imageOption.match(&matchData));
	owner.SetInHeader(L"Content-Type", L"image/png");
	EXPECT_TRUE(imageOption.match(&matchData));

	XmlhttprequestOption xmlrequestOption;
	EXPECT_FALSE(xmlrequestOption.match(&matchData));
	owner.SetOutHeader(L"X-Requested-With", L"XMLHttpRequest");
	EXPECT_TRUE(xmlrequestOption.match(&matchData));

	ThirdPartyOption thirdpartyOption;
	owner.SetOutHeader(L"Referer", L"http://example.com/");
	EXPECT_TRUE(thirdpartyOption.match(&matchData));
	owner.SetOutHeader(L"Referer", L"http://www.host.org/");
	EXPECT_FALSE(thirdpartyOption.match(&matchData));

	{
		DomainOption domainOption(L"example.com");
		owner.SetOutHeader(L"Referer", L"http://example.com/");
		EXPECT_TRUE(domainOption.match(&matchData));
		owner.SetOutHeader(L"Referer", L"http://www.example.com/");
		EXPECT_TRUE(domainOption.match(&matchData));
		owner.SetOutHeader(L"Referer", L"http://badexample.com/");
		EXPECT_FALSE(domainOption.match(&matchData));
	}
	{
		DomainOption domainOption(L"~example.com");
		owner.SetOutHeader(L"Referer", L"http://example.com/");
		EXPECT_FALSE(domainOption.match(&matchData));
		owner.SetOutHeader(L"Referer", L"http://www.example.com/");
		EXPECT_FALSE(domainOption.match(&matchData));
		owner.SetOutHeader(L"Referer", L"http://badexample.com/");
		EXPECT_TRUE(domainOption.match(&matchData));
	}
	{
		DomainOption domainOption(L"example.com|~foo.example.com");
		owner.SetOutHeader(L"Referer", L"http://example.com/");
		EXPECT_TRUE(domainOption.match(&matchData));
		owner.SetOutHeader(L"Referer", L"http://foo.example.com/");
		EXPECT_FALSE(domainOption.match(&matchData));
	}
}

bool	MatchTestReferer(CAdblockFilter& adblockFilter, const std::wstring& test, const std::wstring& referer, bool bhtml = true)
{
	CFilterOwner owner;
	owner.SetInHeader(L"Host", L"www.host.org");
	owner.SetOutHeader(L"Referer", referer);
	std::wstring url = test;
	owner.url.parseUrl(url);
	//owner.cnxNumber = 1;
	if (bhtml) {
		owner.fileType = "htm";
	} else {
		owner.SetInHeader(L"Content-Type", L"image/png");
	}
	CFilter filter(owner);
	Proxydomo::MatchData matchData(&filter);

	return adblockFilter.match(test.c_str(), test.c_str() + test.length(), &matchData, nullptr) != nullptr;
}

bool	MatchTest(CAdblockFilter& adblockFilter, const std::wstring& test)
{
	return MatchTestReferer(adblockFilter, test, L"");
}




TEST(AdblockFilter, AddPattern)
{
	CAdblockFilter adblockFilter;

	//EXPECT_TRUE(adblockFilter.AddPattern(L"|http://ad.$script"));

	EXPECT_TRUE(adblockFilter.AddPattern(L"|http://ad."));
	EXPECT_TRUE(adblockFilter.AddPattern(L"||example.com"));
	EXPECT_TRUE(adblockFilter.AddPattern(L"||example2.com^abc"));
	EXPECT_TRUE(adblockFilter.AddPattern(L"||example3.com^abc*def"));
	EXPECT_TRUE(adblockFilter.AddPattern(L"||example4.com^a*b*c*"));
	EXPECT_TRUE(adblockFilter.AddPattern(L"||example5.com^ad.js|"));

	EXPECT_TRUE(MatchTest(adblockFilter, L"http://ad."));
	EXPECT_TRUE(MatchTest(adblockFilter, L"http://ad.test"));
	EXPECT_FALSE(MatchTest(adblockFilter, L"_http://ad."));

	EXPECT_TRUE(MatchTest(adblockFilter, L"example.com"));
	EXPECT_TRUE(MatchTest(adblockFilter, L"EXAMPLE.COM"));
	EXPECT_TRUE(MatchTest(adblockFilter, L"example.com/test"));
	EXPECT_TRUE(MatchTest(adblockFilter, L"example2.com/abc"));
	EXPECT_TRUE(MatchTest(adblockFilter, L"example2.com/abcdef"));
	EXPECT_TRUE(MatchTest(adblockFilter, L"example2.com/ABC"));
	EXPECT_TRUE(MatchTest(adblockFilter, L"example2.com:abc"));
	EXPECT_FALSE(MatchTest(adblockFilter, L"example2.com/test"));
	EXPECT_TRUE(MatchTest(adblockFilter, L"example3.com/abc_|_def"));
	EXPECT_TRUE(MatchTest(adblockFilter, L"example4.com/abc"));
	EXPECT_TRUE(MatchTest(adblockFilter, L"example4.com/a_b_c_"));
	EXPECT_FALSE(MatchTest(adblockFilter, L"example4.com/a_c_"));

	EXPECT_TRUE(MatchTest(adblockFilter, L"example5.com/ad.js"));
	EXPECT_FALSE(MatchTest(adblockFilter, L"example5.com/ad.js?"));

	EXPECT_TRUE(adblockFilter.AddPattern(L"ad"));
	EXPECT_TRUE(adblockFilter.AddPattern(L"adblocklisttest"));

	EXPECT_TRUE(MatchTest(adblockFilter, L"ad"));
	EXPECT_TRUE(MatchTest(adblockFilter, L"AD"));
	EXPECT_TRUE(MatchTest(adblockFilter, L"test_ad"));
	EXPECT_TRUE(MatchTest(adblockFilter, L"ad_test"));
	EXPECT_TRUE(MatchTest(adblockFilter, L"test_ad_test"));

	EXPECT_FALSE(MatchTest(adblockFilter, L"a"));
	EXPECT_FALSE(MatchTest(adblockFilter, L"test"));

	EXPECT_TRUE(adblockFilter.AddPattern(L"/ad*test"));
	EXPECT_TRUE(MatchTest(adblockFilter, L"/adtest"));
	EXPECT_TRUE(MatchTest(adblockFilter, L"/adBBBtestaa"));
	EXPECT_TRUE(MatchTest(adblockFilter, L"prefix/adtestaa"));

	//EXPECT_TRUE(adblockFilter.AddPattern(L"||amiami.jp^$third-party"));
	//EXPECT_TRUE(MatchTest(adblockFilter, L"http://amiami.jp/"));

	{
		EXPECT_TRUE(adblockFilter.AddPattern(L"||dmm.com^$third-party,domain=~dmm.co.jp"));

		CFilterOwner owner;
		owner.SetInHeader(L"Host", L"www.host.org");
		std::wstring url = L"http://stat.i3.dmm.com/latest/js/dmm.tracking.min.js";
		owner.url.parseUrl(url);
		//owner.cnxNumber = 1;
		owner.fileType = "htm";
		CFilter filter(owner);
		Proxydomo::MatchData matchData(&filter);

		owner.SetOutHeader(L"Referer", L"http://www.dmm.com/");
		std::wstring test = url;
		EXPECT_FALSE(adblockFilter.match(test.c_str(), test.c_str() + test.length(), &matchData, nullptr) != nullptr);
	}
	{
		EXPECT_TRUE(adblockFilter.AddPattern(L"amazon.co$third-party,~xmlhttprequest,domain=~amazon.co.jp|~amazon.com|~amazon.co.uk|~amazon.de|~aucfan.com|~book.akahoshitakuya.com|~book.asahi.com|~booklog.jp|~bookvinegar.jp|~mail.google.com|~javari.jp|~kakaku.com|~nicovideo.jp"));

		CFilterOwner owner;
		owner.SetInHeader(L"Host", L"www.amazon.co.jp");
		std::wstring url = L"https://www.amazon.co.jp/LED";
		owner.url.parseUrl(url);
		//owner.cnxNumber = 1;
		owner.fileType = "htm";
		CFilter filter(owner);
		Proxydomo::MatchData matchData(&filter);

		owner.SetOutHeader(L"Referer", L"https://www.google.co.jp");
		std::wstring test = url;
		EXPECT_FALSE(adblockFilter.match(test.c_str(), test.c_str() + test.length(), &matchData, nullptr) != nullptr);
	}
}

TEST(AdblockFilter, LoadAdblockFilter)
{
	std::wifstream fs(LR"(C:\Programing\Proxydomo\Test\touhu.txt)", std::ios::in);
	fs.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t, 0x10ffff, std::codecvt_mode::consume_header>));
	std::wstring temp;
	std::getline(fs, temp);
	auto adfilter = LoadAdblockFilter(fs, ":test:");

	EXPECT_TRUE(MatchTest(*adfilter, L"http://ad.test.jp/"));
	EXPECT_TRUE(MatchTest(*adfilter, L"http://p.dmm.com/p/top/bnr/transparent.png"));
	//adfilter->match()
	EXPECT_TRUE(MatchTest(*adfilter, L"http://imagefreak.com"));
	EXPECT_TRUE(MatchTest(*adfilter, L"http://imagefreak_test.com"));
	EXPECT_TRUE(MatchTest(*adfilter, L"http://www.imagefreak.com"));
	EXPECT_TRUE(MatchTest(*adfilter, L"http://2bee.jp/count.php"));
	EXPECT_TRUE(MatchTest(*adfilter, L"http://test.2bee.jp/count.php"));
	EXPECT_TRUE(MatchTest(*adfilter, L"http://tatsumi-sys.jp/test.asp?uid="));

	EXPECT_TRUE(MatchTestReferer(*adfilter, L"http://suvt.socdm.com/aux", L"http://www.tenki.jp/forecast", false));

	
	int a = 0;
}

TEST(AdblockFilter, element_hiding_list)
{
	{
		CAdblockFilter adblockFilter;
		EXPECT_TRUE(adblockFilter.AddPattern(L"##div"));
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"example.com") == L"div");
	}
	{
		CAdblockFilter adblockFilter;
		EXPECT_TRUE(adblockFilter.AddPattern(L"example.com##div"));
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"example.com") == L"div");
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"www.example.com") == L"div");
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"test.com") == L"");
	}
	{
		CAdblockFilter adblockFilter;
		EXPECT_TRUE(adblockFilter.AddPattern(L"example.com,test.jp##div"));
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"example.com") == L"div");
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"test.jp") == L"div");
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"jp") == L"");
	}
	{
		CAdblockFilter adblockFilter;
		EXPECT_TRUE(adblockFilter.AddPattern(L"~example.com##div"));
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"example.com") == L"");
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"www.example.com") == L"");
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"test.com") == L"div");
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"com") == L"div");
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"www.test.com") == L"div");
	}
	{
		CAdblockFilter adblockFilter;
		EXPECT_TRUE(adblockFilter.AddPattern(L"##div"));
		EXPECT_TRUE(adblockFilter.AddPattern(L"example.com#@#div"));
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"example.com") == L"");
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"www.example.com") == L"");
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"test.com") == L"div");
	}
	{
		CAdblockFilter adblockFilter;
		EXPECT_TRUE(adblockFilter.AddPattern(L"##div"));
		EXPECT_TRUE(adblockFilter.AddPattern(L"example.com##div"));
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"example.com") == L"div");
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"test.com") == L"div");

		EXPECT_TRUE(adblockFilter.AddPattern(L"###ad"));
		EXPECT_TRUE(adblockFilter.AddPattern(L"##.cc"));
		EXPECT_STREQ(adblockFilter.ElementHidingCssSelector(L"test.com").c_str(), L"#ad,.cc,div");

		EXPECT_TRUE(adblockFilter.AddPattern(L"test.com#@##ad"));
		EXPECT_STREQ(adblockFilter.ElementHidingCssSelector(L"test.com").c_str(), L".cc,div");
	}
	{
		CAdblockFilter adblockFilter;
		EXPECT_TRUE(adblockFilter.AddPattern(L"example.com,~mail.example.com##selector"));
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"example.com") == L"selector");
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"adverts.example.com") == L"selector");
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"mail.example.com") == L"");
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"www.mail.example.com") == L"");
	}
	{
		CAdblockFilter adblockFilter;
		EXPECT_TRUE(adblockFilter.AddPattern(L"~example.com,~test.jp##selector"));
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"abc.com") == L"selector");
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"example.com") == L"");
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"test.jp") == L"");
		EXPECT_TRUE(adblockFilter.ElementHidingCssSelector(L"www.example.com") == L"");
	}
}

#endif	// UNIT_TEST











