INCLUDES=-I. -I../../../ -I../../../linux -I../../../../guilib

SRCS=libc.c

LIB=overrides.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
