# mbchar.m4 serial 2
dnl Copyright (C) 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl autoconf tests required for use of mbchar.m4
dnl From Bruno Haible.

AC_DEFUN([gl_MBCHAR],
[
  AC_REQUIRE([AC_GNU_SOURCE])
  dnl The following line is that so the user can test
  dnl HAVE_WCHAR_H && HAVE_WCTYPE_H before #include "mbchar.h".
  AC_CHECK_HEADERS_ONCE(wchar.h wctype.h)
  dnl Compile mbchar.c only if HAVE_WCHAR_H && HAVE_WCTYPE_H.
  if test $ac_cv_header_wchar_h = yes && test $ac_cv_header_wctype_h = yes; then
    AC_LIBOBJ([mbchar])
  fi
])
