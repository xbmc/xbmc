# @(#) $Id: Makefile.am,v 1.26 2005/12/01 10:12:10 yeti Exp $
TESTS_ENVIRONMENT = srcdir=@srcdir@ top_srcdir=@top_srcdir@ top_builddir=@top_builddir@

TESTS = \
  ./test-lists.sh \
  ./test-long-texts.sh \
  ./test-convlist.sh \
  ./test-guess-short.sh \
  ./test-guess-stdin.sh \
  ./test-guess-utf8.sh \
  ./test-pP.sh \
  ./test-aliases.sh \
  ./test-empty.sh \
  ./test-convert-64.sh \
  ./test-convert-filter.sh \
  ./test-errors.sh \
  ./test-ENCAOPT.sh \
  ./test-default-cs.sh \
  ./test-external.sh \
  ./test-iconv.sh \
  ./test-recode.sh

noinst_SCRIPTS = \
  dummy-converter.sh \
  failing-converter.sh \
  failing-converter2.sh \
  finish.sh \
  setup.sh

if MAINTAINER_MODE
noinst_PROGRAMS = simtable
simtable_SOURCES = simtable.c
simtable_LDFLAGS = @LDFLAGS@ @LIBS@
simtable_LDADD = $(top_builddir)/lib/libenca.la
simtable_DEPENDENCIES = $(top_builddir)/lib/libenca.la
INCLUDES = -I$(top_srcdir)/lib
endif

EXTRA_DIST = $(TEST_SAMPLES) $(TESTS) $(TEST_RESULTS) $(noinst_SCRIPTS)

TEST_RESULTS = \
  test-guess-short.expected \
  test-guess-utf8.expected \
  test-external.expected \
  test-pP.expected

TEST_SAMPLES =  \
  be-s.cp1251 \
  be-s.ibm855 \
  be-s.ibm866 \
  be-s.iso88595 \
  be-s.koi8uni \
  be-s.maccyr \
  be-s.utf8 \
  be-utf8.ibm855 \
  be-utf8.iso88595 \
  bg-s.cp1251 \
  bg-s.cp1251.qp \
  bg-s.ecma113 \
  bg-s.ibm855 \
  bg-s.iso88595 \
  bg-s.maccyr \
  bg-s.utf8 \
  bg-utf8.bin \
  bg-utf8.cp1251 \
  bg-utf8.ecma113 \
  bg-utf8.ibm855 \
  bg-utf8.iso88595 \
  bg-utf8.maccyr \
  cs-s.cork \
  cs-s.cp1250 \
  cs-s.ibm852 \
  cs-s.ibm852.qp \
  cs-s.iso88592 \
  cs-s.keybcs2 \
  cs-s.koi8cs2 \
  cs-s.macce \
  cs-s.utf8 \
  cs-utf8.bin \
  cs-utf8.cork \
  cs-utf8.ibm852 \
  cs-utf8.iso88592 \
  et-s.baltic \
  et-s.cp1257 \
  et-s.ibm775 \
  et-s.iso885913 \
  et-s.iso88594 \
  et-s.iso88594.qp \
  et-s.macce \
  et-s.utf8 \
  et-utf8.bin \
  et-utf8.ibm775 \
  et-utf8.iso885913 \
  et-utf8.iso88594 \
  hr-s.cork \
  hr-s.cp1250 \
  hr-s.ibm852 \
  hr-s.iso88592 \
  hr-s.iso88592.qp \
  hr-s.macce \
  hr-s.utf8 \
  hr-utf8.cp1250 \
  hr-utf8.ibm852 \
  hu-s.cork \
  hu-s.cp1250 \
  hu-s.ibm852 \
  hu-s.iso88592 \
  hu-s.macce \
  hu-s.macce.qp \
  hu-s.utf8 \
  hu-utf8.cork \
  hu-utf8.cp1250 \
  hu-utf8.ibm852 \
  hu-utf8.macce \
  lt-s.baltic \
  lt-s.cp1257 \
  lt-s.ibm775 \
  lt-s.iso885913 \
  lt-s.iso88594 \
  lt-s.macce \
  lt-s.utf8 \
  lt-utf8.ibm775 \
  lt-utf8.iso88594 \
  lt-utf8.macce \
  lv-s.baltic \
  lv-s.cp1257 \
  lv-s.ibm775 \
  lv-s.iso885913 \
  lv-s.iso885913.qp \
  lv-s.iso88594 \
  lv-s.macce \
  lv-s.utf8 \
  lv-utf8.ibm775 \
  lv-utf8.iso88594 \
  lv-utf8.macce \
  none-s.ucs2 \
  none-s.ucs2le \
  none-s.ucs2le.qp \
  none-s.ucs4 \
  none-s.ucs4le \
  none-s.utf7 \
  none-s.utf8 \
  none-utf8.messy \
  pl-s.baltic \
  pl-s.baltic.qp \
  pl-s.cork \
  pl-s.cp1250 \
  pl-s.ibm852 \
  pl-s.iso885913 \
  pl-s.iso885916 \
  pl-s.iso88592 \
  pl-s.utf8 \
  pl-utf8.baltic \
  pl-utf8.ibm852 \
  pl-utf8.iso885916 \
  ru-s.cp1251 \
  ru-s.cp866 \
  ru-s.iso88595 \
  ru-s.koi8r \
  ru-s.maccyr \
  ru-s.maccyr.qp \
  ru-s.utf8 \
  ru-utf8.cp866 \
  ru-utf8.koi8r \
  ru-utf8.maccyr \
  sk-s.cork \
  sk-s.cp1250 \
  sk-s.cp1250.qp \
  sk-s.ibm852 \
  sk-s.iso88592 \
  sk-s.keybcs2 \
  sk-s.koi8cs2 \
  sk-s.macce \
  sk-s.utf8 \
  sk-utf8.cp1250 \
  sk-utf8.keybcs2 \
  sl-s.cork \
  sl-s.cp1250 \
  sl-s.ibm852 \
  sl-s.ibm852.qp \
  sl-s.iso88592 \
  sl-s.macce \
  sl-s.utf8 \
  sl-utf8.cork \
  sl-utf8.ibm852 \
  sl-utf8.iso88592 \
  sl-utf8.macce \
  uk-s.cp1125 \
  uk-s.cp1251 \
  uk-s.ibm855 \
  uk-s.iso88595 \
  uk-s.koi8u \
  uk-s.maccyr \
  uk-s.utf8 \
  uk-utf8.bin \
  uk-utf8.cp1251 \
  zh-s.big5 \
  zh-s.gbk \
  zh-s.hz \
  zh-s.utf8

# alias test=check
test: check

.PHONY: test

clean-local:
	rm -f test-*.actual core.* core *.tmp *~ *-big.* test-guess-stdin.expected test-ENCAOPT.expected

dist-hook:
	chmod 755 $(distdir)/test-*.sh

