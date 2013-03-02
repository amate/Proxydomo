#include "stdafx.h"
#include "CppUnitTest.h"
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