SRCS=	\
	TestAlarmClock.cpp \
	TestAliasShortcutUtils.cpp \
	TestArchive.cpp \
	TestAsyncFileCopy.cpp \
	TestBase64.cpp \
	TestBitstreamStats.cpp \
	TestCharsetConverter.cpp \
	TestCPUInfo.cpp \
	TestCrc32.cpp \
	TestCryptThreading.cpp \
	TestDatabaseUtils.cpp \
	TestDownloadQueue.cpp \
	TestDownloadQueueManager.cpp \
	TestEndianSwap.cpp \
	Testfastmemcpy.cpp \
	Testfft.cpp \
	TestFileOperationJob.cpp \
	TestFileUtils.cpp \
	Testfstrcmp.cpp \
	TestGlobalsHandling.cpp \
	TestHTMLTable.cpp \
	TestHTMLUtil.cpp \
	TestHttpHeader.cpp \
	TestHttpParser.cpp \
	TestHttpResponse.cpp \
	TestJobManager.cpp \
	TestJSONVariantParser.cpp \
	TestJSONVariantWriter.cpp \
	TestLabelFormatter.cpp \
	TestLangCodeExpander.cpp \
	Testlog.cpp \
	TestMathUtils.cpp \
	Testmd5.cpp \
	TestMime.cpp \
	TestPerformanceSample.cpp \
	TestPOUtils.cpp \
	TestRegExp.cpp \
	TestRingBuffer.cpp \
	TestScraperParser.cpp \
	TestScraperUrl.cpp \
	TestSortUtils.cpp \
	TestStdString.cpp \
	TestStopwatch.cpp \
	TestStreamDetails.cpp \
	TestStreamUtils.cpp \
	TestStringUtils.cpp \
	TestSystemInfo.cpp \
	TestTimeSmoother.cpp \
	TestTimeUtils.cpp \
	TestURIUtils.cpp \
	TestVariant.cpp \
	TestXBMCTinyXML.cpp \
	TestXMLUtils.cpp

LIB=utilsTest.a

INCLUDES += -I../../../lib/gtest/include

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
