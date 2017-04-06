
#include "stdafx.h"

#ifdef UNIT_TEST
#include <gtest\gtest.h>
//#ifdef _WIN64
//#pragma comment(lib, "gtest.lib")
//#else
#pragma comment(lib, "gtestd.lib")
//#endif

#include "BrotliDecompressor.h"
#include <filesystem>
#include "Misc.h"
#include "proximodo\util.h"

using path = std::tr2::sys::path;

///////////////////////////////////////////////////////////////////////////////////
// BrotliDecompressorTest

TEST(BrotliDecompressor, test)
{
	CBrotliDecompressor decompressor;

	path testDataFolder = path(std::wstring(Misc::GetExeDirectory())).parent_path().parent_path() / "testdata/brotli";
	auto rawcompressedData = CUtil::LoadBinaryFile(testDataFolder / "alice29.txt.compressed");
	auto rawdecomressedData = CUtil::LoadBinaryFile(testDataFolder / "alice29.txt");

	std::string strdata(reinterpret_cast<const char*>(rawcompressedData.data()), rawcompressedData.size());

	decompressor.feed(strdata);
	std::string decompressedData = decompressor.read();

	EXPECT_TRUE(decompressedData.size() == rawdecomressedData.size());
	EXPECT_TRUE(memcmp(decompressedData.data(), rawdecomressedData.data(), rawdecomressedData.size()) == 0);

}

TEST(BrotliDecompressor, split_test)
{
	CBrotliDecompressor decompressor;

	path testDataFolder = path(std::wstring(Misc::GetExeDirectory())).parent_path().parent_path() / "testdata/brotli";
	auto rawcompressedData = CUtil::LoadBinaryFile(testDataFolder / "alice29.txt.compressed");
	auto rawdecomressedData = CUtil::LoadBinaryFile(testDataFolder / "alice29.txt");

	std::string strdata1(reinterpret_cast<const char*>(rawcompressedData.data()), 128);
	std::string strdata2(reinterpret_cast<const char*>(rawcompressedData.data() + 128), rawcompressedData.size() - 128);

	decompressor.feed(strdata1);
	std::string decompressedData = decompressor.read();

	decompressor.feed(strdata2);
	decompressedData += decompressor.read();

	decompressor.dump();

	EXPECT_TRUE(decompressedData.size() == rawdecomressedData.size());
	EXPECT_TRUE(memcmp(decompressedData.data(), rawdecomressedData.data(), rawdecomressedData.size()) == 0);

}


#endif