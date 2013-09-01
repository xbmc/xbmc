# -*-Makefile-*-
# This Makefile fragment tries to be general-purpose enough to be
# used by at least coreutils, idutils, CPPI, Bison, and Autoconf.

## Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009,
## 2010 Free Software Foundation, Inc.
##
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.

# This is reported not to work with make-3.79.1
# ME := $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))
ME := maint.mk

# Override this in cfg.mk if you use a non-standard build-aux directory.
build_aux ?= $(srcdir)/build-aux

# Do not save the original name or timestamp in the .tar.gz file.
# Use --rsyncable if available.
gzip_rsyncable := \
  $(shell gzip --help 2>/dev/null|grep rsyncable >/dev/null \
    && printf %s --rsyncable)
GZIP_ENV = '--no-name --best $(gzip_rsyncable)'

GIT = git
VC = $(GIT)
VC-tag = git tag -s -m '$(VERSION)' -u '$(gpg_key_ID)'

VC_LIST = $(build_aux)/vc-list-files -C $(srcdir)

VC_LIST_EXCEPT = \
  $(VC_LIST) | if test -f $(srcdir)/.x-$@; then	\
    grep -vEf $(srcdir)/.x-$@;			\
  else						\
    grep -v ChangeLog;				\
  fi

ifeq ($(origin prev_version_file), undefined)
  prev_version_file = $(srcdir)/.prev-version
endif

PREV_VERSION := $(shell cat $(prev_version_file) 2>/dev/null)
VERSION_REGEXP = $(subst .,\.,$(VERSION))
PREV_VERSION_REGEXP = $(subst .,\.,$(PREV_VERSION))

this-vc-tag = v$(VERSION)
this-vc-tag-regexp = v$(VERSION_REGEXP)
my_distdir = $(PACKAGE)-$(VERSION)

# Old releases are stored here.
# Used for diffs.
release_archive_dir ?= ../release

# Override gnu_rel_host and url_dir_list in cfg.mk if these are not right.
# Use alpha.gnu.org for alpha and beta releases.
# Use ftp.gnu.org for stable releases.
gnu_ftp_host-alpha = alpha.gnu.org
gnu_ftp_host-beta = alpha.gnu.org
gnu_ftp_host-stable = ftp.gnu.org
gnu_rel_host ?= $(gnu_ftp_host-$(RELEASE_TYPE))

ifeq ($(gnu_rel_host),ftp.gnu.org)
url_dir_list ?= http://ftpmirror.gnu.org/$(PACKAGE)
else
url_dir_list ?= ftp://$(gnu_rel_host)/gnu/$(PACKAGE)
endif

# Prevent programs like 'sort' from considering distinct strings to be equal.
# Doing it here saves us from having to set LC_ALL elsewhere in this file.
export LC_ALL = C



## --------------- ##
## Sanity checks.  ##
## --------------- ##

# Collect the names of rules starting with `sc_'.
syntax-check-rules := $(shell sed -n 's/^\(sc_[a-zA-Z0-9_-]*\):.*/\1/p' \
			$(srcdir)/$(ME))
.PHONY: $(syntax-check-rules)

local-checks-available = \
  po-check copyright-check writable-files m4-check author_mark_check \
  changelog-check patch-check strftime-check $(syntax-check-rules) \
  makefile_path_separator_check \
  makefile-check check-AUTHORS
.PHONY: $(local-checks-available)

local-check := $(filter-out $(local-checks-to-skip), $(local-checks-available))

syntax-check: $(local-check)
#	@shopt -s nullglob;						\
#	grep -nE '#  *include <(limits|std(def|arg|bool))\.h>'		\
#	    $$(find -type f -name '*.[chly]') /dev/null &&		\
#	  { echo '$(ME): found conditional include' 1>&2;		\
#	    exit 1; } || :

#	grep -nE '^#  *include <(string|stdlib)\.h>'			\
#	    $(srcdir)/{lib,src}/*.[chly] /dev/null &&			\
#	  { echo '$(ME): FIXME' 1>&2;					\
#	    exit 1; } || :
# FIXME: don't allow `#include .strings\.h' anywhere

sc_cast_of_argument_to_free:
	@shopt -s nullglob;						\
	grep -nE '\<free \(\('						\
	    $(srcdir)/{lib,src}/*.[chly] /dev/null &&			\
	  { echo '$(ME): don'\''t cast free argument' 1>&2;		\
	    exit 1; } || :

sc_cast_of_x_alloc_return_value:
	@shopt -s nullglob;						\
	grep -nE --exclude=$(srcdir)/lib/regex.c			\
	    '\*\) *x(m|c|re)alloc\>'					\
	    $(srcdir)/{lib,src}/*.[chly] /dev/null &&			\
	  { echo '$(ME): don'\''t cast x*alloc return value' 1>&2;	\
	    exit 1; } || :

sc_cast_of_alloca_return_value:
	@shopt -s nullglob;						\
	grep -nE '\*\) *alloca\>'					\
	    $(srcdir)/src/*.[chly] /dev/null &&				\
	  { echo '$(ME): don'\''t cast alloca return value' 1>&2;	\
	    exit 1; } || :

sc_space_tab:
	@grep -n '[ ]	' $$($(VC_LIST_EXCEPT)) &&			\
	  { echo '$(ME): found SPACE-TAB sequence; remove the SPACE'	\
		1>&2; exit 1; } || :

# Don't use *scanf or the old ato* functions in `real' code.
# They provide no error checking mechanism.
# Instead, use strto* functions.
sc_prohibit_atoi_atof:
	@grep -nE '\<([fs]?scanf|ato([filq]|ll))\>' $$($(VC_LIST_EXCEPT)) && \
	  { echo '$(ME): do not use *scan''f, ato''f, ato''i, ato''l, ato''ll, ato''q, or ss''canf'	\
		1>&2; exit 1; } || :

# Using EXIT_SUCCESS as the first argument to error is misleading,
# since when that parameter is 0, error does not exit.  Use `0' instead.
sc_error_exit_success:
	@grep -nF 'error (EXIT_SUCCESS,'				\
	    $$(find -type f -name '*.[chly]') /dev/null &&		\
	  { echo '$(ME): found error (EXIT_SUCCESS' 1>&2;		\
	    exit 1; } || :

sc_file_system:
	@grep -ni 'file''system' $$($(VC_LIST_EXCEPT))			\
	  | grep -v 'File''system Hierarchy Standard' &&		\
	  { echo '$(ME): found use of "file''system";'			\
	    'rewrite to use "file system"' 1>&2;			\
	    exit 1; } || :

sc_no_have_config_h:
	@grep -n '^# *if.*HAVE''_CONFIG_H' $$($(VC_LIST_EXCEPT)) &&	\
	  { echo '$(ME): found use of HAVE''_CONFIG_H; remove'		\
		1>&2; exit 1; } || :

# Nearly all .c files must include <config.h>.
sc_require_config_h:
	@if $(VC_LIST_EXCEPT) | grep '\.c$$' > /dev/null; then		\
	  grep -L '^# *include <config\.h>'				\
		$$($(VC_LIST_EXCEPT) | grep '\.c$$')			\
	      | grep . &&						\
	  { echo '$(ME): the above files do not include <config.h>'	\
		1>&2; exit 1; } || :;					\
	else :;								\
	fi

# To use this "command" macro, you must first define two shell variables:
# h: the header, enclosed in <> or ""
# re: a regular expression that matches IFF something provided by $h is used.
define _header_without_use
  h_esc=`echo "$$h"|sed 's/\./\\./'`;					\
  if $(VC_LIST_EXCEPT) | grep '\.c$$' > /dev/null; then			\
    files=$$(grep -l '^# *include '"$$h_esc"				\
	     $$($(VC_LIST_EXCEPT) | grep '\.c$$')) &&			\
    grep -LE "$$re" $$files | grep . &&					\
      { echo "$(ME): the above files include $$h but don't use it"	\
	1>&2; exit 1; } || :;						\
  else :;								\
  fi
endef

# Prohibit the inclusion of assert.h without an actual use of assert.
sc_prohibit_assert_without_use:
	@h='<assert.h>' re='\<assert *\(' $(_header_without_use)

# Prohibit the inclusion of getopt.h without an actual use.
sc_prohibit_getopt_without_use:
	@h='<getopt.h>' re='\<getopt(_long)? *\(' $(_header_without_use)

# Don't include quotearg.h unless you use one of its functions.
sc_prohibit_quotearg_without_use:
	@h='"quotearg.h"' re='\<quotearg(_[^ ]+)? *\(' $(_header_without_use)

# Don't include quote.h unless you use one of its functions.
sc_prohibit_quote_without_use:
	@h='"quote.h"' re='\<quote(_n)? *\(' $(_header_without_use)

# Don't include this header unless you use one of its functions.
sc_prohibit_long_options_without_use:
	@h='"long-options.h"' re='\<parse_long_options *\(' \
	  $(_header_without_use)

# Don't include this header unless you use one of its functions.
sc_prohibit_inttostr_without_use:
	@h='"inttostr.h"' re='\<(off|[iu]max|uint)tostr *\(' \
	  $(_header_without_use)

# Don't include this header unless you use one of its functions.
sc_prohibit_error_without_use:
	@h='"error.h"' \
	re='\<error(_at_line|_print_progname|_one_per_line|_message_count)? *\('\
	  $(_header_without_use)

sc_prohibit_safe_read_without_use:
	@h='"safe-read.h"' re='(\<SAFE_READ_ERROR\>|\<safe_read *\()' \
	  $(_header_without_use)

sc_prohibit_argmatch_without_use:
	@h='"argmatch.h"' \
	re='(\<(ARRAY_CARDINALITY|X?ARGMATCH(|_TO_ARGUMENT|_VERIFY))\>|\<argmatch(_exit_fn|_(in)?valid) *\()' \
	  $(_header_without_use)

sc_prohibit_root_dev_ino_without_use:
	@h='"root-dev-ino.h"' \
	re='(\<ROOT_DEV_INO_(CHECK|WARN)\>|\<get_root_dev_ino *\()' \
	  $(_header_without_use)

sc_obsolete_symbols:
	@grep -nE '\<(HAVE''_FCNTL_H|O''_NDELAY)\>'			\
	     $$($(VC_LIST_EXCEPT)) &&					\
	  { echo '$(ME): do not use HAVE''_FCNTL_H or O''_NDELAY'	\
		1>&2; exit 1; } || :

# FIXME: warn about definitions of EXIT_FAILURE, EXIT_SUCCESS, STREQ

# Each nonempty line must start with a year number, or a TAB.
sc_changelog:
	@grep -n '^[^12	]' $$(find $(srcdir) -maxdepth 2 -name ChangeLog) && \
	  { echo '$(ME): found unexpected prefix in a ChangeLog' 1>&2;	\
	    exit 1; } || :

# Ensure that dd's definition of LONGEST_SYMBOL stays in sync
# with the strings from the two affected variables.
dd_c = $(srcdir)/src/dd.c
sc_dd_max_sym_length:
ifneq ($(wildcard $(dd_c)),)
	@len=$$( (sed -n '/conversions\[\] =$$/,/^};/p' $(dd_c);\
		 sed -n '/flags\[\] =$$/,/^};/p' $(dd_c) )	\
		|sed -n '/"/s/^[^"]*"\([^"]*\)".*/\1/p'		\
	      | wc --max-line-length);				\
	max=$$(sed -n '/^#define LONGEST_SYMBOL /s///p' $(dd_c)	\
	      |tr -d '"' | wc --max-line-length);		\
	if test "$$len" = "$$max"; then :; else			\
	  echo 'dd.c: LONGEST_SYMBOL is not longest' 1>&2;	\
	  exit 1;						\
	fi
endif

# Many m4 macros names once began with `jm_'.
# On 2004-04-13, they were all changed to start with gl_ instead.
# Make sure that none are inadvertently reintroduced.
sc_prohibit_jm_in_m4:
	@grep -nE 'jm_[A-Z]'					\
		$$($(VC_LIST) m4 |grep '\.m4$$') &&		\
	    { echo '$(ME): do not use jm_ in m4 macro names'	\
	      1>&2; exit 1; } || :

sc_root_tests:
	@t1=sc-root.expected; t2=sc-root.actual;			\
	grep -nl '^PRIV_CHECK_ARG=require-root'				\
	  $$($(VC_LIST) tests) |sed s/tests/./ |sort > $$t1;		\
	sed -n 's,	cd \([^ ]*\) .*MAKE..check TESTS=\(.*\),./\1/\2,p' \
	  $(srcdir)/tests/Makefile.am |sort > $$t2;			\
	diff -u $$t1 $$t2 || diff=1;					\
	rm -f $$t1 $$t2;						\
	test "$$diff"							\
	  && { echo 'tests/Makefile.am: missing check-root action'>&2;	\
	       exit 1; } || :

# Files in src/ should not include directly any of
# the headers already included via system.h.
sc_system_h_headers:
	@if test -f $(srcdir)/src/system.h; then			\
	  pat=$$(							\
	    sed -n '/^# *include /s///p' $(srcdir)/src/system.h /dev/null \
	    | grep -Ev 'sys/(param|file)\.h'				\
	    | sed 's/ .*//;;s/^["<]/^# *include [<"]/;s/\.h[">]$$/\\.h[">]/' \
	  ) &&								\
	  grep -nE -f "$pat"						\
	      $$($(VC_LIST) src |					\
		 grep -Ev '((copy|system)\.h|parse-gram\.c)$$')		\
	    && { echo '$(ME): the above are already included via system.h'\
		  1>&2;  exit 1; } || :;				\
	fi

sc_sun_os_names:
	@grep -nEi \
	    'solaris[^[:alnum:]]*2\.(7|8|9|[1-9][0-9])|sunos[^[:alnum:]][6-9]' \
	    $$($(VC_LIST_EXCEPT)) &&					\
	  { echo '$(ME): found misuse of Sun OS version numbers' 1>&2;	\
	    exit 1; } || :

sc_the_the:
	@grep -ni '\<the ''the\>' $$($(VC_LIST_EXCEPT)) &&		\
	  { echo '$(ME): found use of "the ''the";' 1>&2;		\
	    exit 1; } || :

sc_tight_scope:
	test ! -d src || $(MAKE) -C src $@

sc_trailing_blank:
	@grep -n '[	 ]$$' $$($(VC_LIST_EXCEPT)) &&			\
	  { echo '$(ME): found trailing blank(s)'			\
		1>&2; exit 1; } || :

# Match lines like the following, but where there is only one space
# between the options and the description:
#   -D, --all-repeated[=delimit-method]  print all duplicate lines\n
longopt_re = --[a-z][0-9A-Za-z-]*(\[?=[0-9A-Za-z-]*\]?)?
sc_two_space_separator_in_usage:
	@grep -nE '^   *(-[A-Za-z],)? $(longopt_re) [^ ].*\\$$'		\
	    $$($(VC_LIST_EXCEPT)) &&					\
	  { echo "$(ME): help2man requires at least two spaces between"; \
	    echo "$(ME): an option and its description"; \
		1>&2; exit 1; } || :

# Look for diagnostics that aren't marked for translation.
# This won't find any for which error's format string is on a separate line.
sc_unmarked_diagnostics:
	@grep -nE							\
	    '\<error \([^"]*"[^"]*[a-z]{3}' $$($(VC_LIST_EXCEPT))	\
	  | grep -v '_''(' &&						\
	  { echo '$(ME): found unmarked diagnostic(s)' 1>&2;		\
	    exit 1; } || :

# Avoid useless parentheses like those in this example:
# #if defined (SYMBOL) || defined (SYM2)
sc_useless_cpp_parens:
	@grep -n '^# *if .*defined *(' $$($(VC_LIST_EXCEPT)) &&		\
	  { echo '$(ME): found useless parentheses in cpp directive'	\
		1>&2; exit 1; } || :

# Ensure that the c99-to-c89 patch applies cleanly.
patch-check:
	if test -f src/c99-to-c89.diff; then				\
	  rm -rf src-c89 $@.1 $@.2 &&					\
	  cp -a src src-c89 &&						\
	  (cd src-c89; patch -V never --fuzz=0) < src/c99-to-c89.diff	\
	    > $@.1 2>&1 &&						\
	  { grep -v '^patching file ' $@.1 > $@.2 || :; } &&		\
	  test -s $@.2 &&						\
	  rm -rf src-c89 $@.1 $@.2;					\
	fi

# Ensure that date's --help output stays in sync with the info
# documentation for GNU strftime.  The only exception is %N,
# which date accepts but GNU strftime does not.
extract_char = sed 's/^[^%][^%]*%\(.\).*/\1/'
strftime-check:
	if test -f $(srcdir)/src/date.c; then				\
	  grep '^  %.  ' $(srcdir)/src/date.c | sort			\
	    | $(extract_char) > $@-src;					\
	  { echo N;							\
	    info libc date calendar format | grep '^    `%.'\'		\
	      | $(extract_char); } | sort > $@-info;			\
	  diff -u $@-src $@-info || exit 1;				\
	  rm -f $@-src $@-info;						\
	fi

check-AUTHORS:
	test ! -d src || $(MAKE) -C src $@

NEWS_hash =								\
  $$(sed -n '/^\*.* $(PREV_VERSION_REGEXP) ([0-9-]*)/,$$p'		\
       $(srcdir)/NEWS							\
     | perl -0777 -pe							\
	's/^Copyright.+?Free\sSoftware\sFoundation,\sInc\.\n//ms'	\
     | md5sum -								\
     | sed 's/ .*//')

# Ensure that we don't accidentally insert an entry into an old NEWS block.
sc_immutable_NEWS:
	@if test -f $(srcdir)/NEWS; then				\
	  test "$(NEWS_hash)" = '$(old_NEWS_hash)' && : ||		\
	    { echo '$(ME): you have modified old NEWS' 1>&2; exit 1; };	\
	fi

# Update the hash stored above.  Do this after each release and
# for any corrections to old entries.
update-NEWS-hash: NEWS
	perl -pi -e 's/^(old_NEWS_hash[ \t]+:?=[ \t]+).*/$${1}'"$(NEWS_hash)/" \
	  $(srcdir)/cfg.mk

# Ensure that we use only the standard $(VAR) notation,
# not @...@ in Makefile.am, now that we can rely on automake
# to emit a definition for each substituted variable.
makefile-check:
	grep -nE '@[A-Z_0-9]+@' `find $(srcdir) -name Makefile.am` \
	  && { echo '$(ME): use $$(...), not @...@' 1>&2; exit 1; } || :

news-date-check: NEWS
	today=`date +%Y-%m-%d`;						\
	if head NEWS | grep '^\*.* $(VERSION_REGEXP) ('$$today')'	\
	    >/dev/null; then						\
	  :;								\
	else								\
	  echo "version or today's date is not in NEWS" 1>&2;		\
	  exit 1;							\
	fi

changelog-check:
	if head ChangeLog | grep 'Version $(VERSION_REGEXP)\.$$'	\
	    >/dev/null; then						\
	  :;								\
	else								\
	  echo "$(VERSION) not in ChangeLog" 1>&2;			\
	  exit 1;							\
	fi

m4-check:
	@shopt -s nullglob;						\
	grep 'AC_DEFUN([^[]' m4/*.m4 /dev/null				\
	  && { echo '$(ME): quote the first arg to AC_DEFUN' 1>&2; \
	       exit 1; } || :

# Verify that all source files using _() are listed in po/POTFILES.in.
# FIXME: don't hard-code file names below; use a more general mechanism.
po-check:
	if test -f po/POTFILES.in; then					\
	  grep -E -v '^(#|$$)' po/POTFILES.in				\
	    | grep -v '^src/false\.c$$' | sort > $@-1;			\
	  files=;							\
	  for file in $$($(VC_LIST_EXCEPT)) lib/*.[ch]; do		\
	    case $$file in						\
	    djgpp/* | man/*) continue;;					\
	    */c99-to-c89.diff) continue;;				\
	    esac;							\
	    case $$file in						\
	    *.[ch])							\
	      base=`expr " $$file" : ' \(.*\)\..'`;			\
	      { test -f $$base.l || test -f $$base.y; } && continue;;	\
	    esac;							\
	    files="$$files $$file";					\
	  done;								\
	  grep -E -l '\b(N?_|gettext *)\([^)"]*("|$$)' $$files		\
	    | sort -u > $@-2;						\
	  diff -u $@-1 $@-2 || exit 1;					\
	  rm -f $@-1 $@-2;						\
	fi

# In a definition of #define AUTHORS "... and ..." where the RHS contains
# the English word `and', the string must be marked with `N_ (...)' so that
# gettext recognizes it as a string requiring translation.
author_mark_check:
	@shopt -s nullglob;						\
	grep -n '^# *define AUTHORS "[^"]* and ' src/*.c /dev/null	\
	  | grep -v ' N_ (' &&						\
	  { echo '$(ME): enclose the above strings in N_ (...)' 1>&2; \
	    exit 1; } || :

# Sometimes it is useful to change the PATH environment variable
# in Makefiles.  When doing so, it's better not to use the Unix-centric
# path separator of `:', but rather the automake-provided `@PATH_SEPARATOR@'.
# It'd be better to use `find -print0 ...|xargs -0 ...', but less portable,
# and there probably aren't many projects with so many Makefile.am files
# that we'd have to worry about limits on command line length.
msg = '$(ME): Do not use `:'\'' above; use @PATH_SEPARATOR@ instead'
makefile_path_separator_check:
	@grep -n 'PATH=.*:' `find $(srcdir) -name Makefile.am` \
	  && { echo $(msg) 1>&2; exit 1; } || :

# Check that `make alpha' will not fail at the end of the process.
writable-files:
	if test -d $(release_archive_dir); then :; else			\
	  mkdir $(release_archive_dir);					\
	fi
	for file in $(distdir).tar.gz					\
		    $(release_archive_dir)/$(distdir).tar.gz; do	\
	  test -e $$file || continue;					\
	  test -w $$file						\
	    || { echo ERROR: $$file is not writable; fail=1; };		\
	done;								\
	test "$$fail" && exit 1 || :

v_etc_file = lib/version-etc.c
# Make sure that the copyright date in $(v_etc_file) is up to date.
copyright-check:
	@if test -f $(v_etc_file); then \
	  grep 'enum { COPYRIGHT_YEAR = '$$(date +%Y)' };' $(v_etc_file) \
	    >/dev/null \
	  || { echo 'out of date copyright in $(v_etc_file); update it' 1>&2; \
	       exit 1; }; \
	fi

vc-diff-check:
	(CDPATH=; cd $(srcdir) && $(VC) diff) > vc-diffs || :
	if test -s vc-diffs; then				\
	  cat vc-diffs;						\
	  echo "Some files are locally modified:" 1>&2;		\
	  exit 1;						\
	else							\
	  rm vc-diffs;						\
	fi

cvs-check: vc-diff-check

maintainer-distcheck:
	$(MAKE) distcheck
	$(MAKE) -C tests $(AM_MAKEFLAGS) maintainer-check
	$(MAKE) my-distcheck

# Don't make a distribution if checks fail.
# Also, make sure the NEWS file is up-to-date.
vc-dist: $(local-check) cvs-check maintainer-distcheck
	$(MAKE) dist

# Use this to make sure we don't run these programs when building
# from a virgin tgz file, below.
null_AM_MAKEFLAGS = \
  ACLOCAL=false \
  AUTOCONF=false \
  AUTOMAKE=false \
  AUTOHEADER=false \
  MAKEINFO=false

# Detect format-string/arg-list mismatches that would normally be obscured
# by the use of _().  The --disable-nls effectively defines away that macro,
# and building with CFLAGS='-Wformat -Werror' causes any format warning to be
# treated as a failure.  Also, check for shadowing problems with -Wshadow,
# and for pointer arithmetic problems with -Wpointer-arith.
# These CFLAGS are pretty strict.  If you build this target, you probably
# have to have a recent version of gcc and glibc headers.
TMPDIR ?= /tmp
t=$(TMPDIR)/$(PACKAGE)/test
my-distcheck: $(local-check) $(release_archive_dir)/$(prev-tgz)
	-rm -rf $(t)
	mkdir -p $(t)
	GZIP=$(GZIP_ENV) $(AMTAR) -C $(t) -zxf $(distdir).tar.gz
	cd $(t)/$(distdir)				\
	  && ./configure --disable-nls			\
	  && $(MAKE) CFLAGS='-Werror -Wall -Wformat -Wshadow -Wpointer-arith' \
	      AM_MAKEFLAGS='$(null_AM_MAKEFLAGS)'	\
	  && $(MAKE) dvi				\
	  && $(MAKE) check				\
	  && $(MAKE) distclean
	(cd $(t) && mv $(distdir) $(distdir).old	\
	  && $(AMTAR) -zxf - ) < $(distdir).tar.gz
	diff -ur $(t)/$(distdir).old $(t)/$(distdir)
	-rm -rf $(t)
	@echo "========================"; \
	echo "$(distdir).tar.gz is ready for distribution"; \
	echo "========================"

prev-tgz = $(PACKAGE)-$(PREV_VERSION).tar.gz

rel-files = $(DIST_ARCHIVES)

gnulib_dir ?= $(srcdir)/gnulib
gnulib-version = $$(cd $(gnulib_dir) && git describe)
bootstrap-tools ?= autoconf,automake,gnulib

# If it's not already specified, derive the GPG key ID from
# the signed tag we've just applied to mark this release.
gpg_key_ID ?= \
  $$(git cat-file tag v$(VERSION) > .ann-sig \
     && gpgv .ann-sig - < /dev/null 2>&1 \
	  | sed -n '/.*key ID \([0-9A-F]*\)/s//\1/p'; rm -f .ann-sig)

translation_project_ ?= coordinator@translationproject.org
announcement_Cc_ ?= $(translation_project_), $(PACKAGE_BUGREPORT)
announcement_mail_headers_ ?=						\
To: info-gnu@gnu.org							\
Cc: $(announcement_Cc_)							\
Mail-Followup-To: $(PACKAGE_BUGREPORT)

announcement: NEWS ChangeLog $(rel-files)
	@$(build_aux)/announce-gen					\
	    --mail-headers='$(announcement_mail_headers_)'		\
	    --release-type=$(RELEASE_TYPE)				\
	    --package=$(PACKAGE)					\
	    --prev=$(PREV_VERSION)					\
	    --curr=$(VERSION)						\
	    --gpg-key-id=$(gpg_key_ID)					\
	    --news=$(srcdir)/NEWS					\
	    --bootstrap-tools=$(bootstrap-tools)			\
	    --no-print-checksums					\
	    $(addprefix --url-dir=, $(url_dir_list))

## ---------------- ##
## Updating files.  ##
## ---------------- ##

ftp-gnu = ftp://ftp.gnu.org/gnu
www-gnu = http://www.gnu.org

# Use mv, if you don't have/want move-if-change.
move_if_change ?= move-if-change

upload_dest_dir_ ?= $(PACKAGE)

emit_upload_commands:
	@echo =====================================
	@echo =====================================
	@echo "$(build_aux)/gnupload $(GNUPLOADFLAGS) \\"
	@echo "    --to $(gnu_rel_host):$(upload_dest_dir_) \\"
	@echo "  $(rel-files)"
	@echo '# send the ~/announce-$(my_distdir) e-mail'
	@echo =====================================
	@echo =====================================

noteworthy = * Noteworthy changes in release ?.? (????-??-??) [?]
define emit-commit-log
  printf '%s\n' 'post-release administrivia' '' \
    '* NEWS: Add header line for next release.' \
    '* .prev-version: Record previous version.' \
    '* cfg.mk (old_NEWS_hash): Auto-update.'
endef

.PHONY: alpha beta stable
ALL_RECURSIVE_TARGETS += alpha beta stable
alpha beta stable: news-date-check changelog-check $(local-check)
	test $@ = stable						\
	  && { echo $(VERSION) | grep -E '^[0-9]+(\.[0-9]+)+$$'		\
	       || { echo "invalid version string: $(VERSION)" 1>&2; exit 1;};}\
	  || :
	$(MAKE) vc-dist
	$(MAKE) dist XZ_OPT=-9ev
	$(MAKE) $(release-prep-hook) RELEASE_TYPE=$@
	$(MAKE) -s emit_upload_commands RELEASE_TYPE=$@

# Override this in cfg.mk if you follow different procedures.
release-prep-hook ?= release-prep

.PHONY: release-prep
release-prep:
	case $$RELEASE_TYPE in alpha|beta|stable) ;; \
	  *) echo "invalid RELEASE_TYPE: $$RELEASE_TYPE" 1>&2; exit 1;; esac
	$(MAKE) -s announcement > ~/announce-$(my_distdir)
	if test -d $(release_archive_dir); then			\
	  ln $(rel-files) $(release_archive_dir);		\
	  chmod a-w $(rel-files);				\
	fi
	echo $(VERSION) > $(prev_version_file)
	$(MAKE) update-NEWS-hash
	perl -pi -e '$$. == 3 and print "$(noteworthy)\n\n\n"' NEWS
	$(emit-commit-log) > .ci-msg
	$(VC) commit -F .ci-msg -a
	rm .ci-msg


# Override this with e.g., -s $(srcdir)/some_other_name.texi
# if the default $(PACKAGE)-derived name doesn't apply.
gendocs_options_ ?=

.PHONY: web-manual
web-manual:
	@test -z "$(manual_title)" \
	  && { echo define manual_title in cfg.mk 1>&2; exit 1; } || :
	@cd '$(srcdir)/doc'; \
	  $(SHELL) ../build-aux/gendocs.sh $(gendocs_options_) \
	     -o '$(abs_builddir)/doc/manual' \
	     --email $(PACKAGE_BUGREPORT) $(PACKAGE) \
	    "$(PACKAGE_NAME) - $(manual_title)"
	@echo " *** Upload the doc/manual directory to web-cvs."

# If you want to set UPDATE_COPYRIGHT_* environment variables,
# put the assignments in this variable.
update-copyright-env ?=

# Run this rule once per year (usually early in January)
# to update all FSF copyright year lists in your project.
# If you have an additional project-specific rule,
# add it in cfg.mk along with a line 'update-copyright: prereq'.
# By default, exclude all variants of COPYING; you can also
# add exemptions (such as ChangeLog..* for rotated change logs)
# in the file .x-update-copyright.
.PHONY: update-copyright
update-copyright:
	grep -l -w Copyright $$($(VC_LIST_EXCEPT))		\
		$(srcdir)/ChangeLog | grep -v COPYING		\
	  | $(update-copyright-env) xargs $(build_aux)/$@
