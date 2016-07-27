set(SOURCES TestAlarmClock.cpp
            TestAliasShortcutUtils.cpp
            TestArchive.cpp
            TestAsyncFileCopy.cpp
            TestBase64.cpp
            TestBitstreamStats.cpp
            TestCharsetConverter.cpp
            TestCPUInfo.cpp
            TestCrc32.cpp
            TestDatabaseUtils.cpp
            TestEndianSwap.cpp
            TestFileOperationJob.cpp
            TestFileUtils.cpp
            Testfstrcmp.cpp
            TestGlobalsHandling.cpp
            TestHTMLUtil.cpp
            TestHttpHeader.cpp
            TestHttpParser.cpp
            TestHttpRangeUtils.cpp
            TestHttpResponse.cpp
            TestJobManager.cpp
            TestJSONVariantParser.cpp
            TestJSONVariantWriter.cpp
            TestLabelFormatter.cpp
            TestLangCodeExpander.cpp
            TestLocale.cpp
            Testlog.cpp
            TestMathUtils.cpp
            Testmd5.cpp
            TestMime.cpp
            TestPerformanceSample.cpp
            TestPOUtils.cpp
            TestRegExp.cpp
            Testrfft.cpp
            TestRingBuffer.cpp
            TestScraperParser.cpp
            TestScraperUrl.cpp
            TestSortUtils.cpp
            TestStopwatch.cpp
            TestStreamDetails.cpp
            TestStreamUtils.cpp
            TestStringUtils.cpp
            TestSystemInfo.cpp
            TestURIUtils.cpp
            TestUrlOptions.cpp
            TestVariant.cpp
            TestXBMCTinyXML.cpp
            TestXMLUtils.cpp)

set(HEADERS TestGlobalsHandlingPattern1.h)

if(NOT CORE_SYSTEM_NAME STREQUAL windows)
  list(APPEND SOURCES TestCryptThreading.cpp)
endif()

core_add_test_library(utils_test)
