
#include "stdafx.h"
#include "DomainJudge.h"
#include "Misc.h"
#include <string>
#include <fstream>
#include <codecvt>
#include <unordered_map>
#include <memory>
#include "Logger.h"

std::deque<std::wstring>	SpliteDomain(const std::wstring& urlHost);

namespace {

LPCWSTR kPublicSuffixListPath = L"lists\\public_suffix_list.dat";

struct DomainUnit {
	std::unordered_map<std::wstring, std::unique_ptr<DomainUnit>>	mapChildDomainSuffixList;
	//std::vector<NodeOption>	vecNode;
};
std::unordered_map<std::wstring, std::unique_ptr<DomainUnit>>	DomainSuffixList;

bool	AddDomainSuffixList(const std::wstring& strLine)
{
	std::deque<std::wstring> deqDomain = SpliteDomain(strLine);
	auto	pmapChildDomainSuffixList = &DomainSuffixList;
	for (auto it = deqDomain.rbegin(); it != deqDomain.rend(); ++it) {
		auto& pHash = (*pmapChildDomainSuffixList)[*it];
		if (pHash == nullptr)
			pHash.reset(new DomainUnit);

		if (std::next(it) == deqDomain.rend()) {
			return true;
		}
		pmapChildDomainSuffixList = &pHash->mapChildDomainSuffixList;
	}
	ATLASSERT(FALSE);
	return true;
}

}	// namespace

bool	Load_public_suffix_list()
{
	std::wstring filePath = static_cast<LPCWSTR>(Misc::GetExeDirectory() + kPublicSuffixListPath);

	std::wifstream fs(filePath, std::ios::in);
	fs.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t, 0x10ffff, std::codecvt_mode::consume_header>));
	if (!fs) {
		ATLASSERT(FALSE);
		ERROR_LOG << L"Load_public_suffix_list : public_suffix_list.dat open failed";
		return false;
	}

	std::wstring strLine;
	while (std::getline(fs, strLine)) {
		if (strLine == L"// ===END ICANN DOMAINS===") {
			break;
		}
		if (strLine.length() > 0 && strLine.front() != L'/') {
			AddDomainSuffixList(strLine);
		}

		if (fs.eof()) {
			break;
		}
	}
	return true;
}

std::wstring ParseOriginHostName(const std::wstring& host)
{
	std::wstring originHostName;

	std::deque<std::wstring> deqDomain = SpliteDomain(host);
	auto	pmapChildDomainSuffixList = &DomainSuffixList;
	for (auto it = deqDomain.rbegin(); it != deqDomain.rend(); ++it) {
		auto itfound = pmapChildDomainSuffixList->find(*it);
		if (itfound == pmapChildDomainSuffixList->end()) {
			originHostName.insert(0, *it);

			// * ‚ª‚ ‚ê‚Î‚³‚ç‚É‘O‚ÌƒhƒƒCƒ“‚ð’Ç‰Á‚·‚é
			auto itfoundastarisk = pmapChildDomainSuffixList->find(std::wstring(L"*"));
			if (itfoundastarisk != pmapChildDomainSuffixList->end()) {
				if (std::next(it) != deqDomain.rend()) {
					originHostName.insert(0, L".");
					originHostName.insert(0, *std::next(it));
				} else {
					ATLASSERT(FALSE);	// *.ck ‚Å test.ck ‚È‚Ç‚ª—^‚¦‚ç‚ê‚½‚Æ‚«
				}
			}
			break;
		}
		if (std::next(it) == deqDomain.rend()) {	// tokyo.jp ‚É tokyo.jp ‚ð—^‚¦‚½‚Æ‚«‚Æ‚©
			originHostName.insert(0, *it);
			break;
		}

		originHostName.insert(0, *it);
		originHostName.insert(0, L".");
		pmapChildDomainSuffixList = &itfound->second->mapChildDomainSuffixList;
	}
	ATLASSERT(originHostName.length());
	return originHostName;
}



#ifdef UNIT_TEST
#include <gtest\gtest.h>

TEST(DomainJudge, 1)
{
	AddDomainSuffixList(L"bb");

	EXPECT_TRUE(ParseOriginHostName(L"test.bb") == L"test.bb");
	EXPECT_TRUE(ParseOriginHostName(L"www.test.bb") == L"test.bb");
}

TEST(DomainJudge, test)
{
	ASSERT_TRUE(Load_public_suffix_list());

	EXPECT_TRUE(ParseOriginHostName(L"dmm.com") == L"dmm.com");
	EXPECT_TRUE(ParseOriginHostName(L"www.dmm.com") == L"dmm.com");
	EXPECT_TRUE(ParseOriginHostName(L"stat.i3.dmm.com") == L"dmm.com");

	EXPECT_TRUE(ParseOriginHostName(L"tokyo.jp") == L"tokyo.jp");
	EXPECT_TRUE(ParseOriginHostName(L"adachi.tokyo.jp") == L"adachi.tokyo.jp");


}

#endif	// UNIT_TEST

