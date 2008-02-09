OBJS=argv.o cbuf2.o charset.o debug.o external.o filelib.o findsep.o http.o iconvert.o ictest.o mchar.o mpeg.o parse.o relaylib.o ripaac.o rip_manager.o ripogg.o ripstream.o socklib.o threadlib.o utf8.o

%.o : %.c
	gcc -DHAVE_CONFIG_H -I. -Wall -D__UNIX__ -DXBMC -c $< 

libshout-i486-linux.a : $(OBJS)
	ar rs $@  $(OBJS)

clean:
	$(RM) $(OBJS) librtv-i486-linux.a

