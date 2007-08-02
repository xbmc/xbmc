CFLAGS=-D_LINUX -fPIC -I../../../XBMC/xbmc/linux -I../../../XBMC/xbmc
OBJS=gym_play.cpp psg.o XBMCGYM.o ym2612.o

gensapu-i486-linux.so: $(OBJS)
	g++ -shared -o $@ *.o `cat ../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.def` ../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.o

clean:
	$(RM) *.o *.so

%o : %c
	gcc -O2 $(CFLAGS) -c $< -o $@

%o : %cpp
	g++ -O2 $(CFLAGS) -c $< -o $@
