LIB=cximage.a
DEFINES+=-D_DLL -DXBMC
ifeq ($(findstring osx,$(ARCH)), osx)
	INCLUDES+=-I../jasper/include -I../jpeg/
endif
OBJS= \
	DllInterface.o ximadsp.o ximage.o ximaico.o ximajas.o \
	ximalpha.o ximapal.o ximaraw.o ximatga.o ximatran.o \
	ximawnd.o tif_xfile.o ximaenc.o ximagif.o ximainfo.o \
	ximajbg.o ximalyr.o ximapcx.o ximasel.o ximath.o ximawbmp.o \
	xmemfile.o ximabmp.o ximaexif.o ximahist.o ximaint.o \
	ximajpg.o ximamng.o ximapng.o ximaska.o ximatif.o ximawmf.o

include ../../../../Makefile.include

