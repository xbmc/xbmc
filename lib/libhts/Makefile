INCLUDES=-I.

SRCS = htsmsg.c \
       htsmsg_binary.c \
       htsmsg_json.c \
       htsmsg_xml.c \
       htsbuf.c \
       htsstr.c \
       net_posix.c \
       sha1.c \

CFLAGS=-std=c99 -D_GNU_SOURCE
LIB=libhts.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
