#pragma once

#include "Decompressor.h"
#include <brotli\decode.h>
#include <sstream>


class CBrotliDecompressor : public IDecompressor
{
	enum { kBufferSize = 4096 };

public:
	CBrotliDecompressor();
	~CBrotliDecompressor();

	// overrides

	virtual void feed(const std::string& data) override;
	virtual void dump() override;
	virtual std::string read() override;

private:
	BrotliDecoderState* m_brotliState;
	std::string			m_buffer;
	std::stringstream	m_output;
};