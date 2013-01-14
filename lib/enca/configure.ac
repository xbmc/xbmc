dnl configure.in by David Necas (Yeti) <yeti@physics.muni.cz>
dnl @(#) $Id: configure.ac,v 1.34 2005/12/18 12:07:46 yeti Exp $
dnl This file is in public domain.
dnl Process this file with autoconf to produce a configure script.
dnl Shake before use.
AC_INIT(Enca, 1.12, [Michal Cihar <michal@cihar.com>])
AC_PREREQ(2.52)
AC_CONFIG_SRCDIR(src/enca.c)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES( \
  Makefile \
  enca.spec \
  enca.pc \
  devel-docs/Makefile \
  data/Makefile \
  lib/Makefile \
  script/Makefile \
  script/b-cstocs \
  script/b-map \
  script/b-piconv \
  script/b-umap \
  src/Makefile \
  src/HELP \
  test/Makefile \
  tools/Makefile)

AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE([1.8 gnits check-news dist-bzip2])
AM_ACLOCAL_INCLUDE(m4)
AM_MAINTAINER_MODE

dnl RPM release (normally always 1, may be manually changed by package
dnl maintainer in spec later)
RELEASE=1

dnl Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_GNU_SOURCE
AC_AIX
AC_ISC_POSIX
AC_PROG_LIBTOOL
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PATH_PROG(MKTEMP_PROG, mktemp)

dnl With GCC, be paranoiac.  It should compile really cleanly.
dnl Except -Wno-sign-compare, but older gcc's don't know it.
if test "$GCC" = yes; then
  CFLAGS="-Wall -W -pedantic $CFLAGS"
fi

dnl Check for various external converters.
AC_PATH_PROG(CSTOCS_PROG, cstocs)
AC_PATH_PROG(RECODE_PROG, recode)
AC_PATH_PROG(UMAP_PROG, umap)
AC_PATH_PROG(PICONV_PROG, piconv)
AC_PATH_PROG(MAP_PROG, map)

dnl External convetor.  It may be disabled later, if fork() doesn't work.
AC_ARG_ENABLE(external,
  [  --enable-external       enable conversion using external programs @<:@yes@:>@],
  [case "${enableval}" in
    yes) WANT_EXTERNAL=yes ;;
    no)  WANT_EXTERNAL=no ;;
    *)   AC_MSG_ERROR(bad value ${enableval} for --enable-external) ;;
    esac],
  [WANT_EXTERNAL=yes])

dnl Fool autoconf, it can't parse the for-cycle below
if false; then
AC_DEFINE_UNQUOTED(CSTOCS_PROG,"",[Define to path to the cstocs recoder.])
AC_DEFINE_UNQUOTED(MAP_PROG,"",[Define to path to the map recoder.])
AC_DEFINE_UNQUOTED(RECODE_PROG,"",[Define to path to the recode recoder.])
AC_DEFINE_UNQUOTED(PICONV_PROG,"",[Define to path to the piconv recoder.])
AC_DEFINE_UNQUOTED(UMAP_PROG,"",[Define to path to the umap recoder.])
fi

dnl Find default external converter
for w in cstocs recode piconv map umap; do
  wPROG=`echo $w | tr $as_cr_letters $as_cr_LETTERS`_PROG
  # Is this POSIX?  But even ash supports it.
  eval wPROGcv=$`echo ac_cv_path_$wPROG`
  if test -n "$wPROGcv"; then
    AC_DEFINE_UNQUOTED($wPROG,"$wPROGcv")
    if test -z "$DEFAULT_EXTERNAL_CONVERTER"; then
      DEFAULT_EXTERNAL_CONVERTER=$w
    fi
  fi
done

dnl And for gtk-doc
gtk_CHECK_GTK_DOC

dnl Check for good random number sources
AC_CHECK_FILES(/dev/random /dev/urandom /dev/srandom /dev/arandom)

dnl Dirty path hack.  Helps some people with badly set up search paths.
if test "$prefix" = "NONE"; then
  LDFLAGS="$LDFLAGS -L$ac_default_prefix/lib"
  CPPFLAGS="$CPPFLAGS -I$ac_default_prefix/include"
else
  LDFLAGS="$LDFLAGS -L$prefix/lib"
  CPPFLAGS="$CPPFLAGS -I$prefix/include"
fi

dnl Checks for libraries.
ye_CHECK_LIBM

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_HEADER_TIME
AC_HEADER_STAT
AC_HEADER_STDBOOL
AC_CHECK_HEADERS( \
  errno.h \
  fcntl.h \
  getopt.h \
  langinfo.h \
  limits.h \
  locale.h \
  memory.h \
  string.h \
  strings.h \
  sys/stat.h \
  sys/types.h \
  sys/wait.h \
  sys/time.h \
  time.h \
  unistd.h \
  wordexp.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_UID_T
AC_CHECK_TYPE(ssize_t, int)
# FIXME: This is crude.  It seems to work on Linux, though.
AC_CHECK_DECLS(LC_MESSAGES,,,[#include <locale.h>])

ye_CHECK_VAR_PROGRAM_INVOCATION_SHORT_NAME

dnl Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS( \
  canonicalize_file_name \
  ftruncate \
  gettimeofday \
  isatty \
  nl_langinfo \
  random \
  realpath \
  strstr \
  stpcpy \
  setlocale \
  ttyname \
  wordexp)
AC_REPLACE_FUNCS(getopt_long)
dnl Following two check for much more than just library functions, iconv test
dnl in fact has to build a simple iconv app to find out wheter it's usable.
CONVERTER_LIBS=
ye_CHECK_FUNC_ICONV_USABLE
ye_CHECK_LIB_RECODE

DEFAULT_CONVERTER_LIST="built-in"
if test "$librecode_ok" = "yes"; then
  DEFAULT_CONVERTER_LIST="$DEFAULT_CONVERTER_LIST,librecode"
else
  if test "$libiconv_ok" = "yes"; then
    DEFAULT_CONVERTER_LIST="$DEFAULT_CONVERTER_LIST,iconv"
  fi
fi
AC_DEFINE_UNQUOTED(DEFAULT_CONVERTER_LIST,"$DEFAULT_CONVERTER_LIST",[Define to default converter list.])

if test "$WANT_EXTERNAL" = "yes" -a "$ac_cv_func_fork_works" = "yes"; then
  AC_DEFINE(ENABLE_EXTERNAL,1,[Define to enable external converter programs.])
  if test -n "$DEFAULT_EXTERNAL_CONVERTER"; then
    enable_external="yes ($DEFAULT_EXTERNAL_CONVERTER)"
  else
    enable_external="yes (but no suitable found)"
  fi
else
  enable_external=no
  DEFAULT_EXTERNAL_CONVERTER=
fi
AC_DEFINE_UNQUOTED(DEFAULT_EXTERNAL_CONVERTER,"$DEFAULT_EXTERNAL_CONVERTER",[Define to default external converter program.])

dnl Try to find locale.alias.  FIXME! FIXME! FIXME! This works on GNU/Linux and
dnl perhaps nowhere else.  Any suggestions are welcome.
ye_PATH_LOCALE_ALIAS

dnl Random filename generation in scripts
if test -n "$MKTEMP_PROG"; then
  SHELL_RANDOM_FILENAME='`'$MKTEMP_PROG' /tmp/enca-$$-XXXXXXXX`'
else
  SHELL_RANDOM_FILENAME='/tmp/enca-$$-$RANDOM'
fi

AC_SUBST(RELEASE)
AC_SUBST(CONVERTER_LIBS)
AC_SUBST(DEFAULT_EXTERNAL_CONVERTER)
AC_SUBST(DEFAULT_CONVERTER_LIST)
AC_SUBST(SHELL_RANDOM_FILENAME)

AC_OUTPUT

dnl Inform user what optional features will be built.
if test "$yeti_recode_buggy" = yes; then
  librecode_state="$librecode_ok (buggy, see above)"
else
  librecode_state="$librecode_ok"
fi

if test -z "$yeti_cv_file_locale_alias"; then
  locale_alias_state="built-in (naive)"
else
  locale_alias_state="$yeti_cv_file_locale_alias"
fi

if test "$ac_cv_func_wordexp" = yes; then
  ENCAOPT_state="$ac_cv_func_wordexp"
else
  ENCAOPT_state="built-in parser (naive)"
fi

if test "$enable_static" = yes; then
  if test "$enable_shared" = yes; then
    libenca_state="static, shared"
  else
    libenca_state=static
  fi
else
  if test "$enable_shared" = yes; then
    # Can this happen?
    libenca_state=shared
  else
    libenca_state="not at all?"
  fi
fi

echo "================================================================="
echo "  Features:"
echo "    libenca will be built as:            $libenca_state"
echo "    GNU recode library interface:        $librecode_state"
echo "    UNIX98 iconv interface:              $libiconv_ok"
if test "$libiconv_ok" != "yes" && test "$librecode_ok" != "yes"; then
echo "    (consider installing at least one of GNU libiconv and GNU librecode)"
fi
echo "    External converters:                 $enable_external"
echo "    Language preferences from locale:    $ac_cv_func_setlocale"
if test "$ac_cv_func_setlocale" = yes; then
echo "    Language aliases decryption:         $locale_alias_state"
fi
echo "    Target charset from locale:          $ac_cv_func_nl_langinfo"
echo "    ENCAOPT environment variable:        $ENCAOPT_state"
echo "================================================================="

ye_WARN_FAIL($ac_cv_header_stdc,"ISO C headers")
ye_WARN_FAIL($ac_cv_header_unistd_h,"unistd.h")
ye_WARN_FAIL($ac_cv_header_sys_types_h,"sys/types.h")
ye_WARN_FAIL($ac_cv_header_sys_stat_h,"sys/stat.h")

echo
echo "Configure complete, now type \`make' to compile enca."
echo "If it compiles, don't forget to run \`make check'."
