/**
*	@file	TextBuffer.cpp
*	@brief	データを受け取ってテキストフィルターで処理してRequestManagerに返すクラス
*/

#include "stdafx.h"
#include "TextBuffer.h"
#include "FilterOwner.h"

CTextBuffer::CTextBuffer(CFilterOwner& owner, IDataReceptor* output) : m_owner(owner), m_output(output)
{
}


CTextBuffer::~CTextBuffer(void)
{
}


// IDataReceptor

void CTextBuffer::DataReset()
{
}

void CTextBuffer::DataFeed(const std::string& data)
{
}

void CTextBuffer::DataDump()
{
}