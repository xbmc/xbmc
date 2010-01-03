# @(#) $Id: Makefile.am,v 1.22 2005/12/01 10:13:50 yeti Exp $
noinst_PROGRAMS = \
  basetoc \
  countall \
  countpair \
  findletters \
  mystrings \
  pairtoc \
  xlt
noinst_DATA = $(BASES) $(RAWCOUNTS) $(PAIRCOUNTS) $(MAPS) Letters
noinst_HEADERS = \
  belarussian/belarussian.h \
  bulgarian/bulgarian.h \
  chinese/chinese.h \
  chinese/zh_weight_big5.h \
  chinese/zh_weight_gbk.h \
  croatian/croatian.h \
  czech/czech.h \
  estonian/estonian.h \
  hungarian/hungarian.h \
  latvian/latvian.h \
  lithuanian/lithuanian.h \
  polish/polish.h \
  russian/russian.h \
  slovak/slovak.h \
  slovene/slovene.h \
  ukrainian/ukrainian.h

basetoc_SOURCES = basetoc.c
countall_SOURCES = countall.c
countpair_SOURCES = countpair.c
findletters_SOURCES = findletters.c
mystrings_SOURCES = mystrings.c
pairtoc_SOURCES = pairtoc.c
xlt_SOURCES = xlt.c

noinst_SCRPITS = \
  clean.sh \
  doit.sh \
  extreme.pl \
  makepaircounts.sh \
  map2letters.sh \
  normalize.pl \
  totals.pl \
  belarussian/doit.sh \
  bulgarian/doit.sh \
  croatian/doit.sh \
  czech/doit.sh \
  estonian/doit.sh \
  hungarian/doit.sh \
  latvian/doit.sh \
  lithuanian/doit.sh \
  polish/doit.sh \
  russian/doit.sh \
  slovak/doit.sh \
  slovene/doit.sh \
  ukrainian/doit.sh

BASES = \
  belarussian/cp1251.base \
  bulgarian/cp1251.base \
  croatian/cp1250.base \
  czech/iso88592.base \
  estonian/iso88594.base \
  hungarian/iso88592.base \
  russian/koi8r.base \
  latvian/cp1257.base \
  lithuanian/cp1257.base \
  polish/iso88592.base \
  slovak/cp1250.base \
  slovene/iso88592.base \
  ukrainian/cp1251.base

RAWCOUNTS = \
  belarussian/rawcounts.cp1251 \
  bulgarian/rawcounts.cp1251 \
  croatian/rawcounts.cp1250 \
  czech/rawcounts.iso88592 \
  estonian/rawcounts.iso88594 \
  hungarian/rawcounts.iso88592 \
  latvian/rawcounts.cp1257 \
  lithuanian/rawcounts.cp1257 \
  polish/rawcounts.iso88592 \
  russian/rawcounts.koi8r \
  slovak/rawcounts.cp1250 \
  slovene/rawcounts.iso88592 \
  ukrainian/rawcounts.cp1251

PAIRCOUNTS = \
  belarussian/paircounts.cp1251 \
  belarussian/paircounts.ibm866 \
  bulgarian/paircounts.cp1251 \
  russian/paircounts.koi8r \
  ukrainian/paircounts.cp1251

MAPS = \
  maps/baltic.map \
  maps/cork.map \
  maps/cp1125.map \
  maps/cp1250.map \
  maps/cp1251.map \
  maps/cp1257.map \
  maps/ecma113.map \
  maps/ibm775.map \
  maps/ibm852.map \
  maps/ibm855.map \
  maps/ibm866-bad.map \
  maps/ibm866.map \
  maps/iso885913.map \
  maps/iso885916.map \
  maps/iso88592.map \
  maps/iso88594.map \
  maps/iso88595.map \
  maps/keybcs2.map \
  maps/koi8cs2.map \
  maps/koi8r.map \
  maps/koi8u.map \
  maps/koi8ub.map \
  maps/koi8uni.map \
  maps/macce.map \
  maps/maccyr.map

OTHERDATA = \
  chinese/zh_weight_big5.txt \
  chinese/zh_weight_gb.txt

EXTRA_DIST = README $(BASES) $(RAWCOUNTS) $(PAIRCOUNTS) $(MAPS) $(OTHERDATA) \
             $(noinst_SCRPITS) Letters

# We don't want to recreate the headers over and over, namely we don't want
# to recreate them during normal compilation.  Run `make headers' to create
# them.
headers: $(BASES) xlt letters
	./doit.sh

letters:
	./map2letters.sh

.PHONY: headers letters

clean-local:
	-rm -f *~ */*~ core.*

distclean-local:
	-./clean.sh
	-rm -f $(noinst_PROGRAMS)
