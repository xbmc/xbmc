INCLUDES=-I.

SRCSC=bookmark.c commbreak.c connection.c event.c file.c freespace.c keyframe.c livetv.c mysql_query.c mythtv_mysql.c posmap.c proginfo.c proglist.c rec_num.c recorder.c ringbuf.c socket.c timestamp.c debug.c alloc.c debug_refmem.c

LIB=libcmyth.a

include ../../../Makefile.include
-include Makefile.depend

OBJSC+=$(SRCSC:.c=.o)

lib: $(OBJSC)

clean: 
	$(RM) $(OBJSC) $(LIB) $(PCH:.h=.h.gch)
	for d in $(DIRS); do (cd $$d; $(MAKE) clean ); done

depend: $(PCH) $(SRCSC)
	-$(CC) -MM $(DEFINES) $(INCLUDES) $(SRCSC) > Makefile.depend
	-for d in $(DIRS); do (cd $$d; $(MAKE) depend ); done
