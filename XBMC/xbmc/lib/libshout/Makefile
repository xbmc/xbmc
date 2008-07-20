OBJS=argv.o cbuf2.o charset.o debug.o external.o filelib.o findsep.o http.o iconvert.o ictest.o mchar.o mpeg.o parse.o relaylib.o ripaac.o rip_manager.o ripogg.o ripstream.o socklib.o threadlib.o utf8.o

include ../../../Makefile.include

CFLAGS += -D__UNIX__ -DXBMC 

lib: libshout-@ARCH@.a

libshout-@ARCH@.a : $(OBJS)
	ar rs $@  $(OBJS)
