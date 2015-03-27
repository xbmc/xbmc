SRCS=	\
	TestBasicEnvironment.cpp \
	TestFileItem.cpp \
	TestTextureUtils.cpp \
	TestURL.cpp \
	TestUtil.cpp \
	TestUtils.cpp \
	xbmc-test.cpp

LIB=xbmc-test.a

INCLUDES += -I../../lib/gtest/include

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
