INCLUDES+= -I../utils

SRCS= \
	SemaphorePOSIX.cpp

LIB=posix.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))


