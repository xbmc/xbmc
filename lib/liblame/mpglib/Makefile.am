## $Id: Makefile.am,v 1.15.8.1 2010/03/22 14:17:14 robert Exp $

include $(top_srcdir)/Makefile.am.global

INCLUDES = @INCLUDES@ -I$(top_srcdir)/libmp3lame -I$(top_builddir)

EXTRA_DIST = \
	AUTHORS \
	README \
	mpglib_vc6.dsp \
	mpglib_vc8.vcproj

DEFS = @DEFS@ @CONFIG_DEFS@

noinst_LTLIBRARIES = libmpgdecoder.la

libmpgdecoder_la_SOURCES = common.c \
	dct64_i386.c \
	decode_i386.c \
	interface.c \
	layer1.c \
	layer2.c \
	layer3.c \
	tabinit.c

noinst_HEADERS = common.h \
	dct64_i386.h \
	decode_i386.h \
	huffman.h \
	interface.h \
	l2tables.h \
	layer1.h \
	layer2.h \
	layer3.h \
	mpg123.h \
	mpglib.h \
	tabinit.h

LCLINTFLAGS= \
	+posixlib \
	+showsummary \
	+showalluses \
	+whichlib \
	+forcehints \
	-fixedformalarray \
	+matchanyintegral \
	-Dlint

lclint.txt: ${libmpgdecoder_la_SOURCES} ${noinst_HEADERS}
	@lclint ${LCLINTFLAGS} ${INCLUDES} ${DEFS} ${libmpgdecoder_la_SOURCES} 2>&1 >lclint.txt || true

lclint: lclint.txt
	more lclint.txt

#$(OBJECTS): libtool
#libtool: $(LIBTOOL_DEPS)
#	$(SHELL) $(top_builddir)/config.status --recheck
