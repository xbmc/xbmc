INCLUDES=-I. -I../include

SRCS=bookmark.c commbreak.c connection.c event.c file.c freespace.c keyframe.c livetv.c mysql_query.c mythtv_mysql.c posmap.c proginfo.c proglist.c rec_num.c recorder.c ringbuf.c socket.c timestamp.c debug.c utf8tolatin1.c

LIB=libcmyth.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

