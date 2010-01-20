INCLUDES=-I. -I../include

SRCS=alloc.c debug_refmem.c

LIB=librefmem.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

