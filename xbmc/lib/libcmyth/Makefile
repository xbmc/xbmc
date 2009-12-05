INCLUDES=-I. -I../librefmem

SRCS=bookmark.c commbreak.c connection.c event.c file.c freespace.c keyframe.c livetv.c mysql_query.c mythtv_mysql.c posmap.c proginfo.c proglist.c rec_num.c recorder.c ringbuf.c socket.c timestamp.c debug.c utf8tolatin1.c
SYSDIR=../../../system
SLIBS=$(SYSDIR)/librefmem.so
LIB=libcmyth.a
LD_FLAGS=-Wl,-L$(SYSDIR) -lrefmem
.PHONY: compile

$(LIB): $(SRCS)

$(SLIBS): compile

$(SLIBS):
	$(MAKE) ../librefmem

-include ../../../Makefile.include
