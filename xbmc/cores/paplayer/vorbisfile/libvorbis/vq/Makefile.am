## Process this file with automake to produce Makefile.in

INCLUDES = -I../lib -I$(top_srcdir)/include @OGG_CFLAGS@

EXTRA_PROGRAMS = latticebuild latticepare latticehint\
	latticetune huffbuild distribution
CLEANFILES = $(EXTRA_PROGRAMS)

AM_LDFLAGS = -static
LDADD = ../lib/libvorbis.la

latticebuild_SOURCES = latticebuild.c vqgen.c bookutil.c\
	vqgen.h bookutil.h localcodebook.h
latticepare_SOURCES = latticepare.c vqgen.c bookutil.c vqsplit.c\
	vqgen.h vqsplit.h bookutil.h localcodebook.h
latticehint_SOURCES = latticehint.c bookutil.c\
	vqsplit.h bookutil.h localcodebook.h
latticetune_SOURCES = latticetune.c vqgen.c bookutil.c\
	vqgen.h bookutil.h localcodebook.h
huffbuild_SOURCES = huffbuild.c vqgen.c bookutil.c\
	vqgen.h bookutil.h localcodebook.h
distribution_SOURCES = distribution.c bookutil.c\
	bookutil.h localcodebook.h

EXTRA_DIST = residue_entropy auxpartition.pl

debugvq:
	$(MAKE) vq CFLAGS="@DEBUG@"

profilevq:
	$(MAKE) vq CFLAGS="@PROFILE@"

vq: 
	$(MAKE) $(EXTRA_PROGRAMS) 
