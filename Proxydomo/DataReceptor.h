/**
*	@file	DataReceptor.h
*
*/

#pragma once

#include <string>

/* class IDataReceptor
 * This virtual class is for classes that can receive data.
 * It allows chaining text processors. The last CDataReceptor of the
 * chain is not a text processor but the request manager itself.
 */
class IDataReceptor {

public:
    virtual ~IDataReceptor() { }

    virtual void DataReset() = 0;
    virtual void DataFeed(const std::string& data) = 0;
    virtual void DataDump() = 0;
};
