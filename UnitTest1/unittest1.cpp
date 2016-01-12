#include "stdafx.h"
#include "CppUnitTest.h"
#include <chrono>
#include <random>
#include <vector>
#include <boost\format.hpp>
#include <windows.h>
//#include <Proxydomo\TextBuffer.h>
#include <Proxydomo\Settings.h>
#include <Proxydomo\Matcher.h>
#include <Proxydomo\FilterOwner.h>
#include <Proxydomo\proximodo\filter.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest1
{		
	TEST_CLASS(UnitTest1)
	{
	public:

		void	MatchTest(std::shared_ptr<Proxydomo::CMatcher> matcher, const std::wstring& test)
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
			
			Assert::IsTrue(matched || matchData.reached == stop);
		}
		
		TEST_METHOD(TestMethod1)
		{
			// TODO: テスト コードをここに挿入します

			CSettings::LoadSettings();

			std::wstring pattern = L"(^(^<(table|div|p|dl|ol|ul|li|center)\\0[^>]++(class|id)=$AV($LST(IdClassList)))) $NEST(<$TST(\\0)(\\s*|)>,</$TST(\\0) >)";
			auto matcher = Proxydomo::CMatcher::CreateMatcher(pattern);


			std::wstring test1 = L"<table class=\"infobox\" style=\"width:22em;\"><tr><th colspan=\"2\" style=\"text-align:center; font-size:125%; font-weight:bold; color: black; background-color: #dddddd;\">日本語<br /></th></tr><tr><td colspan=\"2\" style=\"text-align:center; font-weight: bold; color: black; background-color: #dddddd;\"><small>にほんご、にっぽんご</small></td></tr><tr><th scope=\"row\" style=\"text-align:left; white-space:nowrap; width: 5.5em; padding-left: 0.5em;\">発音</th><td style=\"padding-left: 0.5em;\"><a href=\"/wiki/%E5%9B%BD%E9%9A%9B%E9%9F%B3%E5%A3%B0%E8%A8%98%E5%8F%B7\" title=\"国際音声記号\">IPA</a>: <span title=\"IPA発音記号\" class=\"IPA\"><span class=\"IPA\" title=\"国際音声記号 (IPA) 表記\" style=\"font-size: 103%;\">[nʲiho̞ŋŋo]</span> (<span class=\"IPA\" title=\"国際音声記号 (IPA) 表記\" style=\"font-size: 103%;\">[nʲip̚poŋŋo]</span>)</span></td></tr><tr><th scope=\"row\" style=\"text-align:left; white-space:nowrap; width: 5.5em; padding-left: 0.5em;\">話される国</th><td style=\"padding-left: 0.5em;\"><a href=\"/wiki/%E6%97%A5%E6%9C%AC\" title=\"日本\">日本</a>など（詳細は「<a href=\"#.E5.88.86.E5.B8.83\">分布</a>」を参照）　　.</td></tr><tr><th scope=\"row\" style=\"text-align:left; white-space:nowrap; width: 5.5em; padding-left: 0.5em;\">地域</th><td style=\"padding-left: 0.5em;\"><a href=\"/wiki/%E6%9D%B1%E3%82%A2%E3%82%B8%E3%82%A2\" title=\"東アジア\">東アジア</a>など</td></tr><tr><th scope=\"row\" style=\"text-align:left; white-space:nowrap; width: 5.5em; padding-left: 0.5em;\">話者数</th><td style=\"padding-left: 0.5em;\">約1億3,000万人（日本の人口を基にした場合の概数）</td></tr><tr><th scope=\"row\" style=\"text-align:left; white-space:nowrap; width: 5.5em; padding-left: 0.5em;\"><a href=\"/wiki/%E3%83%8D%E3%82%A4%E3%83%86%E3%82%A3%E3%83%96%E3%82%B9%E3%83%94%E3%83%BC%E3%82%AB%E3%83%BC%E3%81%AE%E6%95%B0%E3%81%8C%E5%A4%9A%E3%81%84%E8%A8%80%E8%AA%9E%E3%81%AE%E4%B8%80%E8%A6%A7\" title=\"ネイティブスピーカーの数が多い言語の一覧\">話者数の順位</a></th><td style=\"padding-left: 0.5em;\">9</td></tr><tr><th scope=\"row\" style=\"text-align:left; white-space:nowrap; width: 5.5em; padding-left: 0.5em;\"><a href=\"/wiki/%E8%A8%80%E8%AA%9E%E3%81%AE%E3%82%B0%E3%83%AB%E3%83%BC%E3%83%97%E3%81%AE%E4%B8%80%E8%A6%A7\" title=\"言語のグループの一覧\">言語系統</a></th><td style=\"padding-left: 0.5em;\"><div style=\"text-align:left;\"><a href=\"/wiki/%E5%AD%A4%E7%AB%8B%E3%81%97%E3%81%9F%E8%A8%80%E8%AA%9E\" title=\"孤立した言語\">孤立した言語</a><br /><a href=\"/wiki/%E3%82%A2%E3%83%AB%E3%82%BF%E3%82%A4%E8%AB%B8%E8%AA%9E\" title=\"アルタイ諸語\">アルタイ系</a>説、<a href=\"/wiki/%E3%82%AA%E3%83%BC%E3%82%B9%E3%83%88%E3%83%AD%E3%83%8D%E3%82%B7%E3%82%A2%E8%AA%9E%E6%97%8F\" title=\"オーストロネシア語族\">南島系</a>説など論争あり<br /><a href=\"/wiki/%E7%90%89%E7%90%83%E8%AA%9E\" title=\"琉球語\">琉球語</a>と併せて「<a href=\"/wiki/%E6%97%A5%E6%9C%AC%E8%AA%9E%E6%97%8F\" title=\"日本語族\">日本語族</a>」とすることもある（詳細は「<a href=\"#.E7.B3.BB.E7.B5.B1\">系統</a>」を参照）</div></td></tr><tr><th scope=\"row\" style=\"text-align:left; white-space:nowrap; width: 5.5em; padding-left: 0.5em;\"><a href=\"/wiki/%E8%A1%A8%E8%A8%98%E4%BD%93%E7%B3%BB\" title=\"表記体系\" class=\"mw-redirect\">表記体系</a></th><td style=\"padding-left: 0.5em;\"><a href=\"/wiki/%E5%B9%B3%E4%BB%AE%E5%90%8D\" title=\"平仮名\">平仮名</a>、<a href=\"/wiki/%E7%89%87%E4%BB%AE%E5%90%8D\" title=\"片仮名\">片仮名</a>、<a href=\"/wiki/%E6%97%A5%E6%9C%AC%E3%81%AB%E3%81%8A%E3%81%91%E3%82%8B%E6%BC%A2%E5%AD%97\" title=\"日本における漢字\">漢字</a></td></tr><tr><th scope=\"col\" colspan=\"2\" style=\"text-align:center; color: black; background-color: #dddddd;\">公的地位</th></tr><tr><th scope=\"row\" style=\"text-align:left; white-space:nowrap; width: 5.5em; padding-left: 0.5em;\">公用語</th><td style=\"padding-left: 0.5em;\"><a href=\"/wiki/%E3%83%95%E3%82%A1%E3%82%A4%E3%83%AB:Flag_of_Japan.svg\" class=\"image\" title=\"日本の旗\"><img alt=\"日本の旗\" src=\"//upload.wikimedia.org/wikipedia/commons/thumb/9/9e/Flag_of_Japan.svg/25px-Flag_of_Japan.svg.png\" width=\"25\" height=\"17\" class=\"thumbborder\" srcset=\"//upload.wikimedia.org/wikipedia/commons/thumb/9/9e/Flag_of_Japan.svg/38px-Flag_of_Japan.svg.png 1.5x, //upload.wikimedia.org/wikipedia/commons/thumb/9/9e/Flag_of_Japan.svg/50px-Flag_of_Japan.svg.png 2x\" data-file-width=\"900\" data-file-height=\"600\" /></a> <a href=\"/wiki/%E6%97%A5%E6%9C%AC\" title=\"日本\">日本</a>（事実上。詳細は「<a href=\"#.E5.88.86.E5.B8.83\">分布</a>」を参照）<br /><a href=\"/wiki/%E3%83%95%E3%82%A1%E3%82%A4%E3%83%AB:Flag_of_Palau.svg\" class=\"image\" title=\"パラオの旗\"><img alt=\"パラオの旗\" src=\"//upload.wikimedia.org/wikipedia/commons/thumb/4/48/Flag_of_Palau.svg/25px-Flag_of_Palau.svg.png\" width=\"25\" height=\"16\" class=\"thumbborder\" srcset=\"//upload.wikimedia.org/wikipedia/commons/thumb/4/48/Flag_of_Palau.svg/38px-Flag_of_Palau.svg.png 1.5x, //upload.wikimedia.org/wikipedia/commons/thumb/4/48/Flag_of_Palau.svg/50px-Flag_of_Palau.svg.png 2x\" data-file-width=\"800\" data-file-height=\"500\" /></a> <a href=\"/wiki/%E3%83%91%E3%83%A9%E3%82%AA\" title=\"パラオ\">パラオ</a> <a href=\"/wiki/%E3%82%A2%E3%83%B3%E3%82%AC%E3%82%A6%E3%83%AB%E5%B7%9E\" title=\"アンガウル州\">アンガウル州</a></td></tr><tr><th scope=\"row\" style=\"text-align:left; white-space:nowrap; width: 5.5em; padding-left: 0.5em;\">統制機関</th><td style=\"padding-left: 0.5em;\">（特になし）<br /><a href=\"/wiki/%E3%83%95%E3%82%A1%E3%82%A4%E3%83%AB:Flag_of_Japan.svg\" class=\"image\" title=\"日本の旗\"><img alt=\"日本の旗\" src=\"//upload.wikimedia.org/wikipedia/commons/thumb/9/9e/Flag_of_Japan.svg/25px-Flag_of_Japan.svg.png\" width=\"25\" height=\"17\" class=\"thumbborder\" srcset=\"//upload.wikimedia.org/wikipedia/commons/thumb/9/9e/Flag_of_Japan.svg/38px-Flag_of_Japan.svg.png 1.5x, //upload.wikimedia.org/wikipedia/commons/thumb/9/9e/Flag_of_Japan.svg/50px-Flag_of_Japan.svg.png 2x\" data-file-width=\"900\" data-file-height=\"600\" /></a> <a href=\"/wiki/%E6%97%A5%E6%9C%AC\" title=\"日本\">日本</a>：<a href=\"/wiki/%E6%96%87%E5%8C%96%E5%BA%81\" title=\"文化庁\">文化庁</a><a href=\"/wiki/%E6%96%87%E5%8C%96%E5%AF%A9%E8%AD%B0%E4%BC%9A\" title=\"文化審議会\">文化審議会</a>国語分科会（事実上）</td></tr><tr><th scope=\"col\" colspan=\"2\" style=\"text-align:center; color: black; background-color: #dddddd;\">言語コード</th></tr><tr><th scope=\"row\" style=\"text-align:left; white-space:nowrap; width: 5.5em; padding-left: 0.5em;\"><a href=\"/wiki/ISO_639-1\" title=\"ISO 639-1\">ISO 639-1</a></th><td style=\"padding-left: 0.5em;\"><tt>ja</tt></td></tr><tr><th scope=\"row\" style=\"text-align:left; white-space:nowrap; width: 5.5em; padding-left: 0.5em;\"><a href=\"/wiki/ISO_639-2\" title=\"ISO 639-2\">ISO 639-2</a></th><td style=\"padding-left: 0.5em;\"><tt>jpn</tt></td></tr><tr><th scope=\"row\" style=\"text-align:left; white-space:nowrap; width: 5.5em; padding-left: 0.5em;\"><a href=\"/wiki/ISO_639-3\" title=\"ISO 639-3\">ISO 639-3</a></th><td style=\"padding-left: 0.5em;\"><tt><a href=\"http://www.sil.org/iso639-3/documentation.asp?id=jpn\" class=\"extiw\" title=\"iso639-3:jpn\">jpn</a></tt></td></tr><tr><th scope=\"row\" style=\"text-align:left; white-space:nowrap; width: 5.5em; padding-left: 0.5em;\"><a href=\"/wiki/%E5%9B%BD%E9%9A%9BSIL#SIL.E3.82.B3.E3.83.BC.E3.83.89\" title=\"国際SIL\">SIL</a></th><td style=\"padding-left: 0.5em;\">JPN</td></tr><tr class=\"noprint\"><td colspan=\"2\" style=\"text-align:right; font-size:85%;\"><a href=\"/wiki/Template:Infobox_Language\" title=\"Template:Infobox Language\">テンプレートを表示</a></td></tr></table>";

			MatchTest(matcher, test1);

			std::wstring test2 = L"<ta";
			MatchTest(matcher, test2);

			std::wstring test3 = L"<table";
			MatchTest(matcher, test3);

			std::wstring test4 = L"<table class=\"info";
			MatchTest(matcher, test4);

			std::wstring test5 = L"<table class=\"infobox\" style=\"width:22em;\">";
			MatchTest(matcher, test5);

			std::wstring test6 = L"<table class=\"infobox\" style=\"width:22em;\"> test ";
			MatchTest(matcher, test6);

			std::wstring test7 = L"<table class=\"infobox\" style=\"width:22em;\"> test </table>";
			MatchTest(matcher, test7);
		}

	};
}