
#include "stdafx.h"
#include "brotliDecompressor.h"
#include <memory>
#include <assert.h>
#include "Logger.h"

CBrotliDecompressor::CBrotliDecompressor() : m_brotliState(nullptr)
{
	m_brotliState = ::BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
	assert(m_brotliState);
}

CBrotliDecompressor::~CBrotliDecompressor()
{
	::BrotliDecoderDestroyInstance(m_brotliState);
}

void CBrotliDecompressor::feed(const std::string& data)
{
	auto outBuffer = std::make_unique<uint8_t[]>(kBufferSize);
	BrotliDecoderResult result = BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT;

	m_buffer += data;
	size_t remaining = m_buffer.size();
	const uint8_t* next_in = reinterpret_cast<const uint8_t*>(m_buffer.data());//encoded_buffer;

	while (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
		size_t available_in = remaining;//encoded_size;

		size_t available_out = kBufferSize;	//*decoded_size;
		uint8_t* next_out = outBuffer.get();		//decoded_buffer;
		size_t total_out = 0;

		result = ::BrotliDecoderDecompressStream(
			m_brotliState, &available_in, &next_in, &available_out, &next_out, &total_out);

		size_t outSize = kBufferSize - available_out;
		m_output.write(reinterpret_cast<const char*>(outBuffer.get()), outSize);
		remaining = available_in;

		assert(result != BROTLI_DECODER_RESULT_ERROR);
		if (result == BROTLI_DECODER_RESULT_ERROR) {
			throw std::runtime_error("CBrotliDecompressor::feed : BROTLI_DECODER_RESULT_ERROR");
		}
	}
	// —]‚è‚ð•Û‘¶
	m_buffer = m_buffer.substr(m_buffer.size() - remaining);
}

void CBrotliDecompressor::dump()
{
	
	bool bSuccess = ::BrotliDecoderIsFinished(m_brotliState) == BROTLI_TRUE;
	if (bSuccess == false) {
		ERROR_LOG << L"CBrotliDecompressor::dump : BrotliDecoderIsFinished false";
		assert(false);
	}
	else if (m_buffer.size() > 0) {
		ERROR_LOG << L"CBrotliDecompressor::dump : m_buffer.size() > 0";
		assert(false);
	}
}

std::string CBrotliDecompressor::read()
{
	std::string out = m_output.str();
	m_output.str("");
	return out;
}

