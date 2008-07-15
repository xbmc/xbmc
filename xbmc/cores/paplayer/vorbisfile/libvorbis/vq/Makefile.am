## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I../lib -I$(top_srcdir)/include @OGG_CFLAGS@

EXTRA_PROGRAMS = latticebuild latticepare latticehint\
	latticetune huffbuild distribution
CLEANFILES = $(EXTRA_PROGRAMS)

LDFLAGS = -static
LDADD = ../lib/libvorbis.la

latticebuild_SOURCES = latticebuild.c vqgen.c bookutil.c\
	vqgen.h bookutil.h
latticepare_SOURCES = latticepare.c vqgen.c bookutil.c vqsplit.c\
	vqgen.h bookutil.h
latticehint_SOURCES = latticehint.c bookutil.c\
	bookutil.h
latticetune_SOURCES = latticetune.c vqgen.c bookutil.c\
	vqgen.h bookutil.h
huffbuild_SOURCES = huffbuild.c vqgen.c bookutil.c\
	vqgen.h bookutil.h
distribution_SOURCES = distribution.c bookutil.c\
	bookutil.h

EXTRA_DIST = residue_entropy auxpartition.pl

debugvq:
	$(MAKE) vq CFLAGS="@DEBUG@"

profilevq:
	$(MAKE) vq CFLAGS="@PROFILE@"

vq: 
	$(MAKE) $(EXTRA_PROGRAMS) 
