SRCS=	\
	TestBasicEnvironment.cpp \
	TestFileItem.cpp \
	TestTextureCache.cpp \
	TestUtils.cpp \
	xbmc-test.cpp

LIB=xbmc-test.a

INCLUDES += -I../../lib/gtest/include

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
