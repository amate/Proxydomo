
#include "stdafx.h"

#ifdef UNIT_TEST
#include <gtest\gtest.h>
#pragma comment(lib, "gtestd.lib")

#include "Nodes.h"
#include "FilterOwner.h"
#include "Matcher.h"
using namespace Proxydomo;

bool	nodeMatchTest(CNode& node, const std::wstring& test)
{
	CFilterOwner owner;
	owner.SetInHeader(L"Host", L"www.host.org");
	std::wstring url = L"http://www.host.org/path/page.html?query=true#anchor";
	owner.url.parseUrl(url);
	//owner.cnxNumber = 1;
	owner.fileType = "htm";
	CFilter filter(owner);
	Proxydomo::MatchData matchData(&filter);

	const UChar* start = test.c_str();
	const UChar* stop = test.c_str() + test.length();

	const UChar* ret = node.match(start, stop, &matchData);
	return ret != nullptr;
}

bool	nodeFullMatchTest(CNode& node, const std::wstring& test)
{
	CFilterOwner owner;
	owner.SetInHeader(L"Host", L"www.host.org");
	std::wstring url = L"http://www.host.org/path/page.html?query=true#anchor";
	owner.url.parseUrl(url);
	//owner.cnxNumber = 1;
	owner.fileType = "htm";
	CFilter filter(owner);
	Proxydomo::MatchData matchData(&filter);

	const UChar* start = test.c_str();
	const UChar* stop = test.c_str() + test.length();

	const UChar* ret = node.match(start, stop, &matchData);
	return ret != nullptr && matchData.reached == stop;
}

std::pair<bool, const UChar*>	nodeMatchReachedTest(CNode& node, std::wstring& test)
{
	CFilterOwner owner;
	owner.SetInHeader(L"Host", L"www.host.org");
	std::wstring url = L"http://www.host.org/path/page.html?query=true#anchor";
	owner.url.parseUrl(url);
	//owner.cnxNumber = 1;
	owner.fileType = "htm";
	CFilter filter(owner);
	Proxydomo::MatchData matchData(&filter);

	const UChar* start = test.c_str();
	const UChar* stop = test.c_str() + test.length();

	const UChar* ret = node.match(start, stop, &matchData);
	return { ret != nullptr, matchData.reached };
}

std::pair<bool, CFilter>	nodeMatchMemoryTest(CNode& node, std::wstring& test)
{
	CFilterOwner owner;
	owner.SetInHeader(L"Host", L"www.host.org");
	std::wstring url = L"http://www.host.org/path/page.html?query=true#anchor";
	owner.url.parseUrl(url);
	//owner.cnxNumber = 1;
	owner.fileType = "htm";
	CFilter filter(owner);
	Proxydomo::MatchData matchData(&filter);

	

	const UChar* start = test.c_str();
	const UChar* stop = test.c_str() + test.length();

	const UChar* ret = node.match(start, stop, &matchData);
	return{ ret != nullptr, filter };
}

bool	MatchTest(std::shared_ptr<CMatcher> matcher, const std::wstring& test)
{
	CFilterOwner owner;
	owner.SetInHeader(L"Host", L"www.host.org");
	std::wstring url = L"http://www.host.org/path/page.html?query=true#anchor";
	owner.url.parseUrl(url);
	//owner.cnxNumber = 1;
	owner.fileType = "htm";
	CFilter filter(owner);
	Proxydomo::MatchData matchData(&filter);

	const wchar_t* index = test.c_str();;
	const wchar_t* stop = test.c_str() + test.length();
	const wchar_t* endOfMatched = test.c_str();

	// try matching
	bool matched = matcher->match(index, stop, endOfMatched, &matchData);
	return matched;
}

bool	FullMatchTest(std::shared_ptr<CMatcher> matcher, const std::wstring& test)
{
	CFilterOwner owner;
	owner.SetInHeader(L"Host", L"www.host.org");
	std::wstring url = L"http://www.host.org/path/page.html?query=true#anchor";
	owner.url.parseUrl(url);
	//owner.cnxNumber = 1;
	owner.fileType = "htm";
	CFilter filter(owner);
	Proxydomo::MatchData matchData(&filter);

	const wchar_t* index = test.c_str();;
	const wchar_t* stop = test.c_str() + test.length();
	const wchar_t* endOfMatched = test.c_str();

	// try matching
	bool matched = matcher->match(index, stop, endOfMatched, &matchData);
	return matched && matchData.reached == (const UChar*)stop;
}

std::pair<bool, const UChar*>	MatchReachedTest(std::shared_ptr<CMatcher> matcher, std::wstring& test)
{
	CFilterOwner owner;
	owner.SetInHeader(L"Host", L"www.host.org");
	std::wstring url = L"http://www.host.org/path/page.html?query=true#anchor";
	owner.url.parseUrl(url);
	//owner.cnxNumber = 1;
	owner.fileType = "htm";
	CFilter filter(owner);
	Proxydomo::MatchData matchData(&filter);

	const wchar_t* index = test.c_str();;
	const wchar_t* stop = test.c_str() + test.length();
	const wchar_t* endOfMatched = test.c_str();

	// try matching
	bool matched = matcher->match(index, stop, endOfMatched, &matchData);
	return { matched, matchData.reached };
}

bool	NonMatchReachedTest(std::shared_ptr<CMatcher> matcher, const std::wstring& test1)
{
	auto test1ret = MatchReachedTest(matcher, const_cast<std::wstring&>(test1));
	return test1ret.first == false && test1ret.second == test1.c_str() + test1.length();
}

std::pair<bool, CFilter>	MatchMemoryTest(std::shared_ptr<CMatcher> matcher, std::wstring& test)
{
	CFilterOwner owner;
	owner.SetInHeader(L"Host", L"www.host.org");
	std::wstring url = L"http://www.host.org/path/page.html?query=true#anchor";
	owner.url.parseUrl(url);
	//owner.cnxNumber = 1;
	owner.fileType = "htm";
	CFilter filter(owner);
	Proxydomo::MatchData matchData(&filter);

	const wchar_t* index = test.c_str();;
	const wchar_t* stop = test.c_str() + test.length();
	const wchar_t* endOfMatched = test.c_str();

	// try matching
	bool matched = matcher->match(index, stop, endOfMatched, &matchData);
	return { matched, filter };
}


///////////////////////////////////////////////////////////////////////////////////
// NodeTest

TEST(NodeTest, CNode_Star)
{
	CNode_Star	nodeStar;
	nodeStar.setNextNode(nullptr);
	EXPECT_TRUE(nodeFullMatchTest(nodeStar, L"abc"));

	// *a
	CNode_Char* nodeChar = new CNode_Char(L'a');
	nodeStar.setNextNode(nodeChar);
	EXPECT_TRUE(nodeFullMatchTest(nodeStar, L"bca"));

	std::wstring test1(L"testab");
	auto test1ret = nodeMatchReachedTest(nodeStar, test1);
	EXPECT_TRUE(test1ret.first && test1ret.second == (test1.c_str() + 5));

	EXPECT_FALSE(nodeFullMatchTest(nodeStar, L"b"));

	std::wstring test2(L"bbb");
	auto test2ret = nodeMatchReachedTest(nodeStar, test2);
	EXPECT_TRUE(test2ret.first == false && test2ret.second == (test2.c_str() + test2.length()));
}

TEST(NodeTest, CNode_Space)
{
	CNode_Space nodeSpace;

	EXPECT_TRUE(nodeMatchTest(nodeSpace, L" "));
	EXPECT_TRUE(nodeMatchTest(nodeSpace, L"\t"));
	EXPECT_TRUE(nodeMatchTest(nodeSpace, L"\r"));
	EXPECT_TRUE(nodeMatchTest(nodeSpace, L"\n"));
	EXPECT_TRUE(nodeMatchTest(nodeSpace, L"a"));
	EXPECT_TRUE(nodeFullMatchTest(nodeSpace, L" "));

	EXPECT_FALSE(nodeFullMatchTest(nodeSpace, L"a"));

}

TEST(NodeTest, CNode_Equal)
{
	CNode_Equal nodeEqual;
	EXPECT_TRUE(nodeMatchTest(nodeEqual, L"="));
	EXPECT_TRUE(nodeFullMatchTest(nodeEqual, L"="));

	EXPECT_TRUE(nodeFullMatchTest(nodeEqual, L" = "));

	EXPECT_FALSE(nodeMatchTest(nodeEqual, L" "));
	EXPECT_FALSE(nodeMatchTest(nodeEqual, L"a"));
}

TEST(NodeTest, CNode_Quote)
{
	{
		CNode_Quote nodeQuote(L'"');
		EXPECT_TRUE(nodeFullMatchTest(nodeQuote, L"\""));
		EXPECT_TRUE(nodeFullMatchTest(nodeQuote, L"'"));

		EXPECT_FALSE(nodeFullMatchTest(nodeQuote, L"a"));
	}
	{
		CNode_Quote nodeQuote(L'\'');
		EXPECT_TRUE(nodeFullMatchTest(nodeQuote, L"'"));
		EXPECT_FALSE(nodeFullMatchTest(nodeQuote, L"\""));

		EXPECT_FALSE(nodeFullMatchTest(nodeQuote, L"a"));
	}

	{	// pattern ""
		CNode_Quote nodeLastQuote(L'"');
		CNode_Quote nodeOpeningQuote(L'"');
		nodeOpeningQuote.setNextNode(&nodeLastQuote);
		nodeLastQuote.setOpeningQuote(&nodeOpeningQuote);

		EXPECT_TRUE(nodeFullMatchTest(nodeOpeningQuote, LR"("")"));
		EXPECT_TRUE(nodeFullMatchTest(nodeOpeningQuote, LR"('')"));
		
		EXPECT_TRUE(nodeFullMatchTest(nodeOpeningQuote, LR"("')"));
		EXPECT_TRUE(nodeFullMatchTest(nodeOpeningQuote, LR"('")"));

	}
	{	// pattern ''
		CNode_Quote nodeLastQuote(L'\'');
		CNode_Quote nodeOpeningQuote(L'\'');
		nodeOpeningQuote.setNextNode(&nodeLastQuote);
		nodeLastQuote.setOpeningQuote(&nodeOpeningQuote);

		EXPECT_FALSE(nodeFullMatchTest(nodeOpeningQuote, LR"("")"));
		EXPECT_TRUE(nodeFullMatchTest(nodeOpeningQuote, LR"('')"));

		EXPECT_FALSE(nodeFullMatchTest(nodeOpeningQuote, LR"("')"));
		EXPECT_FALSE(nodeFullMatchTest(nodeOpeningQuote, LR"('")"));

	}
	{	// pattern '"
		CNode_Quote nodeLastQuote(L'"');
		CNode_Quote nodeOpeningQuote(L'\'');
		nodeOpeningQuote.setNextNode(&nodeLastQuote);
		nodeLastQuote.setOpeningQuote(&nodeOpeningQuote);

		EXPECT_FALSE(nodeFullMatchTest(nodeOpeningQuote, LR"("")"));
		EXPECT_TRUE(nodeFullMatchTest(nodeOpeningQuote, LR"('')"));

		EXPECT_FALSE(nodeFullMatchTest(nodeOpeningQuote, LR"("')"));
		EXPECT_TRUE(nodeFullMatchTest(nodeOpeningQuote, LR"('")"));

	}
	{	// pattern "'
		CNode_Quote nodeLastQuote(L'\'');
		CNode_Quote nodeOpeningQuote(L'"');
		nodeOpeningQuote.setNextNode(&nodeLastQuote);
		nodeLastQuote.setOpeningQuote(&nodeOpeningQuote);

		EXPECT_TRUE(nodeFullMatchTest(nodeOpeningQuote, LR"("")"));
		EXPECT_TRUE(nodeFullMatchTest(nodeOpeningQuote, LR"('')"));

		EXPECT_FALSE(nodeFullMatchTest(nodeOpeningQuote, LR"("')"));
		EXPECT_FALSE(nodeFullMatchTest(nodeOpeningQuote, LR"('")"));
	}
}

TEST(NodeTest, CNode_Char)
{
	{
		CNode_Char nodeChar(L'a');
		EXPECT_TRUE(nodeFullMatchTest(nodeChar, L"a"));
		EXPECT_FALSE(nodeFullMatchTest(nodeChar, L"z"));
	}
	{
		CNode_Char nodeChar(L'あ');
		EXPECT_TRUE(nodeFullMatchTest(nodeChar, L"あ"));
		EXPECT_FALSE(nodeFullMatchTest(nodeChar, L"z"));
	}
	{	// とりあえずサロゲートペアは非対応
		//CNode_Char nodeChar(L'𠮟');	// サロゲートペア
		//EXPECT_TRUE(nodeFullMatchTest(nodeChar, L"𠮟"));
		//EXPECT_FALSE(nodeFullMatchTest(nodeChar, L"a"));
	}
}

TEST(NodeTest, CNode_String)
{
	CNode_String nodeString(L"abc");
	EXPECT_TRUE(nodeFullMatchTest(nodeString, L"abc"));
	EXPECT_FALSE(nodeFullMatchTest(nodeString, L"d"));
	EXPECT_FALSE(nodeFullMatchTest(nodeString, L"ab"));
	EXPECT_FALSE(nodeFullMatchTest(nodeString, L"abcd"));

	std::wstring test1(L"a");
	auto test1ret = nodeMatchReachedTest(nodeString, test1);
	EXPECT_TRUE(test1ret.first == false && test1ret.second == (test1.c_str() + test1.length()));
}

TEST(NodeTest, CNode_Chars)
{
	{
		CNode_Chars nodeChars(L"abc", true);
		EXPECT_TRUE(nodeFullMatchTest(nodeChars, L"a"));
		EXPECT_TRUE(nodeFullMatchTest(nodeChars, L"c"));
		EXPECT_FALSE(nodeFullMatchTest(nodeChars, L"d"));

		std::wstring test1(L"b");
		auto test1ret = nodeMatchReachedTest(nodeChars, test1);
		EXPECT_TRUE(test1ret.first == true && test1ret.second == (test1.c_str() + test1.length()));
	}
	{
		CNode_Chars nodeChars(L"abc", false);
		EXPECT_FALSE(nodeFullMatchTest(nodeChars, L"a"));
		EXPECT_FALSE(nodeFullMatchTest(nodeChars, L"c"));
		EXPECT_TRUE(nodeFullMatchTest(nodeChars, L"d"));

		std::wstring test1(L"d");
		auto test1ret = nodeMatchReachedTest(nodeChars, test1);
		EXPECT_TRUE(test1ret.first == true && test1ret.second == (test1.c_str() + test1.length()));
	}
}

TEST(NodeTest, CNode_Empty)
{
	CNode_Empty nodeEmpty;
	EXPECT_TRUE(nodeFullMatchTest(nodeEmpty, L""));

	EXPECT_TRUE(nodeMatchTest(nodeEmpty, L"a"));
	EXPECT_FALSE(nodeFullMatchTest(nodeEmpty, L"a"));

}

TEST(NodeTest, CNode_Any)
{
	CNode_Any nodeAny;
	EXPECT_TRUE(nodeFullMatchTest(nodeAny, L"a"));
	EXPECT_FALSE(nodeFullMatchTest(nodeAny, L""));

}

TEST(NodeTest, CNode_Memory)
{
	CNode_Char* nodeChar = new CNode_Char(L'a');
	CNode_Memory nodeMemory(nodeChar, 0);
	nodeMemory.setNextNode(nullptr);
	std::wstring test1(L"a");
	auto test1ret = nodeMatchMemoryTest(nodeMemory, test1);
	EXPECT_TRUE(test1ret.first && test1ret.second.memoryTable[0].getValue() == L"a");

	EXPECT_FALSE(nodeFullMatchTest(nodeMemory, L"b"));

	std::wstring test2(L"b");
	auto test2ret = nodeMatchMemoryTest(nodeMemory, test2);
	EXPECT_TRUE(test2ret.first == false && test2ret.second.memoryTable[0].getValue().empty());

}

TEST(NodeTest, CNode_Negate)
{
	CNode_Char* nodeChar = new CNode_Char(L'a');
	CNode_Negate nodeNegate(nodeChar);

	EXPECT_FALSE(nodeMatchTest(nodeNegate, L"a"));
	EXPECT_TRUE(nodeMatchTest(nodeNegate, L"b"));

}

TEST(NodeTest, CNode_AV)
{
	{
		CNode_Char* nodeChar = new CNode_Char(L'a');
		CNode_AV nodeAv(nodeChar, false);
		EXPECT_TRUE(nodeFullMatchTest(nodeAv, LR"("a")"));
		EXPECT_TRUE(nodeFullMatchTest(nodeAv, LR"('a')"));
		EXPECT_TRUE(nodeFullMatchTest(nodeAv, LR"(a)"));

		EXPECT_FALSE(nodeFullMatchTest(nodeAv, L"b"));
	}
	{
		CNode_Char* nodeChar = new CNode_Char(L'a');
		CNode_AV nodeAvq(nodeChar, true);
		EXPECT_FALSE(nodeFullMatchTest(nodeAvq, LR"("a")"));
		EXPECT_FALSE(nodeFullMatchTest(nodeAvq, LR"('a')"));
		EXPECT_TRUE(nodeFullMatchTest(nodeAvq, LR"(a)"));

		EXPECT_FALSE(nodeFullMatchTest(nodeAvq, L"b"));
	}
}

TEST(NodeTest, CNode_Url)
{
	{
		CNode_Url nodeUrl(L'u');
		EXPECT_TRUE(nodeFullMatchTest(nodeUrl, L"http://www.host.org/path/page.html?query=true#anchor"));
		EXPECT_FALSE(nodeFullMatchTest(nodeUrl, L"http://www.example.org/path/page.html?query=true#anchor"));
	}
	{
		CNode_Url nodeUrl(L'h');
		EXPECT_TRUE(nodeFullMatchTest(nodeUrl, L"www.host.org"));
		EXPECT_FALSE(nodeFullMatchTest(nodeUrl, L"www.example.org"));
	}
	{
		CNode_Url nodeUrl(L'p');
		EXPECT_TRUE(nodeFullMatchTest(nodeUrl, L"/path/page.html"));
		EXPECT_FALSE(nodeFullMatchTest(nodeUrl, L"/path/index.html"));
	}
	{
		CNode_Url nodeUrl(L'q');
		EXPECT_TRUE(nodeFullMatchTest(nodeUrl, L"?query=true"));
		EXPECT_FALSE(nodeFullMatchTest(nodeUrl, L"?query=false"));
	}
	{
		CNode_Url nodeUrl(L'a');
		EXPECT_TRUE(nodeFullMatchTest(nodeUrl, L"#anchor"));
		EXPECT_FALSE(nodeFullMatchTest(nodeUrl, L"#test"));
	}
}


TEST(MatchTest, MemStar)
{
	{
		auto matcher = CMatcher::CreateMatcher(L"\\0");
		std::wstring test1 = L"abc";
		auto test1ret = MatchMemoryTest(matcher, test1);
		EXPECT_TRUE(test1ret.first && test1ret.second.memoryTable[0].getValue() == L"abc");
	}
	{
		auto matcher = CMatcher::CreateMatcher(L"\\0d");
		std::wstring test1 = L"abcd";
		auto test1ret = MatchMemoryTest(matcher, test1);
		EXPECT_TRUE(test1ret.first && test1ret.second.memoryTable[0].getValue() == L"abc");
		EXPECT_TRUE(FullMatchTest(matcher, test1));
		EXPECT_FALSE(MatchTest(matcher, L"abc"));

		std::wstring test2 = L"d";
		auto test2ret = MatchMemoryTest(matcher, test2);
		EXPECT_TRUE(test2ret.first && test2ret.second.memoryTable[0].getValue().empty());
		EXPECT_TRUE(FullMatchTest(matcher, test2));
	}
}

TEST(MatchTest, Or)
{
	{
		auto matcher = CMatcher::CreateMatcher(L"a|b");
		EXPECT_TRUE(FullMatchTest(matcher, L"a"));
		EXPECT_TRUE(FullMatchTest(matcher, L"b"));
		EXPECT_FALSE(FullMatchTest(matcher, L"c"));
	}
	{
		auto matcher = CMatcher::CreateMatcher(L"(a|b)c");
		EXPECT_TRUE(FullMatchTest(matcher, L"ac"));
		EXPECT_TRUE(FullMatchTest(matcher, L"bc"));
		EXPECT_FALSE(FullMatchTest(matcher, L"cc"));

		EXPECT_TRUE(NonMatchReachedTest(matcher, L"a"));
	}
}

TEST(MatchTest, And)
{
	{
		auto matcher = CMatcher::CreateMatcher(L"*a&*b");
		EXPECT_TRUE(FullMatchTest(matcher, L"ab"));
		EXPECT_TRUE(FullMatchTest(matcher, L"ba"));
		EXPECT_TRUE(FullMatchTest(matcher, L"testatestb"));
		EXPECT_FALSE(FullMatchTest(matcher, L"a"));
		EXPECT_FALSE(FullMatchTest(matcher, L"b"));

		EXPECT_TRUE(NonMatchReachedTest(matcher, L"test"));
	}
	{
		auto matcher = CMatcher::CreateMatcher(L"(*a*&*b*)c");
		EXPECT_TRUE(FullMatchTest(matcher, L"abc"));
		EXPECT_FALSE(FullMatchTest(matcher, L"ac"));
		EXPECT_TRUE(NonMatchReachedTest(matcher, L"test"));
	}
	{
		auto matcher = CMatcher::CreateMatcher(LR"(*src="picture" & *height=60 & *width=200)");
		EXPECT_TRUE(FullMatchTest(matcher, LR"(src="picture" height=60 width=200)"));
		EXPECT_TRUE(FullMatchTest(matcher, LR"(width=200 height=60 src="picture")"));
		EXPECT_FALSE(FullMatchTest(matcher, LR"(width=200 src="picture")"));

		EXPECT_TRUE(NonMatchReachedTest(matcher, L"src"));
		EXPECT_TRUE(NonMatchReachedTest(matcher, L"width"));
	}
}

TEST(MatchTest, AndAnd)
{
	auto matcher = CMatcher::CreateMatcher(L"<img *>&&\\0");
	EXPECT_TRUE(FullMatchTest(matcher, L"<img >"));
	EXPECT_TRUE(NonMatchReachedTest(matcher, L"<img "));

	std::wstring test1 = L"<img src=test>";
	auto test1ret = MatchMemoryTest(matcher, test1);
	EXPECT_TRUE(test1ret.first && test1ret.second.memoryTable[0].getValue() == test1);

}

TEST(MatchTest, Repeat)
{
	{
		auto matcher = CMatcher::CreateMatcher(L"a+");
		EXPECT_TRUE(FullMatchTest(matcher, L"a"));
		EXPECT_TRUE(FullMatchTest(matcher, L"aaa"));
		EXPECT_TRUE(MatchTest(matcher, L"b"));
	}
	{
		auto matcher = CMatcher::CreateMatcher(L"a++b");
		EXPECT_TRUE(FullMatchTest(matcher, L"ab"));
		EXPECT_TRUE(FullMatchTest(matcher, L"aaab"));
		EXPECT_TRUE(FullMatchTest(matcher, L"b"));
		EXPECT_TRUE(NonMatchReachedTest(matcher, L"aa"));
	}
}

TEST(MatchTest, Nest)
{
	{
		auto matcher = CMatcher::CreateMatcher(L"$NEST(<div,</div>)");
		EXPECT_TRUE(FullMatchTest(matcher, L"<div class=test></div>"));
		EXPECT_TRUE(NonMatchReachedTest(matcher, L"<"));
		EXPECT_TRUE(NonMatchReachedTest(matcher, L"<div class=test"));
		EXPECT_TRUE(NonMatchReachedTest(matcher, L"<div class=test></"));
	}
	{
		auto matcher = CMatcher::CreateMatcher(L"$NEST(<div, class=test*,</div>)");
		EXPECT_TRUE(FullMatchTest(matcher, L"<div class=test></div>"));
		EXPECT_TRUE(NonMatchReachedTest(matcher, L"<"));
		EXPECT_TRUE(NonMatchReachedTest(matcher, L"<div class=test"));
		EXPECT_TRUE(NonMatchReachedTest(matcher, L"<div class=test></"));
		EXPECT_TRUE(FullMatchTest(matcher, L"<div class=test> <div class=abc></div> </div>"));
		EXPECT_TRUE(NonMatchReachedTest(matcher, L"<div class=test><div "));
		EXPECT_TRUE(NonMatchReachedTest(matcher, L"<div class=test><div class=abc></div>"));
	}
}

TEST(MatchTest, INest)
{
	auto matcher = CMatcher::CreateMatcher(L"<div class=outer>$INEST(<div,</div>)</div>");
	EXPECT_TRUE(FullMatchTest(matcher, L"<div class=outer> <div class=abc></div> </div>"));
	EXPECT_TRUE(FullMatchTest(matcher, L"<div class=outer></div>"));
	EXPECT_TRUE(NonMatchReachedTest(matcher, L"<"));
	EXPECT_TRUE(NonMatchReachedTest(matcher, L"<div class=outer"));
	EXPECT_TRUE(NonMatchReachedTest(matcher, L"<div class=outer></"));
	EXPECT_TRUE(NonMatchReachedTest(matcher, L"<div class=outer> <div class=abc></div>"));
}

TEST(MatchTest, TST)
{
	{
		auto matcher = CMatcher::CreateMatcher(L"<\\0 *></$TST(\\0)>");
		EXPECT_TRUE(FullMatchTest(matcher, L"<div class=outer></div>"));
		EXPECT_FALSE(MatchTest(matcher, L"<div class=outer></span>"));
		EXPECT_TRUE(NonMatchReachedTest(matcher, L"<div class=outer></di"));
	}
	{
		auto matcher = CMatcher::CreateMatcher(L"<\\0 *>$TST(\\0=div)");
		EXPECT_TRUE(FullMatchTest(matcher, L"<div class=outer>"));
		EXPECT_FALSE(MatchTest(matcher, L"<span class=outer>"));
	}
}


#include "FilterOwner.h"

TEST(FilterOwner, CleanHeader)
{
	HeadPairList headers;
	headers.emplace_back(L"name", L"");

	CFilterOwner::CleanHeader(headers);
	EXPECT_TRUE(headers.empty());
}

#endif	// UNIT_TEST
