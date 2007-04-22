# SMakefile for zlib
# Modified from the standard UNIX Makefile Copyright Jean-loup Gailly
# Osma Ahvenlampi <Osma.Ahvenlampi@hut.fi>
# Amiga, SAS/C 6.56 & Smake

CC=sc
CFLAGS=OPT
#CFLAGS=OPT CPU=68030
#CFLAGS=DEBUG=LINE
LDFLAGS=LIB z.lib

SCOPTIONS=OPTSCHED OPTINLINE OPTALIAS OPTTIME OPTINLOCAL STRMERGE \
       NOICONS PARMS=BOTH NOSTACKCHECK UTILLIB NOVERSION ERRORREXX 

OBJS = adler32.o compress.o crc32.o gzio.o uncompr.o deflate.o trees.o \
       zutil.o inflate.o infblock.o inftrees.o infcodes.o infutil.o inffast.o

TEST_OBJS = example.o minigzip.o

all: SCOPTIONS example minigzip

test: all
	`cd`/example
	echo hello world | minigzip | minigzip -d 

install: z.lib
	copy zlib.h zconf.h INCLUDE: clone
	copy z.lib LIB: clone

z.lib: $(OBJS)
	oml z.lib r $(OBJS)

example: example.o z.lib
	$(CC) $(CFLAGS) LINK TO $@ example.o $(LDFLAGS)

minigzip: minigzip.o z.lib
	$(CC) $(CFLAGS) LINK TO $@ minigzip.o $(LDFLAGS)

clean:
	-delete force quiet *.o example minigzip z.lib foo.gz *.lnk SCOPTIONS

SCOPTIONS: Smakefile
        copy to $@ <from <
$(SCOPTIONS)
<

# DO NOT DELETE THIS LINE -- make depend depends on it.

adler32.o: zutil.h zlib.h zconf.h
compress.o: zlib.h zconf.h
crc32.o: zutil.h zlib.h zconf.h
deflate.o: deflate.h zutil.h zlib.h zconf.h
example.o: zlib.h zconf.h
gzio.o: zutil.h zlib.h zconf.h
infblock.o: zutil.h zlib.h zconf.h infblock.h inftrees.h infcodes.h infutil.h
infcodes.o: zutil.h zlib.h zconf.h inftrees.h infutil.h infcodes.h inffast.h
inffast.o: zutil.h zlib.h zconf.h inftrees.h infutil.h inffast.h
inflate.o: zutil.h zlib.h zconf.h infblock.h
inftrees.o: zutil.h zlib.h zconf.h inftrees.h
infutil.o: zutil.h zlib.h zconf.h inftrees.h infutil.h
minigzip.o: zlib.h zconf.h
trees.o: deflate.h zutil.h zlib.h zconf.h
uncompr.o: zlib.h zconf.h
zutil.o: zutil.h zlib.h zconf.h
