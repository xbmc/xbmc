INCLUDES=-I. -I../../ -I../../linux -I../../../guilib -I../../utils
SRCS=scrobbler.cpp
OBJS=scrobbler.o

LIB=scrobbler.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
