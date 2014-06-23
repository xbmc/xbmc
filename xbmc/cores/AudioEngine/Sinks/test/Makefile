SRCS=TestAESinkDARWINOSX.cpp

#move this out of the if block if needed
LIB=AESinkTest.a

INCLUDES += -I../../../../../lib/gtest/include

include ../../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
