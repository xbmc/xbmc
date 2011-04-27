INCLUDES=-I.

SRCS = linux/net_posix.c \

CFLAGS=-std=c99 -D_GNU_SOURCE
LIB=libTcpSocket.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
