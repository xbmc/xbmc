SRCS=	\
	TestSwig.cpp

LIB=pythonSwigTest.a

INCLUDES += -I../../../../lib/gtest/include
INCLUDES += -I/usr/include/python2.7

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
