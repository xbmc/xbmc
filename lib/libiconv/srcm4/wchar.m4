dnl A placeholder for ISO C99 <wchar.h>, for platforms that have issues.

dnl Copyright (C) 2007-2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Written by Eric Blake.

# wchar.m4 serial 23

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
  GNULIB_BTOWC=0;      AC_SUBST([GNULIB_BTOWC])
  GNULIB_WCTOB=0;      AC_SUBST([GNULIB_WCTOB])
  GNULIB_MBSINIT=0;    AC_SUBST([GNULIB_MBSINIT])
  GNULIB_MBRTOWC=0;    AC_SUBST([GNULIB_MBRTOWC])
  GNULIB_MBRLEN=0;     AC_SUBST([GNULIB_MBRLEN])
  GNULIB_MBSRTOWCS=0;  AC_SUBST([GNULIB_MBSRTOWCS])
  GNULIB_MBSNRTOWCS=0; AC_SUBST([GNULIB_MBSNRTOWCS])
  GNULIB_WCRTOMB=0;    AC_SUBST([GNULIB_WCRTOMB])
  GNULIB_WCSRTOMBS=0;  AC_SUBST([GNULIB_WCSRTOMBS])
  GNULIB_WCSNRTOMBS=0; AC_SUBST([GNULIB_WCSNRTOMBS])
  GNULIB_WCWIDTH=0;    AC_SUBST([GNULIB_WCWIDTH])
  dnl Assume proper GNU behavior unless another module says otherwise.
  HAVE_BTOWC=1;         AC_SUBST([HAVE_BTOWC])
  HAVE_MBSINIT=1;       AC_SUBST([HAVE_MBSINIT])
  HAVE_MBRTOWC=1;       AC_SUBST([HAVE_MBRTOWC])
  HAVE_MBRLEN=1;        AC_SUBST([HAVE_MBRLEN])
  HAVE_MBSRTOWCS=1;     AC_SUBST([HAVE_MBSRTOWCS])
  HAVE_MBSNRTOWCS=1;    AC_SUBST([HAVE_MBSNRTOWCS])
  HAVE_WCRTOMB=1;       AC_SUBST([HAVE_WCRTOMB])
  HAVE_WCSRTOMBS=1;     AC_SUBST([HAVE_WCSRTOMBS])
  HAVE_WCSNRTOMBS=1;    AC_SUBST([HAVE_WCSNRTOMBS])
  HAVE_DECL_WCTOB=1;    AC_SUBST([HAVE_DECL_WCTOB])
  HAVE_DECL_WCWIDTH=1;  AC_SUBST([HAVE_DECL_WCWIDTH])
  REPLACE_MBSTATE_T=0;  AC_SUBST([REPLACE_MBSTATE_T])
  REPLACE_BTOWC=0;      AC_SUBST([REPLACE_BTOWC])
  REPLACE_WCTOB=0;      AC_SUBST([REPLACE_WCTOB])
  REPLACE_MBSINIT=0;    AC_SUBST([REPLACE_MBSINIT])
  REPLACE_MBRTOWC=0;    AC_SUBST([REPLACE_MBRTOWC])
  REPLACE_MBRLEN=0;     AC_SUBST([REPLACE_MBRLEN])
  REPLACE_MBSRTOWCS=0;  AC_SUBST([REPLACE_MBSRTOWCS])
  REPLACE_MBSNRTOWCS=0; AC_SUBST([REPLACE_MBSNRTOWCS])
  REPLACE_WCRTOMB=0;    AC_SUBST([REPLACE_WCRTOMB])
  REPLACE_WCSRTOMBS=0;  AC_SUBST([REPLACE_WCSRTOMBS])
  REPLACE_WCSNRTOMBS=0; AC_SUBST([REPLACE_WCSNRTOMBS])
  REPLACE_WCWIDTH=0;    AC_SUBST([REPLACE_WCWIDTH])
  WCHAR_H='';           AC_SUBST([WCHAR_H])
])
