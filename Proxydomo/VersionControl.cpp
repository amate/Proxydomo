// VersionControl.cpp

#include "stdafx.h"
#include "VersionControl.h"
#include <fstream>
#include <codecvt>
#include <boost\property_tree\ptree.hpp>
#include <boost\property_tree\json_parser.hpp>
#include <boost\property_tree\ini_parser.hpp>
#include <boost\property_tree\xml_parser.hpp>
#include "Misc.h"

using namespace boost::property_tree;


///////////////////////////////////////////////////////////////////
// CVersionControl

void	CVersionControl::Run()
{
	CString settingPath = Misc::GetExeDirectory() + _T("settings.ini");
	std::ifstream fs(settingPath);
	ptree pt;
	if (fs) {	// ファイルが存在しなければ各クラスに任せる
		read_ini(fs, pt);
		fs.close();

		DWORD dwVersion = pt.get<DWORD>("Setting.iniVersion", 0);
		if (dwVersion != LATESTVERSION)
			Misc::CopyToBackupFile(settingPath);

		switch (dwVersion) {
		case 0:	_0to1();
			break;
		}
		
	}
	
	// 最新バージョンを書き込む
	pt.put("Setting.iniVersion", LATESTVERSION);
	std::ofstream ofs(settingPath);
	write_ini(ofs, pt);
}

/// filter.json -> filter.xml
void	CVersionControl::_0to1()
{
	CString jsonfilterPath = Misc::GetExeDirectory() + _T("filter.json");
	if (::PathFileExists(jsonfilterPath) == FALSE)
		return ;

	std::wifstream	fs(jsonfilterPath);
	if (!fs) {
		MessageBox(NULL, _T("filter.jsonのオープンに失敗"), NULL, MB_ICONERROR);
		return ;
	}
	fs.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t>));

	wptree pt;
	read_json(fs, pt);
	fs.close();

	{
		CString filterPath = Misc::GetExeDirectory() + _T("filter.xml");
		std::wofstream	fs(filterPath);
		if (!fs) {
			MessageBox(NULL, _T("filter.xmlのオープンに失敗"), NULL, MB_ICONERROR);
			return ;
		}
		fs.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t>));

		write_xml(fs, pt, xml_parser::xml_writer_make_settings(L' ', 2, xml_parser::widen<wchar_t>("UTF-8")));

		::DeleteFile(jsonfilterPath);
	}
}
