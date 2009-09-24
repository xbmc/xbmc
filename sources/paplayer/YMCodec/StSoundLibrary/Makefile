CFLAGS=-D_LINUX -fPIC
OBJS=Depacker.o digidrum.o XBMCYM.o Ym2149Ex.o Ymload.o YmMusic.o YmUserInterface.o LZH/LZHXLIB.o

stsoundlibrary-i486-linux.so: $(OBJS)
	gcc -fPIC -shared -o $@ $(OBJS) `cat ../../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.def` ../../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.o

clean:
	$(RM) $(OBJS) *.so

%o : %c
	gcc $(CFLAGS) -O2 -fPIC -c $< -o $@

%o : %C
	gcc $(CFLAGS) -O2 -fPIC -c $< -o $@

%o : %cpp
	g++ $(CFLAGS) -O2 -fPIC -c $< -o $@
