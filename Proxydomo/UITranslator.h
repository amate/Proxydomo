/**
*	@file	UITranslator.h
*/

#pragma once

#include <string>
#include <atlwin.h>
#include <atlapp.h>
#include <atlgdi.h>
#include "format.h"

namespace UITranslator {


	void LoadUILanguage();

	std::wstring	getTranslateFormat(int translateId);
	CFontHandle		getFont();

	inline std::wstring GetTranslateMessage(int translateId)
	{
		return getTranslateFormat(translateId);
	}

	template <class... Args>
	std::wstring GetTranslateMessage(int translateId, const Args&... args)
	{
		try {
			std::wstring translatedText = shand::format(getTranslateFormat(translateId).c_str(), args...).str();
			return translatedText;
		}
		catch (...) {
			return getTranslateFormat(translateId);
		}
	}

	inline void	ChangeControlTextForTranslateMessage(CWindow parentWindow, int ctrlID)
	{
		CWindow ctrlItem = parentWindow.GetDlgItem(ctrlID);
		ATLASSERT(ctrlItem.IsWindow());
		if (ctrlItem.IsWindow() == FALSE)
			return;
		ctrlItem.SetWindowTextW(GetTranslateMessage(ctrlID).c_str());

		CFontHandle font = getFont();
		if (font)
			ctrlItem.SetFont(font);
	}

	template <class... Args>
	void	ChangeControlTextForTranslateMessage(CWindow parentWindow, int ctrlID, const Args&... args)
	{
		CWindow ctrlItem = parentWindow.GetDlgItem(ctrlID);
		ATLASSERT(ctrlItem.IsWindow());
		if (ctrlItem.IsWindow() == FALSE)
			return;
		ctrlItem.SetWindowTextW(GetTranslateMessage(ctrlID, args...).c_str());

		CFontHandle font = getFont();
		if (font)
			ctrlItem.SetFont(font);
	}

}	// namespace UITranslator













