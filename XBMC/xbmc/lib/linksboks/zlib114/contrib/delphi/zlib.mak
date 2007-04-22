# Makefile for zlib32bd.lib
# ------------- Borland C++ 4.5 -------------

# The (32-bit) zlib32bd.lib made with this makefile is intended for use 
# in making the (32-bit) DLL, png32bd.dll. It uses the "stdcall" calling 
# convention.

CFLAGS= -ps -O2 -C -K -N- -k- -d -3 -r- -w-par -w-aus -WDE
CC=f:\bc45\bin\bcc32
LIBFLAGS= /C
LIB=f:\bc45\bin\tlib
ZLIB=zlib32bd.lib

.autodepend
.c.obj:
        $(CC) -c $(CFLAGS) $<

OBJ1=adler32.obj compress.obj crc32.obj deflate.obj gzio.obj infblock.obj 
OBJ2=infcodes.obj inflate.obj inftrees.obj infutil.obj inffast.obj 
OBJ3=trees.obj uncompr.obj zutil.obj
pOBJ1=+adler32.obj+compress.obj+crc32.obj+deflate.obj+gzio.obj+infblock.obj 
pOBJ2=+infcodes.obj+inflate.obj+inftrees.obj+infutil.obj+inffast.obj 
pOBJ3=+trees.obj+uncompr.obj+zutil.obj

all: $(ZLIB)

$(ZLIB): $(OBJ1) $(OBJ2) $(OBJ3)
        @if exist $@ del $@
        $(LIB) @&&|
$@ $(LIBFLAGS) &
$(pOBJ1) &
$(pOBJ2) &
$(pOBJ3)
|

# End of makefile for zlib32bd.lib
