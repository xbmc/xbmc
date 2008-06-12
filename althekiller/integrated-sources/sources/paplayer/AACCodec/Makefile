CC=gcc
OBJS=alac.o demux.o stream.o
CFLAGS=-D_LINUX -fPIC -Ialac_decoder -Ilibfaad -Imp4ff -Ilibfaad/include

DIRS=alac_decoder libfaad mp4ff

AACCodec-osx.so: compile AACCodec.o utils.o
	$(CC) -shared -bundle -flat_namespace -undefined suppress -o $@ *.o alac_decoder/*.o libfaad/*.o mp4ff/*.o

AACCodec-i486-linux.so: compile AACCodec.o utils.o
	$(CC) -shared -o $@ *.o alac_decoder/*.o libfaad/*.o mp4ff/*.o `cat ../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.def` ../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.o

compile:
	$(MAKE) -C alac_decoder
	$(MAKE) -C libfaad
	$(MAKE) -C mp4ff

clean:
	for d in $(DIRS); do (cd $$d; $(MAKE) clean ); done
	$(RM) *.o *.so

%o : %c
	$(CC) -c -o $@ $< $(CFLAGS)

include Makefile.include
