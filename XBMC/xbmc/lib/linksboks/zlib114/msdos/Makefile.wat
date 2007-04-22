# Makefile for zlib
# Watcom 10a

# This version of the zlib makefile was adapted by Chris Young for use
# with Watcom 10a 32-bit protected mode flat memory model.  It was created 
# for use with POV-Ray ray tracer and you may choose to edit the CFLAGS to 
# suit your needs but the -DMSDOS is required.
# -- Chris Young 76702.1655@compuserve.com

# To use, do "wmake -f makefile.wat"

# See zconf.h for details about the memory requirements.

# ------------- Watcom 10a -------------
MODEL=-mf 
CFLAGS= $(MODEL) -fpi87 -fp5 -zp4 -5r -w5 -oneatx -DMSDOS
CC=wcc386
LD=wcl386
LIB=wlib -b -c 
LDFLAGS= 
O=.obj

# variables
OBJ1=adler32$(O) compress$(O) crc32$(O) gzio$(O) uncompr$(O) deflate$(O) 
OBJ2=trees$(O) zutil$(O) inflate$(O) infblock$(O) inftrees$(O) infcodes$(O) 
OBJ3=infutil$(O) inffast$(O)
OBJP1=adler32$(O)+compress$(O)+crc32$(O)+gzio$(O)+uncompr$(O)+deflate$(O)
OBJP2=trees$(O)+zutil$(O)+inflate$(O)+infblock$(O)+inftrees$(O)+infcodes$(O)
OBJP3=infutil$(O)+inffast$(O)

all: test

adler32.obj: adler32.c zlib.h zconf.h
	$(CC) $(CFLAGS) $*.c

compress.obj: compress.c zlib.h zconf.h
	$(CC) $(CFLAGS) $*.c

crc32.obj: crc32.c zlib.h zconf.h
	$(CC) $(CFLAGS) $*.c

deflate.obj: deflate.c deflate.h zutil.h zlib.h zconf.h
	$(CC) $(CFLAGS) $*.c

gzio.obj: gzio.c zutil.h zlib.h zconf.h
	$(CC) $(CFLAGS) $*.c

infblock.obj: infblock.c zutil.h zlib.h zconf.h infblock.h inftrees.h &
  infcodes.h infutil.h
	$(CC) $(CFLAGS) $*.c

infcodes.obj: infcodes.c zutil.h zlib.h zconf.h inftrees.h infutil.h &
  infcodes.h inffast.h
	$(CC) $(CFLAGS) $*.c

inflate.obj: inflate.c zutil.h zlib.h zconf.h infblock.h
	$(CC) $(CFLAGS) $*.c

inftrees.obj: inftrees.c zutil.h zlib.h zconf.h inftrees.h
	$(CC) $(CFLAGS) $*.c

infutil.obj: infutil.c zutil.h zlib.h zconf.h inftrees.h infutil.h
	$(CC) $(CFLAGS) $*.c

inffast.obj: inffast.c zutil.h zlib.h zconf.h inftrees.h infutil.h inffast.h
	$(CC) $(CFLAGS) $*.c

trees.obj: trees.c deflate.h zutil.h zlib.h zconf.h
	$(CC) $(CFLAGS) $*.c

uncompr.obj: uncompr.c zlib.h zconf.h
	$(CC) $(CFLAGS) $*.c

zutil.obj: zutil.c zutil.h zlib.h zconf.h
	$(CC) $(CFLAGS) $*.c

example.obj: example.c zlib.h zconf.h
	$(CC) $(CFLAGS) $*.c

minigzip.obj: minigzip.c zlib.h zconf.h
	$(CC) $(CFLAGS) $*.c

# we must cut the command line to fit in the MS/DOS 128 byte limit:
zlib.lib: $(OBJ1) $(OBJ2) $(OBJ3) 
	del zlib.lib
	$(LIB) zlib.lib +$(OBJP1)
	$(LIB) zlib.lib +$(OBJP2)
	$(LIB) zlib.lib +$(OBJP3)

example.exe: example.obj zlib.lib
	$(LD) $(LDFLAGS) example.obj zlib.lib

minigzip.exe: minigzip.obj zlib.lib
	$(LD) $(LDFLAGS) minigzip.obj zlib.lib

test: minigzip.exe example.exe
	example
	echo hello world | minigzip | minigzip -d >test
	type test

#clean:
#	del *.obj
#	del *.exe
