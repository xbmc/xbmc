CC=g++
CFLAGS=-fPIC -DXBMC -D_DLL -D_LINUX
OBJS=ximage.o ximabmp.o ximadsp.o ximaenc.o ximaexif.o ximagif.o ximaico.o ximaj2k.o ximajas.o ximajbg.o ximajpg.o ximalpha.o ximalyr.o ximamng.o ximapal.o ximapcx.o ximapng.o ximasel.o ximatga.o ximatif.o ximatran.o ximawbmp.o ximawmf.o ximawnd.o xmemfile.o DllInterface.o

%o : %cpp
	$(CC) $(CFLAGS) -c -o $@ $< 

cximage.a: $(OBJS)

clean:
	$(RM) $(OBJS)

include ../Makefile.include
