/**
*	@file	CodeConvert.cpp
*	@brief	icuを使ったコード変換
*/

#include "stdafx.h"
#include "CodeConvert.h"

#pragma comment(lib, "icuuc.lib")
#pragma comment(lib, "icuin.lib")

namespace CodeConvert {

std::string	UTF8fromUTF16(const std::wstring& text)
{
	int destLength = 0;
	UErrorCode	errCode = U_ZERO_ERROR;
	u_strToUTF8(nullptr, 0, &destLength, text.c_str(), text.length(), &errCode);
	if (destLength > 0) {
		std::vector<char> temp(destLength + 1);
		errCode = U_ZERO_ERROR;
		u_strToUTF8(temp.data(), destLength, &destLength, text.c_str(), text.length(), &errCode);
		return std::string(temp.begin(), temp.end() - 1);
	}
	return "";
}

std::wstring	UTF16fromUTF8(const std::string& text)
{
	int destLength = 0;
	UErrorCode	errCode = U_ZERO_ERROR;
	u_strFromUTF8(nullptr, 0, &destLength, text.c_str(), text.length(), &errCode);
	if (destLength > 0) {
		std::vector<wchar_t> temp(destLength + 1);
		errCode = U_ZERO_ERROR;
		u_strFromUTF8(temp.data(), destLength, &destLength, text.c_str(), text.length(), &errCode);
		return std::wstring(temp.begin(), temp.end() - 1);
	}
	return L"";
}


/// Converter -> UTF16
std::wstring	UTF16fromConverter(const char* text, int length, UConverter* pConverter)
{
	UErrorCode errCode = U_ZERO_ERROR;
	int destLength = ucnv_toUChars(pConverter, nullptr, 0, text, length, &errCode);
	if (destLength > 0) {
		std::vector<wchar_t>	temp(destLength + 1);
		errCode = U_ZERO_ERROR;
		ucnv_toUChars(pConverter, temp.data(), destLength, text, length, &errCode);
		return std::wstring(temp.begin(), temp.end() - 1);
	}
	return L"";
}

std::wstring	UTF16fromConverter(const std::string& text, UConverter* pConverter)
{
	return UTF16fromConverter(text.c_str(), text.length(), pConverter);
}


/// UTF16 -> Converter
std::string		ConvertFromUTF16(const UChar* text, int length, UConverter* pConverter)
{
	UErrorCode errCode = U_ZERO_ERROR;
	int destLength = ucnv_fromUChars(pConverter, nullptr, 0, text, length, &errCode);
	if (destLength > 0) {
		std::vector<char>	temp(destLength + 1);
		errCode = U_ZERO_ERROR;
		ucnv_fromUChars(pConverter, temp.data(), destLength, text, length, &errCode);
		return std::string(temp.begin(), temp.end() - 1);
	}
	return "";
}

std::string		ConvertFromUTF16(const std::wstring& text, UConverter* pConverter)
{
	return ConvertFromUTF16(text.c_str(), text.length(), pConverter);
}


}	// namespace CodeConvert


