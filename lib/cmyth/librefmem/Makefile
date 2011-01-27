INCLUDES=-I. -I../include -I../libcmyth

SRCS=alloc.c debug_refmem.c

LIB=librefmem.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

