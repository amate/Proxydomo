/**
*	@file	TextBuffer.h
*	@brief	データを受け取ってテキストフィルターで処理してRequestManagerに返すクラス
*/

#pragma once

#include <vector>
#include <memory>
#include "DataReceptor.h"
#include "proximodo\textfilter.h"

class CFilterOwner;
class CTextFilter;

class CTextBuffer : public IDataReceptor
{
public:
	CTextBuffer(CFilterOwner& owner, IDataReceptor* output);
	~CTextBuffer();

	// IDataReceptor
    void DataReset();
    void DataFeed(const std::string& data);
    void DataDump();

private:
	// Data members

    // the object which contains header values and variables
    CFilterOwner&	m_owner;

    // where to send processed data
    IDataReceptor*	m_output;

    // the filters
    std::vector<std::unique_ptr<CTextFilter>> vecpTextfilters;
    
    // the next filter to try (if a filter needs more data,
    // we'll start with it on next data arrival)
    std::vector<std::unique_ptr<CTextFilter>>::iterator	m_currentFilter;

    // pass string to output, escaping HTML chars as needed
    //void escapeOutput(stringstream& out, const char *data, size_t len);
    
    // the actual buffer
    std::string	m_buffer;
};

