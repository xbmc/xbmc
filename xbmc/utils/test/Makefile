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
	TestGlobalsHandling.cpp \
	TestXBMCTinyXML.cpp

LIB=utilsTest.a

INCLUDES += -I../../../lib/gtest/include

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
