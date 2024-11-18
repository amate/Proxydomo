#pragma once

#include "Decompressor.h"
#include <sstream>
#include <memory>
#include <zstd.h>

class CZstdDecompressor : public IDecompressor
{
public:
	CZstdDecompressor();
	~CZstdDecompressor();

	// overrides
	virtual void feed(const std::string& data) override;
	virtual void dump() override;
	virtual std::string read() override;

private:
	ZSTD_DCtx* m_dctx;

	size_t	m_bufferOutSize;
	std::unique_ptr<BYTE[]>	m_bufferOut;

	std::stringstream	m_output;

};

