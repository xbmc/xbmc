CFLAGS=-D_LINUX -fPIC -O2
OBJS=timiditydll.o common.o controls.o filter.o instrum.o mix.o output.o playmidi.o readmidi.o resample.o sdl_a.o sdl_c.o tables.o timidity.o

timidity-i486-linux.so: $(OBJS)
	g++ -shared -o $@ *.o `cat ../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.def` ../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.o

clean:
	$(RM) *.o *.so

%o : %c
	gcc $(CFLAGS) -c $< -o $@

%o : %cpp
	g++ $(CFLAGS) -c $< -o $@
