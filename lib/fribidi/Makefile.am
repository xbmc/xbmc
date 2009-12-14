
AUTOMAKE_OPTIONS = gnu 

noinst_PROGRAMS =	\
	fribidi_benchmark	\
	fribidi_create_char_types	\
	fribidi_create_mirroring	\
	fribidi_bidi_types

libfribidi_charsets =	\
	fribidi_char_sets.c	\
	fribidi_char_sets_utf8.c

libfribidi_charsets_extra =	\
	fribidi_char_sets_cap_rtl.c	\
	fribidi_char_sets_iso8859_6.c	\
	fribidi_char_sets_iso8859_8.c	\
	fribidi_char_sets_cp1255.c	\
	fribidi_char_sets_cp1256.c	\
	fribidi_char_sets_isiri_3342.c

libfribidi_charsets_h =	\
	fribidi_char_sets.h	\
	fribidi_char_sets.i	\
	fribidi_char_sets_utf8.h

libfribidi_charsets_extra_h =	\
	fribidi_char_sets_cap_rtl.h	\
	fribidi_char_sets_cp1255.h	\
	fribidi_char_sets_cp1256.h	\
	fribidi_char_sets_isiri_3342.h	\
	fribidi_char_sets_iso8859_6.h	\
	fribidi_char_sets_iso8859_8.h

libfribidi_extra_h =	

lib_LTLIBRARIES = libfribidi.la

libfribidi_la_SOURCES =	\
	fribidi.c	\
	fribidi_types.c		\
	fribidi_mem.c	\
	fribidi_mirroring.c	\
	fribidi_char_type.c	\
	fribidi_utils.c	\
	wcwidth.c	\
	$(libfribidi_charsets)	\
	$(libfribidi_charsets_extra)

libfribidi_la_LDFLAGS = -no-undefined

libfribidi_la_LIBADD = 

libfribidiincdir = $(includedir)/fribidi

libfribidiinc_HEADERS =	\
	fribidi.h	\
	fribidi_types.h	\
	fribidi_types.i	\
	fribidi_unicode.h	\
	$(libfribidi_charsets_h)	\
	$(libfribidi_charsets_extra_h)	\
	fribidi_config.h	\
	fribidi_mem.h

GETOPT_SRC =	\
	getopt.c	\
	getopt1.c

GETOPT_HDR =	\
	getopt.h	\
	getopt_int.h	\
	gettext.h

TEST_FILES =	\
	tests/test_CapRTL_explicit.input	\
	tests/test_CapRTL_explicit.reference	\
	tests/test_CapRTL_implicit.input	\
	tests/test_CapRTL_implicit.reference	\
	tests/test_ISO8859-8_hebrew.input	\
	tests/test_ISO8859-8_hebrew.reference	\
	tests/test_UTF-8_persian.input	\
	tests/test_UTF-8_persian.reference

TABLE_FILES =	\
	fribidi_tab_mirroring.i	\
	fribidi_tab_char_type_2.i	\
	fribidi_tab_char_type_9.i

UNIDATA_FILES =	\
	unidata/README	\
	unidata/UnicodeData.txt	\
	unidata/BidiMirroring.txt

OTHER_FILES =	\
	wcwidth.i

TESTS = run.tests

EXTRA_HEADERS =	\
	packtab.h	\
	$(libfribidi_extra_h)	\
	$(GETOPT_HDR)

EXTRA_DIST =	\
	bootstrap \
	fribidi.spec	\
	$(TESTS)	\
	$(TEST_FILES)	\
	$(TABLE_FILES)	\
	$(UNIDATA_FILES)	\
	$(OTHER_FILES)	\
	$(EXTRA_HEADERS)

fribidi_benchmark_SOURCES = fribidi_benchmark.c $(GETOPT_SRC)
fribidi_benchmark_LDADD = libfribidi.la

fribidi_bidi_types_LDADD = libfribidi.la

fribidi_create_char_types_SOURCES = fribidi_create_char_types.c packtab.c

fribidi_create_mirroring_SOURCES = fribidi_create_mirroring.c

bin_PROGRAMS = fribidi
fribidi_SOURCES = fribidi_main.c $(GETOPT_SRC)
fribidi_LDADD = libfribidi.la


fribidi_tab_mirroring.i: $(fribidi_create_mirroring_SOURCES) fribidi_types.h	\
		unidata/BidiMirroring.txt
	$(MAKE) fribidi_create_mirroring
	if test -d unidata; then UNIDATA=unidata;	\
	else UNIDATA="$(srcdir)/unidata"; fi &&	\
	./fribidi_create_mirroring "$$UNIDATA" ||	\
	($(RM) "$@"; false)


$(srcdir)/fribidi_mirroring.c: fribidi_tab_mirroring.i
	touch "$@" || $(MAKE) clean

fribidi_tab_mirroring:	fribidi_tab_mirroring.i


fribidi_tab_char_type_%.i: $(fribidi_create_char_types_SOURCES)	\
		unidata/UnicodeData.txt
	$(MAKE) fribidi_create_char_types
	if test -d unidata; then UNIDATA=unidata;	\
	else UNIDATA="$(srcdir)/unidata"; fi &&	\
	./fribidi_create_char_types "`echo "$@"	\
	  | sed 's/[^0-9]*//g'`" "$$UNIDATA" ||	\
	($(RM) "$@"; false)
	$(RM) fribidi_tab_char_type_stamp
	$(MAKE) fribidi_tab_char_type_stamp

fribidi_tab_char_type_stamp:
	touch "$(srcdir)/fribidi_char_type.c" || $(MAKE) clean
	touch "$@"

fribidi_tab_char_type_small:
	$(MAKE) fribidi_tab_char_type_9.i

fribidi_tab_char_type_large:
	$(MAKE) fribidi_tab_char_type_2.i

fribidi_tab tab:	fribidi_tab_char_type_small	\
			fribidi_tab_char_type_large	\
			fribidi_tab_mirroring


config.h: fribidi_tab_char_type_stamp
	./config.status --recheck
	./config.status
	touch "$@"

$(srcdir)/fribidi_char_type.c: config.h

tests/test_%.reference: tests/test_%.input $(libfribidi_la_SOURCES)
	$(MAKE) fribidi
	(test -d tests || mkdir tests) &&	\
	testcase="$@" && 	\
	charset="`echo "$@" | sed 's/_[^_]*$$//;s/.*_//'`" &&	\
	./fribidi --test --charset "$$charset" "$<" > "$@" || \
	($(RM) "$@"; false)

test.reference:
	for testcase in "$(srcdir)"/tests/test_*.input; do	\
		test="`echo "$$testcase"	\
		  | sed 's/\.input$$/.reference/i; s|.*/||;'`" &&	\
		$(MAKE) "tests/$$test" ||	\
		exit 1;	\
	done

test: fribidi
	"$(srcdir)/run.tests"

.PHONY:		test test.reference	\
		fribidi_tab_char_type_small fribidi_tab_char_type_large	\
		fribidi_tab_mirroring fribidi_tab tab

bin_SCRIPTS = fribidi-config

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = fribidi.pc

CLEANFILES = 

DISTCLEANFILES = fribidi_tab_char_type_stamp

MAINTAINERCLEANFILES = \
		$(srcdir)/INSTALL \
		$(srcdir)/aclocal.m4 \
		$(srcdir)/autoscan.log \
		$(srcdir)/compile \
		$(srcdir)/config.guess \
		$(srcdir)/config.h.in \
		$(srcdir)/config.sub \
		$(srcdir)/configure.scan \
		$(srcdir)/depcomp \
		$(srcdir)/install-sh \
		$(srcdir)/ltmain.sh \
		$(srcdir)/missing \
		`find "$(srcdir)" -type f -name Makefile.in -print` \
		`find "$(srcdir)" -type f -name "*~" -print` \
		`$(srcdir)/bootstrap --write-configure`
# The last line above is a horrible hack.
# GNU Coding Standards recommends that `make maintainer-clean' should not 
# remove the configure script.
# We instead make configure call bootstrap.

# Indent all C source and header files, using indent(1):

indent: $(srcdir)/.indent.pro
	cd "$(srcdir)" && ./missing --run indent fribidi_*.[ch]

.PHONY: indent

