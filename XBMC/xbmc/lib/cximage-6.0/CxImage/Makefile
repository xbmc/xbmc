
ifeq ($(ARCH), x86-osx)
	CFLAGS=-fPIC -DXBMC -D_DLL -DNO_LCMS -DNO_JPEG -I../jasper/include -I../jpeg/
else
	CFLAGS=-fPIC -DXBMC -D_DLL -D_LINUX -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
endif

OBJS= \
	DllInterface.o ximadsp.o ximage.o ximaico.o ximajas.o \
	ximalpha.o ximapal.o ximaraw.o ximatga.o ximatran.o \
	ximawnd.o tif_xfile.o ximaenc.o ximagif.o ximainfo.o \
	ximajbg.o ximalyr.o ximapcx.o ximasel.o ximath.o ximawbmp.o \
	xmemfile.o ximabmp.o ximaexif.o ximahist.o ximaint.o \
	ximajpg.o ximamng.o ximapng.o ximaska.o ximatif.o ximawmf.o

%o : %cpp
	$(CXX) $(CFLAGS) -c -o $@ $< 

cximage.a: $(OBJS)
	$(AR) rc $@ $(OBJS)

clean:
	$(RM) -f $(OBJS) cximage.a

distclean: clean

-include ../Makefile.include
