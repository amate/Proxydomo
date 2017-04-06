#pragma once

#include "stdafx.h"
#include <string>

/*
	Content-Encoding gzip deflate br 解凍用のインターフェース
	usage: feed -> read or dump -> read
*/

class IDecompressor
{
public:
	virtual ~IDecompressor() {};

	virtual void feed(const std::string& data) = 0;
	virtual void dump() = 0;
	virtual std::string read() = 0;

};
