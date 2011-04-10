## Process this file with automake to produce Makefile.in.

dist_doc_DATA = \
  doc/pcre.txt \
  doc/pcre-config.txt \
  doc/pcregrep.txt \
  doc/pcretest.txt \
  AUTHORS \
  COPYING \
  ChangeLog \
  LICENCE \
  NEWS \
  README

dist_html_DATA = \
  doc/html/index.html \
  doc/html/pcre.html \
  doc/html/pcre-config.html \
  doc/html/pcre_compile.html \
  doc/html/pcre_compile2.html \
  doc/html/pcre_config.html \
  doc/html/pcre_copy_named_substring.html \
  doc/html/pcre_copy_substring.html \
  doc/html/pcre_dfa_exec.html \
  doc/html/pcre_exec.html \
  doc/html/pcre_free_substring.html \
  doc/html/pcre_free_substring_list.html \
  doc/html/pcre_fullinfo.html \
  doc/html/pcre_get_named_substring.html \
  doc/html/pcre_get_stringnumber.html \
  doc/html/pcre_get_stringtable_entries.html \
  doc/html/pcre_get_substring.html \
  doc/html/pcre_get_substring_list.html \
  doc/html/pcre_info.html \
  doc/html/pcre_maketables.html \
  doc/html/pcre_refcount.html \
  doc/html/pcre_study.html \
  doc/html/pcre_version.html \
  doc/html/pcreapi.html \
  doc/html/pcrebuild.html \
  doc/html/pcrecallout.html \
  doc/html/pcrecompat.html \
  doc/html/pcredemo.html \
  doc/html/pcregrep.html \
  doc/html/pcrematching.html \
  doc/html/pcrepartial.html \
  doc/html/pcrepattern.html \
  doc/html/pcreperform.html \
  doc/html/pcreposix.html \
  doc/html/pcreprecompile.html \
  doc/html/pcresample.html \
  doc/html/pcrestack.html \
  doc/html/pcresyntax.html \
  doc/html/pcretest.html

pcrecpp_html = doc/html/pcrecpp.html
dist_noinst_DATA = $(pcrecpp_html)

if WITH_PCRE_CPP
html_DATA = $(pcrecpp_html)
endif

# The Libtool libraries to install.  We'll add to this later.
lib_LTLIBRARIES =

# Unit tests you want to run when people type 'make check'.
# TESTS is for binary unit tests, check_SCRIPTS for script-based tests
TESTS =
check_SCRIPTS =
dist_noinst_SCRIPTS =

# Some of the binaries we make are to be installed, and others are
# (non-user-visible) helper programs needed to build libpcre.
bin_PROGRAMS =
noinst_PROGRAMS =

# Additional files to delete on 'make clean' and 'make maintainer-clean'.
CLEANFILES =
MAINTAINERCLEANFILES =

# Additional files to bundle with the distribution, over and above what
# the Autotools include by default.
EXTRA_DIST =

# These files contain maintenance information
EXTRA_DIST += \
  doc/perltest.txt \
  NON-UNIX-USE \
  HACKING

# These files are used in the preparation of a release
EXTRA_DIST += \
  PrepareRelease \
  CleanTxt \
  Detrail \
  132html \
  doc/index.html.src

# These files are to do with building for Virtual Pascal
EXTRA_DIST += \
  makevp.bat \
  makevp_c.txt \
  makevp_l.txt \
  pcregexp.pas

# These files are usable versions of pcre.h and config.h that are distributed
# for the benefit of people who are building PCRE manually, without the
# Autotools support.
EXTRA_DIST += \
  pcre.h.generic \
  config.h.generic

pcre.h.generic: configure.ac
	rm -f $@
	cp -p pcre.h $@

MAINTAINERCLEANFILES += pcre.h.generic

# These are the header files we'll install. We do not distribute pcre.h because
# it is generated from pcre.h.in.
nodist_include_HEADERS = \
  pcre.h
include_HEADERS = \
  pcreposix.h

# These additional headers will be be installed if C++ support is enabled. We
# do not distribute pcrecpparg.h or pcre_stringpiece.h, as these are generated
# from corresponding .h.in files (which we do distribute).
if WITH_PCRE_CPP
nodist_include_HEADERS += \
  pcrecpparg.h \
  pcre_stringpiece.h
include_HEADERS += \
  pcrecpp.h \
  pcre_scanner.h
endif # WITH_PCRE_CPP

bin_SCRIPTS = pcre-config

## ---------------------------------------------------------------
## The dftables program is used to rebuild character tables before compiling
## PCRE, if --enable-rebuild-chartables is specified. It is not a user-visible
## program. The default (when --enable-rebuild-chartables is not specified) is
## to copy a distributed set of tables that are defined for ASCII code. In this
## case, dftables is not needed.

if WITH_REBUILD_CHARTABLES

noinst_PROGRAMS += dftables
dftables_SOURCES = dftables.c

pcre_chartables.c: dftables$(EXEEXT)
	./dftables$(EXEEXT) $@
else

pcre_chartables.c: $(srcdir)/pcre_chartables.c.dist
	rm -f $@
	$(LN_S) $(srcdir)/pcre_chartables.c.dist $@

endif # WITH_REBUILD_CHARTABLES


## The main pcre library
lib_LTLIBRARIES += libpcre.la
libpcre_la_SOURCES = \
  pcre_compile.c \
  pcre_config.c \
  pcre_dfa_exec.c \
  pcre_exec.c \
  pcre_fullinfo.c \
  pcre_get.c \
  pcre_globals.c \
  pcre_info.c \
  pcre_internal.h \
  pcre_maketables.c \
  pcre_newline.c \
  pcre_ord2utf8.c \
  pcre_refcount.c \
  pcre_study.c \
  pcre_tables.c \
  pcre_try_flipped.c \
  pcre_ucd.c \
  pcre_valid_utf8.c \
  pcre_version.c \
  pcre_xclass.c \
  ucp.h

## This file is generated as part of the building process, so don't distribute.
nodist_libpcre_la_SOURCES = \
  pcre_chartables.c

# The pcre_printint.src file is #included by some source files, so it must be
# distributed. The pcre_chartables.c.dist file is the default version of
# pcre_chartables.c, used unless --enable-rebuild-chartables is specified.
EXTRA_DIST += pcre_printint.src pcre_chartables.c.dist

libpcre_la_LDFLAGS = $(EXTRA_LIBPCRE_LDFLAGS)

CLEANFILES += pcre_chartables.c

## A version of the main pcre library that has a posix re API.
lib_LTLIBRARIES += libpcreposix.la
libpcreposix_la_SOURCES = \
  pcreposix.c
libpcreposix_la_LDFLAGS = $(EXTRA_LIBPCREPOSIX_LDFLAGS)
libpcreposix_la_LIBADD = libpcre.la

## There's a C++ library as well.
if WITH_PCRE_CPP

lib_LTLIBRARIES += libpcrecpp.la
libpcrecpp_la_SOURCES = \
  pcrecpp_internal.h \
  pcrecpp.cc \
  pcre_scanner.cc \
  pcre_stringpiece.cc
libpcrecpp_la_LDFLAGS = $(EXTRA_LIBPCRECPP_LDFLAGS)
libpcrecpp_la_LIBADD = libpcre.la

TESTS += pcrecpp_unittest
noinst_PROGRAMS += pcrecpp_unittest
pcrecpp_unittest_SOURCES = pcrecpp_unittest.cc
pcrecpp_unittest_LDADD = libpcrecpp.la

TESTS += pcre_scanner_unittest
noinst_PROGRAMS += pcre_scanner_unittest
pcre_scanner_unittest_SOURCES = pcre_scanner_unittest.cc
pcre_scanner_unittest_LDADD = libpcrecpp.la

TESTS += pcre_stringpiece_unittest
noinst_PROGRAMS += pcre_stringpiece_unittest
pcre_stringpiece_unittest_SOURCES = pcre_stringpiece_unittest.cc
pcre_stringpiece_unittest_LDADD = libpcrecpp.la

endif # WITH_PCRE_CPP

## The main unit tests

# Each unit test is a binary plus a script that runs that binary in various
# ways. We install these test binaries in case folks find it helpful.

TESTS += RunTest
dist_noinst_SCRIPTS += RunTest
EXTRA_DIST += RunTest.bat
bin_PROGRAMS += pcretest
pcretest_SOURCES = pcretest.c
pcretest_LDADD = libpcreposix.la $(LIBREADLINE)

TESTS += RunGrepTest
dist_noinst_SCRIPTS += RunGrepTest
bin_PROGRAMS += pcregrep
pcregrep_SOURCES = pcregrep.c
pcregrep_LDADD = libpcreposix.la $(LIBZ) $(LIBBZ2)

EXTRA_DIST += \
  testdata/grepinput \
  testdata/grepinput8 \
  testdata/grepinputv \
  testdata/grepinputx \
  testdata/greplist \
  testdata/grepoutput \
  testdata/grepoutput8 \
  testdata/grepoutputN \
  testdata/testinput1 \
  testdata/testinput2 \
  testdata/testinput3 \
  testdata/testinput4 \
  testdata/testinput5 \
  testdata/testinput6 \
  testdata/testinput7 \
  testdata/testinput8 \
  testdata/testinput9 \
  testdata/testinput10 \
  testdata/testinput11 \
  testdata/testinput12 \
  testdata/testoutput1 \
  testdata/testoutput2 \
  testdata/testoutput3 \
  testdata/testoutput4 \
  testdata/testoutput5 \
  testdata/testoutput6 \
  testdata/testoutput7 \
  testdata/testoutput8 \
  testdata/testoutput9 \
  testdata/testoutput10 \
  testdata/testoutput11 \
  testdata/testoutput12 \
  testdata/wintestinput3 \
  testdata/wintestoutput3 \
  perltest.pl

CLEANFILES += \
	testsavedregex \
	teststderr \
	testtry \
        testNinput


# PCRE demonstration program. No longer built automatcally. The point is that
# the users should build it themselves. So just distribute the source.
# noinst_PROGRAMS += pcredemo
# pcredemo_SOURCES = pcredemo.c
# pcredemo_LDADD = libpcre.la

EXTRA_DIST += pcredemo.c


## Utility rules, documentation, etc.

# A compatibility line, the old build system worked with 'make test'
test: check ;


# A PCRE user submitted the following addition, saying that it "will allow
# anyone using the 'mingw32' compiler to simply type 'make pcre.dll' and get a
# nice DLL for Windows use". (It is used by the pcre.dll target.)
DLL_OBJS= pcre_compile.o pcre_config.o \
	pcre_dfa_exec.o pcre_exec.o pcre_fullinfo.o pcre_get.o \
	pcre_globals.o pcre_info.o pcre_maketables.o \
	pcre_newline.o pcre_ord2utf8.o pcre_refcount.o \
	pcre_study.o pcre_tables.o pcre_try_flipped.o \
	pcre_ucd.o pcre_valid_utf8.o pcre_version.o \
	pcre_chartables.o \
	pcre_xclass.o

# A PCRE user submitted the following addition, saying that it "will allow
# anyone using the 'mingw32' compiler to simply type 'make pcre.dll' and get a
# nice DLL for Windows use".
pcre.dll: $(DLL_OBJS)
	$(CC) -shared -o pcre.dll -Wl,"--strip-all" -Wl,"--export-all-symbols" $(DLL_OBJS)


# We have .pc files for pkg-config users.
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libpcre.pc libpcreposix.pc
if WITH_PCRE_CPP
pkgconfig_DATA += libpcrecpp.pc
endif

dist_man_MANS = \
  doc/pcre.3 \
  doc/pcre-config.1 \
  doc/pcre_compile.3 \
  doc/pcre_compile2.3 \
  doc/pcre_config.3 \
  doc/pcre_copy_named_substring.3 \
  doc/pcre_copy_substring.3 \
  doc/pcre_dfa_exec.3 \
  doc/pcre_exec.3 \
  doc/pcre_free_substring.3 \
  doc/pcre_free_substring_list.3 \
  doc/pcre_fullinfo.3 \
  doc/pcre_get_named_substring.3 \
  doc/pcre_get_stringnumber.3 \
  doc/pcre_get_stringtable_entries.3 \
  doc/pcre_get_substring.3 \
  doc/pcre_get_substring_list.3 \
  doc/pcre_info.3 \
  doc/pcre_maketables.3 \
  doc/pcre_refcount.3 \
  doc/pcre_study.3 \
  doc/pcre_version.3 \
  doc/pcreapi.3 \
  doc/pcrebuild.3 \
  doc/pcrecallout.3 \
  doc/pcrecompat.3 \
  doc/pcregrep.1 \
  doc/pcrematching.3 \
  doc/pcrepartial.3 \
  doc/pcrepattern.3 \
  doc/pcreperform.3 \
  doc/pcreposix.3 \
  doc/pcreprecompile.3 \
  doc/pcresample.3 \
  doc/pcrestack.3 \
  doc/pcresyntax.3 \
  doc/pcretest.1

pcrecpp_man = doc/pcrecpp.3
EXTRA_DIST += $(pcrecpp_man)

if WITH_PCRE_CPP
man_MANS = $(pcrecpp_man)
endif

## CMake support

EXTRA_DIST += \
  cmake/COPYING-CMAKE-SCRIPTS \
  cmake/FindPackageHandleStandardArgs.cmake \
  cmake/FindReadline.cmake \
  CMakeLists.txt \
  config-cmake.h.in

## end Makefile.am
