#include "stdafx.h"
#include "ZstdDecompressor.h"
#include <assert.h>
#include "Logger.h"
#include "CodeConvert.h"

#pragma comment(lib, "zstd.lib")

CZstdDecompressor::CZstdDecompressor()
{
	m_dctx = ::ZSTD_createDCtx();
	assert(m_dctx);

	m_bufferOutSize = ZSTD_DStreamOutSize();
	assert(m_bufferOutSize);
	m_bufferOut.reset(new BYTE[m_bufferOutSize]);
}

CZstdDecompressor::~CZstdDecompressor()
{
	::ZSTD_freeDCtx(m_dctx);
}

void CZstdDecompressor::feed(const std::string& data)
{
	// sample: https://github.com/facebook/zstd/blob/dev/examples/streaming_decompression.c
	ZSTD_inBuffer input = { static_cast<const void*>(data.data()), data.length(), 0 };
	while (input.pos < input.size) {
		ZSTD_outBuffer output = { static_cast<void*>(m_bufferOut.get()), m_bufferOutSize, 0 };

		size_t const ret = ::ZSTD_decompressStream(m_dctx, &output, &input);
		if (ZSTD_isError(ret)) {
			const char* errorMessage = ZSTD_getErrorName(ret);
			WARN_LOG << L"CZstdDecompressor::feed - ZSTD_decompressStream error: " << CodeConvert::UTF16fromUTF8(errorMessage);
			assert(false);
			return;
		}
		m_output.write(static_cast<const char*>(output.dst), output.pos);
	}
}

void CZstdDecompressor::dump()
{
}

std::string CZstdDecompressor::read()
{
	std::string out = m_output.str();
	m_output.str("");
	return out;
}
