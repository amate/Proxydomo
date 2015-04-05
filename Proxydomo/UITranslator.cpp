/**
*	@file	UITranslator.cpp
*/

#include "stdafx.h"
#include "UITranslator.h"
#include <unordered_map>
#include <fstream>
#include <codecvt>
#include <boost\lexical_cast.hpp>
#include "Misc.h"
#include "proximodo\util.h"
#include "Settings.h"

namespace {

	std::unordered_map<int, std::wstring> g_mapTranslateFormat;
	CFont	g_fontJapanese;

}	// namespace


namespace UITranslator {

	void LoadUILanguage()
	{		
		CString traslateFilePath = Misc::GetExeDirectory() + L"lang\\" + CSettings::s_language.c_str() + L".lng";

		std::wifstream fs(traslateFilePath, std::ios::in | std::ios::binary);
		if (!fs) {
			MessageBox(NULL, L"language file load failed", NULL, MB_ICONERROR);
			return;
		}
		fs.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t, 0x10ffff, std::codecvt_mode::consume_header>));
		
		std::wstring strLine;
		while (std::getline(fs, strLine)) {
			if (strLine.length() && strLine[0] != L'#') {
				auto posEqual = strLine.find(L'=');
				if (posEqual == std::wstring::npos)
					continue;

				std::wstring id = strLine.substr(0, posEqual);
				std::wstring format = strLine.substr(posEqual + 1);
				format = CUtil::replaceAll(format, L"\r", L"");
				format = CUtil::replaceAll(format, L"\\r", L"\r");
				format = CUtil::replaceAll(format, L"\\n", L"\n");
				try {
					int translateId = boost::lexical_cast<int>(id);
					g_mapTranslateFormat.insert(std::make_pair(translateId, format));
				}
				catch (boost::bad_lexical_cast&) {
					ATLASSERT(FALSE);
				}
			}
			if (fs.eof())
				break;
		}

		if (::_wcsicmp(CSettings::s_language.c_str(), L"Japanese") == 0) {
			CLogFont lf;
			lf.SetMenuFont();
			::wcscpy_s(lf.lfFaceName, L"Meiryo UI");
			lf.SetHeight(9);
			g_fontJapanese = lf.CreateFontIndirect();
		}
	}

	std::wstring	getTranslateFormat(int translateId)
	{
		auto it = g_mapTranslateFormat.find(translateId);
		if (it != g_mapTranslateFormat.end())
			return it->second;
		return shand::format(L"%1% : No translateMessage", translateId).str();
	}

	CFontHandle		getFont()
	{
		return (HFONT)g_fontJapanese;
	}


}	// namespace UITranslator




























