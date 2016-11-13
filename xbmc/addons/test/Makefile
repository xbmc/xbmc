SRCS=	\
  TestAddonBuilder.cpp \
  TestAddonDatabase.cpp \
  TestAddonFactory.cpp \
  TestAddonVersion.cpp

LIB=addonsTest.a

INCLUDES += -I../../../lib/gtest/include

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
