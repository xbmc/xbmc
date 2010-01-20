# @(#) $Id: Makefile.am,v 1.10 2003/11/17 12:27:40 yeti Exp $
noinst_PROGRAMS = make_hash
noinst_HEADERS = encodings.h
noinst_SCRIPTS = expand_table.pl

make_hash_SOURCES = make_hash.c

BUILT_SOURCES = $(noinst_HEADERS)

TABLE_SRC = \
  ACCEPTED_CHARS.t \
  BASE64.ti \
  BOXVERT_CP1125.t \
  BOXVERT_IBM852.t \
  BOXVERT_KEYBCS2.t \
  BOXVERT_KOI8R.t \
  BOXVERT_KOI8RU.t \
  BOXVERT_KOI8U.t \
  BOXVERT_KOI8UNI.t \
  HEXDIGITS.ti \
  TEX_ACCALPHA.t \
  TEX_ACCPUNCT.t \
  TEX_SKIP.t \
  encodings.dat

EXTRA_DIST = $(TABLE_SRC) $(noinst_SCRIPTS) iconvenc.null

all: encodings.h

encodings.sed: $(top_builddir)/iconvenc.h
	sed -e 's/^#define \([A-Z0-9_]*\) \(.*\)/@\1@ \2/' -e 's/"//g' -e 's/NULL$$//' -e 's/ /\//' -e 's/^\(.*\)$$/s\/\1\//' $(top_builddir)/iconvenc.h >encodings.sed

encodings.h: encodings.sed $(srcdir)/encodings.dat make_hash
	sed -f encodings.sed $(srcdir)/encodings.dat | ./make_hash >encodings.h

# Normally there's no need to regenerate tables, they are copy-and-pasted
# into the C source manually, but the rules are here.  Run `make tables'
# to create them.
TABLES = \
  ACCEPTED_CHARS.h \
  BOXVERT_CP1125.h \
  BOXVERT_IBM852.h \
  BOXVERT_KEYBCS2.h \
  BOXVERT_KOI8R.h \
  BOXVERT_KOI8RU.h \
  BOXVERT_KOI8U.h \
  BOXVERT_KOI8UNI.h \
  TEX_ACCALPHA.h \
  TEX_ACCPUNCT.h \
  TEX_SKIP.h
TABLES_INCR = BASE64.h HEXDIGITS.h

if MAINTAINER_MODE
tables: $(TABLES) $(TABLES_INCR)

%.h: $(srcdir)/%.t $(srcdir)/expand_table.pl
	$(srcdir)/expand_table.pl $< >$@

%.h: $(srcdir)/%.ti $(srcdir)/expand_table.pl
	$(srcdir)/expand_table.pl $< >$@
endif

clean-local:
	rm -f $(TABLES) $(TABLES_INCR) encodings.sed *~ core.*

distclean-local:
	rm -f encodings.h

