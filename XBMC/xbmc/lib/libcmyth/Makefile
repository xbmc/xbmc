INCLUDES+=-I. -I../librefmem

SRCS=bookmark.c commbreak.c connection.c event.c file.c freespace.c keyframe.c livetv.c mysql_query.c mythtv_mysql.c posmap.c proginfo.c proglist.c rec_num.c recorder.c ringbuf.c socket.c timestamp.c debug.c utf8tolatin1.c

REFMEMLIB=../librefmem/librefmem.a

LIB=libcmyth.a
LDFLAGS += $(REFMEMLIB)

$(LIB): $(REFMEMLIB)

$(REFMEMLIB):
	$(MAKE) -C ../librefmem

include ../../../Makefile.include
