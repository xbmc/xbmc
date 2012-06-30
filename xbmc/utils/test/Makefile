SRCS=	\
	TestAlarmClock.cpp \
	TestAliasShortcutUtils.cpp \
	TestArchive.cpp \
	TestAsyncFileCopy.cpp \
	TestBase64.cpp \
	TestBitstreamStats.cpp \
	TestCharsetConverter.cpp \
	TestGlobalsHandling.cpp \
	TestXBMCTinyXML.cpp

LIB=utilsTest.a

INCLUDES += -I../../../lib/gtest/include

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
