dnl A placeholder for ISO C99 <wchar.h>, for platforms that have issues.

dnl Copyright (C) 2007-2008 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Written by Eric Blake.

# wchar.m4 serial 6

AC_DEFUN([gl_WCHAR_H],
[
  AC_REQUIRE([gl_WCHAR_H_DEFAULTS])
  AC_CACHE_CHECK([whether <wchar.h> is standalone],
    [gl_cv_header_wchar_h_standalone],
    [AC_COMPILE_IFELSE([[#include <wchar.h>
wchar_t w;]],
      [gl_cv_header_wchar_h_standalone=yes],
      [gl_cv_header_wchar_h_standalone=no])])

  AC_REQUIRE([gt_TYPE_WINT_T])
  if test $gt_cv_c_wint_t = yes; then
    HAVE_WINT_T=1
  else
    HAVE_WINT_T=0
  fi
  AC_SUBST([HAVE_WINT_T])

  if test $gl_cv_header_wchar_h_standalone != yes || test $gt_cv_c_wint_t != yes; then
    WCHAR_H=wchar.h
  fi

  dnl Prepare for creating substitute <wchar.h>.
  dnl Do it always: WCHAR_H may be empty here but can be set later.
  dnl Check for <wchar.h> (missing in Linux uClibc when built without wide
  dnl character support).
  AC_CHECK_HEADERS_ONCE([wchar.h])
  if test $ac_cv_header_wchar_h = yes; then
    HAVE_WCHAR_H=1
  else
    HAVE_WCHAR_H=0
  fi
  AC_SUBST([HAVE_WCHAR_H])
  gl_CHECK_NEXT_HEADERS([wchar.h])
])

dnl Unconditionally enables the replacement of <wchar.h>.
AC_DEFUN([gl_REPLACE_WCHAR_H],
[
  AC_REQUIRE([gl_WCHAR_H_DEFAULTS])
  WCHAR_H=wchar.h
])

AC_DEFUN([gl_WCHAR_MODULE_INDICATOR],
[
  dnl Use AC_REQUIRE here, so that the default settings are expanded once only.
  AC_REQUIRE([gl_WCHAR_H_DEFAULTS])
  GNULIB_[]m4_translit([$1],[abcdefghijklmnopqrstuvwxyz./-],[ABCDEFGHIJKLMNOPQRSTUVWXYZ___])=1
])

AC_DEFUN([gl_WCHAR_H_DEFAULTS],
[
  GNULIB_WCWIDTH=0; AC_SUBST([GNULIB_WCWIDTH])
  dnl Assume proper GNU behavior unless another module says otherwise.
  HAVE_DECL_WCWIDTH=1; AC_SUBST([HAVE_DECL_WCWIDTH])
  REPLACE_WCWIDTH=0;   AC_SUBST([REPLACE_WCWIDTH])
  WCHAR_H='';          AC_SUBST([WCHAR_H])
])
