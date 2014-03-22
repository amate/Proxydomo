#include "stdafx.h"
#include "CppUnitTest.h"
#include <chrono>
#include <random>
#include <vector>
#include <boost\format.hpp>
#include <windows.h>
//#include <Proxydomo\TextBuffer.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest1
{		
	TEST_CLASS(UnitTest1)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
			// TODO: テスト コードをここに挿入します

			enum {
				kMaxTestTextLenth = 10000,
				kMaxTestCount = 500,

			};
			std::vector<wchar_t>	vecTestText;
			vecTestText.reserve(kMaxTestTextLenth);
			std::vector<wchar_t>	vecTestLowerResult;
			vecTestLowerResult.resize(kMaxTestTextLenth);

			wchar_t testchars[] = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789*/-+";
			std::random_device	rand;
			std::mt19937	engine(rand());
			std::uniform_int_distribution<> dist(0, _countof(testchars) - 1);
			for (int i = 0; i < kMaxTestTextLenth; ++i) 
				vecTestText.push_back(testchars[dist(engine)]);
			
			auto begin = std::chrono::high_resolution_clock::now();
			for (int i = 0; i < kMaxTestCount; ++i) {
				for (wchar_t c : vecTestText) {
					wchar_t cc = towlower(c);
					vecTestLowerResult.push_back(cc);
				}
			}

			Logger::WriteMessage((boost::wformat(L"TEST towlower : %d mili/sec") % std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - begin).count()).str().c_str());

			::Sleep(1000);

			unsigned char code = 0;
			char c1 = 'A', c2 = '1';
            char c = toupper(c1);
            code = (c <= '9' ? c - '0' : c - 'A' + 10) * 16;
            c = toupper(c1);
            code += (c <= '9' ? c - '0' : c - 'A' + 10);
            
			//Logger::WriteMessage(code);

	/*		CFilterOwner filterOwner();
			CFilter	fiter(;
			CTextBuffer	textChain(*/
		}

	};
}