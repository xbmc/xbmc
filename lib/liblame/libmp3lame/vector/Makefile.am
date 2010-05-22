## $Id: Makefile.am,v 1.1 2007/01/09 10:15:53 aleidinger Exp $

include $(top_srcdir)/Makefile.am.global

if WITH_XMM
noinst_LTLIBRARIES = liblamevectorroutines.la
endif

##liblamecpuroutines_la_LIBADD = 
##liblamecpuroutines_la_LDFLAGS =

INCLUDES = @INCLUDES@ \
	-I$(top_srcdir)/libmp3lame \
	-I$(top_srcdir)/mpglib \
	-I$(top_builddir)

DEFS = @DEFS@ @CONFIG_DEFS@

xmm_sources = xmm_quantize_sub.c

if WITH_XMM
liblamevectorroutines_la_SOURCES = $(xmm_sources)
endif

noinst_HEADERS = lame_intrin.h

EXTRA_liblamevectorroutines_la_SOURCES = $(xmm_sources)

CLEANFILES = lclint.txt

LCLINTFLAGS= \
	+posixlib \
	+showsummary \
	+showalluses \
	+whichlib \
	+forcehints \
	-fixedformalarray \
	+matchanyintegral \
	-Dlint

lclint.txt: ${liblamecpuroutines_la_SOURCES} ${noinst_HEADERS}
	@lclint ${LCLINTFLAGS} ${INCLUDES} ${DEFS} ${liblamecpuroutines_la_SOURCES} 2>&1 >lclint.txt || true

lclint: lclint.txt
	more lclint.txt

#$(OBJECTS): libtool
#libtool: $(LIBTOOL_DEPS)
#	$(SHELL) $(top_builddir)/config.status --recheck
